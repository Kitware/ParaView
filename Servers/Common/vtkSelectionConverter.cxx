/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionConverter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionConverter.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"

vtkStandardNewMacro(vtkSelectionConverter);
vtkCxxRevisionMacro(vtkSelectionConverter, "1.9");

//----------------------------------------------------------------------------
vtkSelectionConverter::vtkSelectionConverter()
{
}

//----------------------------------------------------------------------------
vtkSelectionConverter::~vtkSelectionConverter()
{
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::Convert(vtkSelection* input, vtkSelection* output,
  int global_ids)
{
  output->Clear();

  vtkInformation* inputProperties =  input->GetProperties();
  vtkInformation* outputProperties = output->GetProperties();
  
  if (global_ids)
    {
    outputProperties->Set(
      vtkSelection::CONTENT_TYPE(),
      vtkSelection::GLOBALIDS);
    }
  else
    {
    outputProperties->Set(
      vtkSelection::CONTENT_TYPE(),
      inputProperties->Get(vtkSelection::CONTENT_TYPE()));
    }


  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelection* newOutput = vtkSelection::New();
    this->Convert(input->GetChild(i), newOutput, global_ids);
    output->AddChild(newOutput);
    newOutput->Delete();
    }

  if (inputProperties->Get(vtkSelection::CONTENT_TYPE()) !=
      vtkSelection::INDICES
      ||
      !inputProperties->Has(vtkSelection::FIELD_TYPE())
      ||
      inputProperties->Get(vtkSelection::FIELD_TYPE()) !=
      vtkSelection::CELL)
    {
    return;
    }
      
  if (!inputProperties->Has(vtkSelection::SOURCE_ID()) ||
      !inputProperties->Has(vtkSelectionSerializer::ORIGINAL_SOURCE_ID()))
    {
    return;
    }

  vtkIdTypeArray* inputList = vtkIdTypeArray::SafeDownCast(
    input->GetSelectionList());
  if (!inputList)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerID id;
  id.ID = inputProperties->Get(vtkSelection::SOURCE_ID());
  vtkAlgorithm* geomAlg = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(id));
  if (!geomAlg)
    {
    return;
    }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(
    geomAlg->GetOutputDataObject(0));
  if (!ds)
    {
    return;
    }

  vtkIdTypeArray* mapArray = vtkIdTypeArray::SafeDownCast(
    ds->GetCellData()->GetArray("vtkOriginalCellIds"));
  if (!mapArray)
    {
    return;
    }

  /// Get the dataset from the input of the geom filter from which
  /// we get the global ids, if required.
  id.ID = inputProperties->Get(vtkSelectionSerializer::ORIGINAL_SOURCE_ID());
  vtkAlgorithm* originalAlg = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(id));
  vtkDataSet* originalDS = originalAlg? 
    vtkDataSet::SafeDownCast(originalAlg->GetOutputDataObject(0)) : 0;
  vtkIdTypeArray* globalIdsArray = originalDS?
    vtkIdTypeArray::SafeDownCast(originalDS->GetCellData()->GetGlobalIds()): 0;

  if (global_ids && !globalIdsArray)
    {
    return;
    }


  vtkIdTypeArray* outputArray = vtkIdTypeArray::New();

  vtkIdType numCells = inputList->GetNumberOfTuples() *
    inputList->GetNumberOfComponents();
  outputArray->SetNumberOfTuples(numCells);

  for (vtkIdType cellId=0; cellId<numCells; cellId++)
    {
    vtkIdType index = mapArray->GetValue(inputList->GetValue(cellId));
    if (global_ids)
      {
      vtkIdType globalId = globalIdsArray->GetValue(index);
      outputArray->SetValue(cellId, globalId);
      }
    else
      {
      outputArray->SetValue(cellId, index);
      }
    }

  outputProperties->Set(
    vtkSelection::SOURCE_ID(),
    inputProperties->Get(vtkSelectionSerializer::ORIGINAL_SOURCE_ID()));
  
  if (inputProperties->Has(vtkSelection::PROCESS_ID()))
    {
    outputProperties->Set(vtkSelection::PROCESS_ID(),
                          inputProperties->Get(vtkSelection::PROCESS_ID()));
    }
  
  output->SetSelectionList(outputArray);
  outputArray->Delete();
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

