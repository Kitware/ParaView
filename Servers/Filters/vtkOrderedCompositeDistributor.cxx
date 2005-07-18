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

#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDistributedDataFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkOrderedCompositeDistributor, "1.1");
vtkStandardNewMacro(vtkOrderedCompositeDistributor);

vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, PKdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, Controller,
                     vtkMultiProcessController);

//-----------------------------------------------------------------------------

vtkOrderedCompositeDistributor::vtkOrderedCompositeDistributor()
{
  this->PKdTree = NULL;
  this->Controller = NULL;

  this->D3 = NULL;
  this->ToPolyData = NULL;

  this->PassThrough = 0;
}

vtkOrderedCompositeDistributor::~vtkOrderedCompositeDistributor()
{
  this->SetPKdTree(NULL);
  this->SetController(NULL);

  if (this->D3) this->D3->Delete();
  if (this->ToPolyData) this->ToPolyData->Delete();
}

void vtkOrderedCompositeDistributor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "PKdTree: " << this->PKdTree << endl;
  os << "Controller: " << this->Controller << endl;
  os << "PassThrough: " << this->PassThrough << endl;
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

  // Get the input and output.
  vtkDataSet *input = vtkDataSet::SafeDownCast(
                                     inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
                                    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->PassThrough)
    {
    // Don't do anything to the data.
    output->ShallowCopy(input);
    return 1;
    }

  if (this->D3 == NULL)
    {
    this->D3 = vtkDistributedDataFilter::New();
    }

  this->D3->SetBoundaryModeToSplitBoundaryCells();
  this->D3->SetInput(input);
  this->D3->GetKdtree()->SetCuts(this->PKdTree->GetCuts());
  this->D3->SetController(this->Controller);
  this->D3->Update();

  if (input->IsA("vtkUnstructuredGrid"))
    {
    output->ShallowCopy(this->D3->GetOutput());
    }
  else if (input->IsA("vtkPolyData"))
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

  return 1;
}
