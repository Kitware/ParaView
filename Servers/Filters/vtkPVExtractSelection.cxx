/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractSelection.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"

vtkCxxRevisionMacro(vtkPVExtractSelection, "1.6");
vtkStandardNewMacro(vtkPVExtractSelection);

//----------------------------------------------------------------------------
vtkPVExtractSelection::vtkPVExtractSelection()
{
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkPVExtractSelection::~vtkPVExtractSelection()
{
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
    }
  else
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::RequestDataObject(
  vtkInformation* request,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestDataObject(request, inputVector, outputVector))
    {
    return 0;
    }

  vtkInformation* info = outputVector->GetInformationObject(1);
  vtkSelection *selOut = vtkSelection::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!selOut || !selOut->IsA("vtkSelection")) 
    {
    vtkDataObject* newOutput = 
      vtkDataObjectTypes::NewDataObject("vtkSelection");
    if (!newOutput)
      {
      vtkErrorMacro("Could not create vtkSelectionOutput");
      return 0;
      }
    newOutput->SetPipelineInformation(info);
    this->GetOutputPortInformation(1)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    newOutput->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }


  vtkSelection* sel = 0;
  if (inputVector[1]->GetInformationObject(0))
    {
    sel = vtkSelection::SafeDownCast(
      inputVector[1]->GetInformationObject(0)->Get(
        vtkDataObject::DATA_OBJECT()));
    }

  vtkDataSet *geomOutput = vtkDataSet::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));


  //make an ids selection for the second output
  //we can do this because all of the extractSelectedX filters produce 
  //arrays called "vtkOriginalXIds" that record what input cells produced 
  //each output cell, at least as long as PRESERVE_TOPOLOGY is off
  //when we start allowing PreserveTopology, this will have to instead run 
  //through the vtkInsidedNess arrays, and for every on entry, record the
  //entries index
  vtkSelection *output = vtkSelection::SafeDownCast(
    outputVector->GetInformationObject(1)->Get(vtkDataObject::DATA_OBJECT()));

  output->Clear();
  output->SetContentType(vtkSelection::INDICES);
  int ft = vtkSelection::CELL;
  if (sel && sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()))
    {
    ft = sel->GetProperties()->Get(vtkSelection::FIELD_TYPE());
    }
  output->GetProperties()->Set(vtkSelection::FIELD_TYPE(), ft);
  int inv = 0;
  if (sel && sel->GetProperties()->Has(vtkSelection::INVERSE()))
    {
    inv = sel->GetProperties()->Get(vtkSelection::INVERSE());
    }
  output->GetProperties()->Set(vtkSelection::INVERSE(), inv);
  vtkIdTypeArray *oids;
  if (ft == vtkSelection::CELL)
    {
    oids = vtkIdTypeArray::SafeDownCast(
      geomOutput->GetCellData()->GetArray("vtkOriginalCellIds"));
    }
  else
    {
    oids = vtkIdTypeArray::SafeDownCast(
      geomOutput->GetPointData()->GetArray("vtkOriginalPointIds"));
    }
  if (oids)
    {
    output->SetSelectionList(oids);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

