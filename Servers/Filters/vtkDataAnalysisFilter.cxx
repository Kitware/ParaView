/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataAnalysisFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataAnalysisFilter.h"

#include "vtkAlgorithm.h"
#include "vtkAppendFilter.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPPickFilter.h"
#include "vtkPProbeFilter.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkDataAnalysisFilter);
vtkCxxRevisionMacro(vtkDataAnalysisFilter, "1.1.2.1");
vtkCxxSetObjectMacro(vtkDataAnalysisFilter, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkDataAnalysisFilter::vtkDataAnalysisFilter()
{
  this->SetNumberOfInputPorts(2);
  this->Mode = vtkDataAnalysisFilter::PROBE;
  this->ProbeFilter = 0;
  this->PickFilter = 0;
  this->DataSetToUnstructuredGridFilter =0;

  this->SpatialMatch = 0;

  this->PickCell = 0;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->GlobalCellIdArrayName = 0;
  this->SetGlobalCellIdArrayName("GlobalElementId");
  this->GlobalPointIdArrayName = 0;
  this->SetGlobalPointIdArrayName("GlobalNodeId");
  this->Id = 0;
  this->UseIdToPick = 0;
  this->WorldPoint[0] = this->WorldPoint[1] = this->WorldPoint[2] = 0.0;
}

//-----------------------------------------------------------------------------
vtkDataAnalysisFilter::~vtkDataAnalysisFilter()
{
  if (this->ProbeFilter)
    {
    this->ProbeFilter->Delete();
    this->ProbeFilter = 0;
    }
  if (this->PickFilter)
    {
    this->PickFilter->Delete();
    this->PickFilter = 0;
    }
  if (this->DataSetToUnstructuredGridFilter)
    {
    this->DataSetToUnstructuredGridFilter->Delete();
    this->DataSetToUnstructuredGridFilter =0;
    }
  this->SetController(0);
  this->SetGlobalCellIdArrayName(0);  
  this->SetGlobalPointIdArrayName(0);  
}
//----------------------------------------------------------------------------
void vtkDataAnalysisFilter::SetSource(vtkDataObject *input)
{
  this->SetInput(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataAnalysisFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  
  return this->GetExecutive()->GetInputData(1, 0);
}
//----------------------------------------------------------------------------
int vtkDataAnalysisFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  if (port > 0)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}
//-----------------------------------------------------------------------------
int vtkDataAnalysisFilter::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = (!sourceInfo)? 0: vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if ( this->Mode == vtkDataAnalysisFilter::PROBE && !source)
    {
    vtkErrorMacro("No source provided.");
    return 0;
    }
  if (this->Mode == vtkDataAnalysisFilter::PROBE)
    {
    if (!this->ProbeFilter)
      {
      this->ProbeFilter = vtkPProbeFilter::New();
      }
    if (!this->DataSetToUnstructuredGridFilter)
      {
      this->DataSetToUnstructuredGridFilter = vtkAppendFilter::New();
      }
    this->DataSetToUnstructuredGridFilter->AddInput(source);
    vtkPProbeFilter::SafeDownCast(this->ProbeFilter)->SetController(
      this->Controller);
    this->ProbeFilter->SetInputConnection(
      this->DataSetToUnstructuredGridFilter->GetOutputPort());
    this->ProbeFilter->SetSpatialMatch(this->SpatialMatch);
    this->ProbeFilter->SetSource(input);
    this->ProbeFilter->Update();
    // If we don't release the inputs here, the append filter leaks
    // when probing line and the entire application is closed directly
    // (not if the probe source is deleted first and then the application 
    // is closed).
    this->DataSetToUnstructuredGridFilter->RemoveAllInputs();
    output->ShallowCopy(this->ProbeFilter->GetOutput());
    }
  else
    {
    if (!this->PickFilter)
      {
      this->PickFilter = vtkPPickFilter::New();
      }
    this->PickFilter->RemoveAllInputs();
    this->PickFilter->AddInput(input);
    this->PickFilter->SetPickCell(this->PickCell);
    this->PickFilter->SetUseIdToPick(this->UseIdToPick);
    this->PickFilter->SetWorldPoint(this->WorldPoint);
    this->PickFilter->SetId(this->Id);
    this->PickFilter->SetGlobalPointIdArrayName(this->GlobalPointIdArrayName);
    this->PickFilter->SetGlobalCellIdArrayName(this->GlobalCellIdArrayName);
    this->PickFilter->SetController(this->Controller);
    this->PickFilter->Update();
    output->ShallowCopy(this->PickFilter->GetOutput());
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkDataAnalysisFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PickCell: " << this->PickCell << endl;
  os << indent << "UseIdToPick: " << this->UseIdToPick << endl;
  os << indent << "Id: " << this->Id << endl;
  os << indent << "WorldPoint: " << this->WorldPoint[0]
    << "," << this->WorldPoint[1] << "," << this->WorldPoint[2] << endl;
  os << indent << "SpatialMatch: " << this->SpatialMatch << endl;
  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "GlobalPointIdArrayName: " << 
    (this->GlobalPointIdArrayName? this->GlobalPointIdArrayName: "None") 
    << endl;
  os << indent << "GlobalCellIdArrayName: " <<
    (this->GlobalCellIdArrayName? this->GlobalCellIdArrayName : "None")
    << endl;
}
