/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedCompositeDistributor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2005 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkOrderedCompositeDistributor.h"

#include "vtkBSPCuts.h"
#include "vtkCallbackCommand.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPolyData.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_USE_MPI 
#include "vtkUnstructuredGrid.h"

#ifdef PARAVIEW_USE_MPI
# include "vtkDistributedDataFilter.h"
#endif

//-----------------------------------------------------------------------------
#ifdef PARAVIEW_USE_MPI
static void D3UpdateProgress(vtkObject *_D3, unsigned long,
                             void *_distributor, void *)
{
  vtkDistributedDataFilter *D3
    = reinterpret_cast<vtkDistributedDataFilter *>(_D3);
  vtkOrderedCompositeDistributor *distributor
    = reinterpret_cast<vtkOrderedCompositeDistributor *>(_distributor);

  distributor->SetProgressText(D3->GetProgressText());
  distributor->UpdateProgress(D3->GetProgress() * 0.9);
}
#endif
//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkOrderedCompositeDistributor);

vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, PKdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, Controller,
                     vtkMultiProcessController);
#ifdef PARAVIEW_USE_MPI
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, D3,
                     vtkDistributedDataFilter);
#endif
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, ToPolyData,
                     vtkDataSetSurfaceFilter);

//-----------------------------------------------------------------------------

vtkOrderedCompositeDistributor::vtkOrderedCompositeDistributor()
{
  this->PKdTree = NULL;
  this->Controller = NULL;

#ifdef PARAVIEW_USE_MPI
  this->D3 = NULL;
#endif
  this->ToPolyData = NULL;

  this->PassThrough = 0;

  this->OutputType = NULL;

  this->LastInput = NULL;
  this->LastOutput = NULL;
  this->LastCuts = vtkBSPCuts::New();
}

vtkOrderedCompositeDistributor::~vtkOrderedCompositeDistributor()
{
  this->SetPKdTree(NULL);
  this->SetController(NULL);

#ifdef PARAVIEW_USE_MPI
  this->SetD3(NULL);
#endif
  this->SetToPolyData(NULL);

  this->SetOutputType(NULL);

  if (this->LastOutput) this->LastOutput->Delete();
  if (this->LastCuts) this->LastCuts->Delete();
}

void vtkOrderedCompositeDistributor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PKdTree: " << this->PKdTree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "OutputType: " << 
    (this->OutputType? this->OutputType : "(none)") << endl;
#ifdef PARAVIEW_USE_MPI
  os << indent << "D3: " << this->D3 << endl;
#endif
  os << indent << "ToPolyData" << this->ToPolyData << endl;
}

//-----------------------------------------------------------------------------

