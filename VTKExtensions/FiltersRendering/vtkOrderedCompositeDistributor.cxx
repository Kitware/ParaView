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

#include "vtkCallbackCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRedistributeDataSetFilter.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkOrderedCompositeDistributor);
//-----------------------------------------------------------------------------
vtkOrderedCompositeDistributor::vtkOrderedCompositeDistributor()
{
  this->RedistributeDataSetFilter->SetUseExplicitCuts(true);
  this->RedistributeDataSetFilter->SetGenerateGlobalCellIds(false);
}

//-----------------------------------------------------------------------------
vtkOrderedCompositeDistributor::~vtkOrderedCompositeDistributor()
{
}

//-----------------------------------------------------------------------------
void vtkOrderedCompositeDistributor::SetCuts(const std::vector<vtkBoundingBox>& boxes)
{
  this->RedistributeDataSetFilter->SetExplicitCuts(boxes);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOrderedCompositeDistributor::SetController(vtkMultiProcessController* controller)
{
  this->RedistributeDataSetFilter->SetController(controller);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOrderedCompositeDistributor::SetBoundaryMode(int mode)
{
  this->RedistributeDataSetFilter->SetBoundaryMode(mode);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOrderedCompositeDistributor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkOrderedCompositeDistributor::FillInputPortInformation(int, vtkInformation* info)
{
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkOrderedCompositeDistributor::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkDataObject::GetData(outputVector, 0);
  if (input != nullptr && vtkPolyData::SafeDownCast(input))
  {
    if (vtkPolyData::SafeDownCast(output) == nullptr)
    {
      output = vtkPolyData::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    return 1;
  }
  else
  {
    return this->RedistributeDataSetFilter->ProcessRequest(request, inputVector, outputVector);
  }
}

//-----------------------------------------------------------------------------
int vtkOrderedCompositeDistributor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputDO = vtkDataObject::GetData(outputVector, 0);

  this->RedistributeDataSetFilter->SetInputDataObject(inputDO);
  this->RedistributeDataSetFilter->Update();
  auto distributedData = this->RedistributeDataSetFilter->GetOutputDataObject(0);
  if (vtkPolyData::SafeDownCast(inputDO))
  {
    vtkNew<vtkDataSetSurfaceFilter> converter;
    converter->UnstructuredGridExecute(
      vtkDataSet::SafeDownCast(distributedData), vtkPolyData::SafeDownCast(outputDO));
    return 1;
  }
  else if (vtkDataSet::SafeDownCast(inputDO))
  {
    assert(vtkUnstructuredGrid::SafeDownCast(outputDO) &&
      vtkUnstructuredGrid::SafeDownCast(distributedData));
    outputDO->ShallowCopy(distributedData);
    return 1;
  }
  else if (auto cd = vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // now handle the worst case, where we have composite dataset input.
    // things can get complicated here but we use a simple heuristic: if all input
    // leaf nodes are poly-data, then we pass output as polydata
    bool convert_to_polydata = true;
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (vtkPolyData::SafeDownCast(iter->GetCurrentDataObject()) == nullptr)
      {
        convert_to_polydata = false;
        break;
      }
    }
    iter->Delete();
    if (convert_to_polydata)
    {
      vtkNew<vtkDataSetSurfaceFilter> convertor;
      convertor->SetInputDataObject(distributedData);
      convertor->Update();
      outputDO->ShallowCopy(convertor->GetOutputDataObject(0));
      return 1;
    }
    else
    {
      outputDO->SafeDownCast(distributedData);
    }
  }
  return 1;
}
