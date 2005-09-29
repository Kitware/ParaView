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
#include "vtkMultiGroupDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntegrateAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSurfaceVectors.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkIntegrateFlowThroughSurface, "1.4");
vtkStandardNewMacro(vtkIntegrateFlowThroughSurface);

//-----------------------------------------------------------------------------
vtkIntegrateFlowThroughSurface::vtkIntegrateFlowThroughSurface()
{
  // by default process active point vectors
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);
}

//-----------------------------------------------------------------------------
vtkIntegrateFlowThroughSurface::~vtkIntegrateFlowThroughSurface()
{
}

//-----------------------------------------------------------------------------
int vtkIntegrateFlowThroughSurface::RequestUpdateExtent(
                                           vtkInformation * vtkNotUsed(request),
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 
              outInfo->Get(vtkStreamingDemandDrivenPipeline::
                           UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1);

  return 1;
}

//-----------------------------------------------------------------------------
vtkDataSet* vtkIntegrateFlowThroughSurface::GenerateSurfaceVectors(
  vtkDataSet* input, vtkInformationVector **inputVector)
{
  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->CopyStructure(input);
  vtkDataArray *vectors = this->GetInputArrayToProcess(0,inputVector);
  if (vectors == 0)
    {
    vtkErrorMacro("Missing Vectors.");
    inputCopy->Delete();
    return 0;
    }
  inputCopy->GetPointData()->SetVectors(vectors);
  inputCopy->GetCellData()->AddArray(
    input->GetCellData()->GetArray("vtkGhostLevels"));

  vtkSurfaceVectors* dot = vtkSurfaceVectors::New();
  dot->SetInput(inputCopy);
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
int vtkIntegrateFlowThroughSurface::RequestData(vtkInformation *request,
                                                vtkInformationVector **inputVector,
                                                vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIntegrateAttributes* integrate = vtkIntegrateAttributes::New();
  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (hdInput) 
    {
    vtkMultiGroupDataSet* hds = vtkMultiGroupDataSet::New();
    vtkCompositeDataIterator* iter = hdInput->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
        {
        vtkDataSet* intermData = this->GenerateSurfaceVectors(input, inputVector);
        hds->SetDataSet(0, hds->GetNumberOfDataSets(0), intermData);
        intermData->Delete();
        }
      iter->GoToNextItem();
      }
    iter->Delete();
    inInfo->Set(vtkCompositeDataSet::COMPOSITE_DATA_SET(), hds);
    hds->Delete();
    }
  else
    {
    vtkDataSet* intermData = this->GenerateSurfaceVectors(input, inputVector);
    if (!intermData)
      {
      return 0;
      }
    inInfo->Set(vtkDataSet::DATA_OBJECT(), intermData);
    intermData->Delete();
    }

  integrate->ProcessRequest(request, inputVector, outputVector);

  if (hdInput) 
    {
    inInfo->Set(vtkCompositeDataSet::COMPOSITE_DATA_SET(), hdInput);
    }
  else
    {
    inInfo->Set(vtkDataObject::DATA_OBJECT(), input);
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
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkIntegrateFlowThroughSurface::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkCompositeDataSet");
  return 1;
}
