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
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntegrateAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSurfaceVectors.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkIntegrateFlowThroughSurface, "1.2");
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
int vtkIntegrateFlowThroughSurface::RequestData(vtkInformation *vtkNotUsed(request),
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
  vtkDataSet* inputCopy;

  inputCopy = input->NewInstance();
  inputCopy->CopyStructure(input);
  vtkDataArray *vectors = this->GetInputArrayToProcess(0,inputVector);
  if (vectors == 0)
    {
    vtkErrorMacro("Missing Vectors.");
    inputCopy->Delete();
    return 1;
    }
  inputCopy->GetPointData()->SetVectors(vectors);
  inputCopy->GetCellData()->AddArray(
                               input->GetCellData()->GetArray("vtkGhostLevels"));

  vtkSurfaceVectors* dot = vtkSurfaceVectors::New();
  dot->SetInput(inputCopy);
  dot->SetConstraintModeToPerpendicularScale();
  vtkIntegrateAttributes* integrate = vtkIntegrateAttributes::New();
  integrate->SetInput(dot->GetOutput());
  vtkUnstructuredGrid* integrateOutput = integrate->GetOutput();
  integrateOutput->Update();

  output->CopyStructure(integrateOutput);
  output->GetPointData()->PassData(integrateOutput->GetPointData());
  vtkDataArray* flow = output->GetPointData()->GetArray("Perpendicular Scale");
  if (flow)
    {
    flow->SetName("Surface Flow");
    }  
  
  dot->Delete();
  dot = 0;
  integrate->Delete();
  integrate = 0;
  inputCopy->Delete();
  inputCopy = 0;

  return 1;
}        

//-----------------------------------------------------------------------------
void vtkIntegrateFlowThroughSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkIntegrateFlowThroughSurface::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
