/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAllToNRedistributeCompositePolyData.h"

#include "vtkAllToNRedistributePolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkAllToNRedistributeCompositePolyData);
vtkCxxSetObjectMacro(vtkAllToNRedistributeCompositePolyData, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkAllToNRedistributeCompositePolyData::vtkAllToNRedistributeCompositePolyData()
{
  this->Controller = 0;
  this->NumberOfProcesses = 1;
}

//----------------------------------------------------------------------------
vtkAllToNRedistributeCompositePolyData::~vtkAllToNRedistributeCompositePolyData()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkAllToNRedistributeCompositePolyData::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkAllToNRedistributeCompositePolyData::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkAllToNRedistributeCompositePolyData::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* output = vtkDataSet::GetData(outputVector, 0);

  if (input)
  {
    // If input is composite-data, then output is multi-block of polydata,
    // otherwise it's a poly data.
    if (vtkCompositeDataSet::SafeDownCast(input))
    {
      // Some developers have sub-classed vtkMultiBlockDataSet, in which
      // case, we try to preserve the type.
      output = input->NewInstance();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
      return 1;
    }

    if (vtkPolyData::SafeDownCast(output) == NULL)
    {
      output = vtkPolyData::New();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), output);
      output->FastDelete();
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkAllToNRedistributeCompositePolyData::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);

  vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::SafeDownCast(output);
  if (!cdInput)
  {
    vtkPolyData* inputCopy = vtkPolyData::New();
    inputCopy->ShallowCopy(input);
    vtkAllToNRedistributePolyData* allToN = vtkAllToNRedistributePolyData::New();
    allToN->SetController(this->Controller);
    allToN->SetNumberOfProcesses(this->NumberOfProcesses);
    allToN->SetInputData(inputCopy);
    allToN->Update();
    output->ShallowCopy(allToN->GetOutput());
    inputCopy->Delete();
    allToN->Delete();
    return 1;
  }

  cdOutput->CopyStructure(cdInput);

  vtkAllToNRedistributePolyData* allToN = vtkAllToNRedistributePolyData::New();
  allToN->SetController(this->Controller);
  allToN->SetNumberOfProcesses(this->NumberOfProcesses);

  // This assumes that the vtkPVGeometryFilter has ensured that all processes
  // have the same non-null leaf nodes.
  vtkCompositeDataIterator* iter = cdInput->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPolyData* pdInput = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    if (pdInput)
    {
      vtkPolyData* pdOutput = vtkPolyData::New();
      cdOutput->SetDataSet(iter, pdOutput);
      pdOutput->FastDelete();

      allToN->SetInputData(pdInput);
      allToN->Modified(); // essential to force modification just in case the two
      // blocks are the same polydata instance on any one of the processes.
      allToN->Update();
      pdOutput->ShallowCopy(allToN->GetOutput());
    }
  }

  allToN->Delete();
  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkAllToNRedistributeCompositePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