void vtkOrderedCompositeDistributor::ReportReferences(
                                                 vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
#ifdef PARAVIEW_USE_MPI
  vtkGarbageCollectorReport(collector, this->D3, "D3");
#endif
  vtkGarbageCollectorReport(collector, this->ToPolyData, "ToPolyData");
  vtkGarbageCollectorReport(collector, this->PKdTree, "PKdTree");
  vtkGarbageCollectorReport(collector, this->Controller, "Controller");
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::FillInputPortInformation(
                                                 int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::RequestDataObject(
                                             vtkInformation* request,
                                             vtkInformationVector** inputVector,
                                             vtkInformationVector* outputVector)
{
  if (!this->OutputType || this->OutputType[0] == '\0')
    {
    return this->Superclass::RequestDataObject(request,
                                               inputVector, outputVector);
    }


  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

    if (!output || !output->IsA(this->OutputType)) 
      {
      output = vtkDataObjectTypes::NewDataObject(this->OutputType);
      if (!output)
        {
        return 0;
        }
      info->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                             output->GetExtentType());
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::RequestData(
                                            vtkInformation *vtkNotUsed(request),
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  // Get the info objects.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (!inInfo || !outInfo)
    {
    // Ignore request.
    return 1;
    }

  // Get the input and output.
  vtkDataSet *input = vtkDataSet::SafeDownCast(
                                     inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
                                    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input)
    {
    // Ignore request.
    return 1;
    }

  if (this->PassThrough || Controller->GetNumberOfProcesses() == 1)
    {
    // Don't do anything to the data.
    output->ShallowCopy(input);
    return 1;
    }
#ifdef PARAVIEW_USE_MPI
  if (!this->PKdTree)
    {
    vtkWarningMacro("No PKdTree set. vtkOrderedCompositeDistributor requires that"
      " at least an empty PKdTree be set.");
    }

  vtkBSPCuts *cuts = this->PKdTree? this->PKdTree->GetCuts() : NULL;
  if (cuts == NULL)
    {
    // No partitioning has been defined.  Just pass the data through.
    output->ShallowCopy(input);
    return 1;
    }

  // Check to see if update needs to be done (parallel operation), and pass
  // through old data if not.
  int needUpdate = 0;
  if (   (this->LastInput != input)
      || (this->LastUpdate < input->GetMTime())
      || !this->LastCuts->Equals(cuts) )
    {
    needUpdate = 1;
    }

  int reduced_needUpdate = 0;
  this->Controller->AllReduce(&needUpdate, &reduced_needUpdate, 1,
    vtkCommunicator::MAX_OP);
  if (!reduced_needUpdate)
    {
    output->ShallowCopy(this->LastOutput);
    return 1;
    }

  // Handle the case where all inputs on all processes are empty.
  double bounds[6];
  input->GetBounds(bounds);
  int valid_bounds = vtkMath::AreBoundsInitialized(bounds);
  int reduced_valid_bounds = 0;
  this->Controller->AllReduce(&valid_bounds, &reduced_valid_bounds, 1,
    vtkCommunicator::MAX_OP);
  if (!reduced_valid_bounds)
    {
    output->ShallowCopy(input);
    return 1;
    }

  this->UpdateProgress(0.01);

  if (this->D3 == NULL)
    {
    this->D3 = vtkDistributedDataFilter::New();
    }

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(D3UpdateProgress);
  this->D3->AddObserver(vtkCommand::ProgressEvent, cbc);

  this->D3->SetBoundaryModeToSplitBoundaryCells();
  this->D3->SetInputData(input);
  this->D3->SetCuts(cuts);
  // We need to pass the region assignments from PKdTree to D3
  // (Refer to BUG #10828).
  this->D3->SetUserRegionAssignments(
    this->PKdTree->GetRegionAssignmentMap(),
    this->PKdTree->GetRegionAssignmentMapLength());
  this->D3->SetController(this->Controller);
  this->D3->Modified();
  this->D3->Update();

  this->D3->RemoveObserver(cbc);
  cbc->Delete();

  if (output->IsA("vtkUnstructuredGrid"))
    {
    output->ShallowCopy(this->D3->GetOutput());
    }
  else if (output->IsA("vtkPolyData"))
    {
    if (this->ToPolyData == NULL)
      {
      this->ToPolyData = vtkDataSetSurfaceFilter::New();
      }
    this->ToPolyData->SetInputConnection(0, this->D3->GetOutputPort(0));
    this->ToPolyData->Update();

    output->ShallowCopy(this->ToPolyData->GetOutput());
    }
  else
    {
    vtkErrorMacro(<< "vtkOrderedCompositeDistributor used with unsupported "
                  << "type.");
    return 0;
    }

  this->LastUpdate.Modified();
  this->LastInput = input;
  this->LastCuts->CreateCuts(cuts->GetKdNodeTree());

  if (this->LastOutput && !this->LastOutput->IsA(output->GetClassName()))
    {
    this->LastOutput->Delete();
    this->LastOutput = NULL;
    }
  if (!this->LastOutput)
    {
    this->LastOutput = output->NewInstance();
    }
  this->LastOutput->ShallowCopy(output);

  return 1;
#endif
}
