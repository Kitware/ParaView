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
#include "vtkDataSetSurfaceFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#if VTK_MODULE_ENABLE_VTK_FiltersParallelMPI
#include "vtkDistributedDataFilter.h"
#endif

//-----------------------------------------------------------------------------
#if VTK_MODULE_ENABLE_VTK_FiltersParallelMPI
static void D3UpdateProgress(vtkObject* _D3, unsigned long, void* _distributor, void*)
{
  vtkDistributedDataFilter* D3 = reinterpret_cast<vtkDistributedDataFilter*>(_D3);
  vtkOrderedCompositeDistributor* distributor =
    reinterpret_cast<vtkOrderedCompositeDistributor*>(_distributor);

  distributor->SetProgressText(D3->GetProgressText());
  distributor->UpdateProgress(D3->GetProgress() * 0.9);
}
#endif
//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkOrderedCompositeDistributor);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, PKdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkOrderedCompositeDistributor::vtkOrderedCompositeDistributor()
{
  this->BoundaryMode = SPLIT_BOUNDARY_CELLS;
  this->PKdTree = NULL;
  this->Controller = NULL;
  this->PassThrough = false;
  this->OutputType = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkOrderedCompositeDistributor::~vtkOrderedCompositeDistributor()
{
  this->SetPKdTree(NULL);
  this->SetController(NULL);
  this->SetOutputType(NULL);
}

//-----------------------------------------------------------------------------
void vtkOrderedCompositeDistributor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BoundaryMode: " << this->BoundaryMode << endl;
  os << indent << "PKdTree: " << this->PKdTree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "OutputType: " << (this->OutputType ? this->OutputType : "(none)") << endl;
}

//-----------------------------------------------------------------------------
int vtkOrderedCompositeDistributor::FillInputPortInformation(int port, vtkInformation* info)
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
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->OutputType || this->OutputType[0] == '\0')
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }

  // for each output
  for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
  {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

    if (!output || !output->IsA(this->OutputType))
    {
      output = vtkDataObjectTypes::NewDataObject(this->OutputType);
      if (!output)
      {
        return 0;
      }
      info->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!inInfo || !outInfo)
  {
    // Ignore request.
    return 1;
  }

  // Get the input and output.
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input)
  {
    // Ignore request.
    return 1;
  }

  if (this->PassThrough || this->Controller == NULL ||
    this->Controller->GetNumberOfProcesses() == 1)
  {
    // Don't do anything to the data.
    output->ShallowCopy(input);
    return 1;
  }

#if VTK_MODULE_ENABLE_VTK_FiltersParallelMPI
  if (!this->PKdTree)
  {
    vtkWarningMacro("No PKdTree set. vtkOrderedCompositeDistributor requires that"
                    " at least an empty PKdTree be set.");
  }

  vtkBSPCuts* cuts = this->PKdTree ? this->PKdTree->GetCuts() : NULL;
  if (cuts == NULL)
  {
    // No partitioning has been defined.  Just pass the data through.
    output->ShallowCopy(input);
    return 1;
  }

  // Handle the case where all inputs on all processes are empty.
  double bounds[6];
  input->GetBounds(bounds);
  int valid_bounds = vtkMath::AreBoundsInitialized(bounds);
  int reduced_valid_bounds = 0;
  this->Controller->AllReduce(&valid_bounds, &reduced_valid_bounds, 1, vtkCommunicator::MAX_OP);
  if (!reduced_valid_bounds)
  {
    output->ShallowCopy(input);
    return 1;
  }

  this->UpdateProgress(0.01);

  vtkNew<vtkDistributedDataFilter> d3;

  // add progress observer.
  vtkNew<vtkCallbackCommand> cbc;
  cbc->SetClientData(this);
  cbc->SetCallback(D3UpdateProgress);
  d3->AddObserver(vtkCommand::ProgressEvent, cbc.GetPointer());
  switch (this->BoundaryMode)
  {
    case SPLIT_BOUNDARY_CELLS:
      d3->SetBoundaryModeToSplitBoundaryCells();
      break;
    case ASSIGN_TO_ONE_REGION:
      d3->SetBoundaryModeToAssignToOneRegion();
      break;
    case ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      d3->SetBoundaryModeToAssignToAllIntersectingRegions();
      break;
  }
  d3->SetInputData(input);
  d3->SetCuts(cuts);

  // We need to pass the region assignments from PKdTree to D3
  // (Refer to BUG #10828).
  d3->SetUserRegionAssignments(
    this->PKdTree->GetRegionAssignmentMap(), this->PKdTree->GetRegionAssignmentMapLength());
  d3->SetController(this->Controller);
  // d3->SetClipAlgorithmType(vtkDistributedDataFilter::USE_TABLEBASEDCLIPDATASET);
  d3->Update();

  vtkDataSet* distributedData = vtkDataSet::SafeDownCast(d3->GetOutputDataObject(0));
  // D3 can result in certain processes having empty datasets. Since we use
  // internal methods on vtkDataSetSurfaceFilter, they are not empty-data safe
  // and hence can segfault. This check avoids such segfaults.
  if (distributedData && distributedData->GetNumberOfPoints() > 0 &&
    distributedData->GetNumberOfCells() > 0)
  {
    if (output->IsA("vtkUnstructuredGrid"))
    {
      output->ShallowCopy(distributedData);
    }
    else if (output->IsA("vtkPolyData"))
    {
      vtkNew<vtkDataSetSurfaceFilter> converter;
      converter->UnstructuredGridExecute(distributedData, vtkPolyData::SafeDownCast(output));
    }
    else
    {
      vtkErrorMacro(<< "vtkOrderedCompositeDistributor used with unsupported "
                    << "type.");
      return 0;
    }
  }
#endif

  return 1;
}
