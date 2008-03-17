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

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"

vtkCxxRevisionMacro(vtkPVExtractSelection, "1.7");
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
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
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

  vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::GetData(outputVector, 0);
  vtkDataSet *geomOutput = vtkDataSet::GetData(outputVector, 0);

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

  // TODO: Fix to create multiple vtkSelections for different blocks.
  if (cdOutput)
    {
    output->SetContentType(vtkSelection::SELECTIONS);
    vtkCompositeDataIterator* iter = cdOutput->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
      {
      vtkSelection* curSel = this->LocateSelection(iter->GetCurrentFlatIndex(),
        sel);
      geomOutput = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (curSel && geomOutput)
        {
        vtkSelection* outputChild = this->RequestDataInternal(
          geomOutput, curSel);
        outputChild->GetProperties()->Set(vtkSelection::COMPOSITE_INDEX(),
          iter->GetCurrentFlatIndex());
        output->AddChild(outputChild);
        outputChild->Delete();
        }
      }
    iter->Delete();
    }
  else if (geomOutput)
    {
    vtkSelection* child = this->RequestDataInternal(
      geomOutput, sel);
    output->ShallowCopy(child);
    child->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkSelection* vtkPVExtractSelection::LocateSelection(unsigned int composite_index,
  vtkSelection* sel)
{
  if (sel->GetContentType() == vtkSelection::SELECTIONS)
    {
    unsigned int numChildren = sel->GetNumberOfChildren();
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* child = sel->GetChild(cc);
      if (child)
        {
        vtkSelection* outputChild = this->LocateSelection(composite_index, child);
        if (outputChild)
          {
          return outputChild;
          }
        }
      }
    return NULL;
    }

  if (sel->GetProperties()->Has(vtkSelection::COMPOSITE_INDEX()) &&
    sel->GetProperties()->Get(vtkSelection::COMPOSITE_INDEX()) == 
    static_cast<int>(composite_index))
    {
    return sel;
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkSelection* vtkPVExtractSelection::RequestDataInternal(
  vtkDataSet* geomOutput, vtkSelection* sel)
{
  vtkSelection* output = vtkSelection::New();
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
  vtkIdTypeArray *oids=0;
  if (geomOutput)
    {
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
    }
  if (oids)
    {
    output->SetSelectionList(oids);
    }

  return output;
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

