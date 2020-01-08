/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntegrateFlowThroughSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIntegrateFlowThroughSurface.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntegrateAttributes.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSurfaceVectors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkIntegrateFlowThroughSurface);

//-----------------------------------------------------------------------------
vtkIntegrateFlowThroughSurface::vtkIntegrateFlowThroughSurface()
{
  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

//-----------------------------------------------------------------------------
vtkIntegrateFlowThroughSurface::~vtkIntegrateFlowThroughSurface()
{
}

//-----------------------------------------------------------------------------
int vtkIntegrateFlowThroughSurface::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1);

  return 1;
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkIntegrateFlowThroughSurface::GenerateSurfaceVectors(vtkDataSet* input)
{
  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->CopyStructure(input);
  vtkDataArray* vectors = this->GetInputArrayToProcess(0, input);
  if (vectors)
  {
    inputCopy->GetPointData()->SetVectors(vectors);
  }
  inputCopy->GetCellData()->AddArray(input->GetCellGhostArray());

  vtkSurfaceVectors* dot = vtkSurfaceVectors::New();
  dot->SetInputData(inputCopy);
  dot->SetConstraintModeToPerpendicularScale();
  dot->Update();

  vtkDataSet* output = dot->GetOutput();
  vtkDataSet* outputCopy = output->NewInstance();
  outputCopy->ShallowCopy(output);

  dot->Delete();
  inputCopy->Delete();

  return outputCopy;
}

//-----------------------------------------------------------------------------
int vtkIntegrateFlowThroughSurface::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkSmartPointer<vtkDataObject> input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIntegrateAttributes* integrate = vtkIntegrateAttributes::New();
  vtkCompositeDataSet* hdInput =
    vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (hdInput)
  {
    vtkMultiBlockDataSet* hds = vtkMultiBlockDataSet::New();
    vtkCompositeDataIterator* iter = hdInput->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        vtkDataSet* intermData = this->GenerateSurfaceVectors(ds);
        if (intermData)
        {
          hds->SetBlock(hds->GetNumberOfBlocks(), intermData);
          intermData->Delete();
        }
      }
      iter->GoToNextItem();
    }
    iter->Delete();
    inInfo->Set(vtkDataObject::DATA_OBJECT(), hds);
    hds->Delete();
  }
  else if (dsInput)
  {
    vtkDataSet* intermData = this->GenerateSurfaceVectors(dsInput);
    if (!intermData)
    {
      return 1;
    }
    inInfo->Set(vtkDataSet::DATA_OBJECT(), intermData);
    intermData->Delete();
  }
  else
  {
    if (input)
    {
      vtkErrorMacro("This filter cannot handle input of type: " << input->GetClassName());
    }
    return 0;
  }

  integrate->ProcessRequest(request, inputVector, outputVector);

  if (hdInput)
  {
    inInfo->Set(vtkDataObject::DATA_OBJECT(), hdInput);
  }
  else if (dsInput)
  {
    inInfo->Set(vtkDataObject::DATA_OBJECT(), dsInput);
  }

  vtkDataArray* flow = output->GetPointData()->GetArray("Perpendicular Scale");
  if (flow)
  {
    flow->SetName("Surface Flow");
  }

  integrate->Delete();
  integrate = 0;

  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkIntegrateFlowThroughSurface::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void vtkIntegrateFlowThroughSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkIntegrateFlowThroughSurface::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}
