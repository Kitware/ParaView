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
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkIdList.h"
#include "vtkstd/set"

vtkStandardNewMacro(vtkSelectionConverter);
vtkCxxRevisionMacro(vtkSelectionConverter, "1.12");

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
    vtkInformation *childProps = input->GetChild(i)->GetProperties();
    if (!childProps->Has(vtkSelection::PROCESS_ID()) ||
        ( childProps->Get(vtkSelection::PROCESS_ID()) ==
          vtkProcessModule::GetProcessModule()->GetPartitionId() )
      )
      {
      vtkSelection* newOutput = vtkSelection::New();
      this->Convert(input->GetChild(i), newOutput, global_ids);
      output->AddChild(newOutput);
      newOutput->Delete();
      }
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

  vtkIdTypeArray* cellMapArray = vtkIdTypeArray::SafeDownCast(
    ds->GetCellData()->GetArray("vtkOriginalCellIds"));
  if (!cellMapArray)
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

  vtkIdType numHits = inputList->GetNumberOfTuples() *
    inputList->GetNumberOfComponents();

  vtkIdTypeArray* vertptrs = vtkIdTypeArray::SafeDownCast(
    input->GetSelectionData()->GetArray("vertptrs"));
  vtkIdTypeArray* vertlist = vtkIdTypeArray::SafeDownCast(
    input->GetSelectionData()->GetArray("vertlist"));
  if (inputProperties->Has(vtkSelection::INDEXED_VERTICES())
      &&
      (inputProperties->Get(vtkSelection::INDEXED_VERTICES()) == 1)
      &&
      vertptrs
      &&
      vertlist
    )
    {    

    vtkIdTypeArray* pointMapArray = vtkIdTypeArray::SafeDownCast(
      ds->GetPointData()->GetArray("vtkOriginalPointIds"));
    if (!pointMapArray)
      {
      return;
      }

    outputProperties->Set(vtkSelection::FIELD_TYPE(),vtkSelection::POINT);

    vtkIdList *idlist = vtkIdList::New();
    vtkstd::set<vtkIdType> visverts;

    //lookup each hit cell in the polygonal shell, and find those of its
    //vertices which were hit. For those lookup vertex id in original data set.
    for (vtkIdType hitId=0; hitId<numHits; hitId++)
      {
      vtkIdType ptr = vertptrs->GetValue(hitId);
      if (ptr != -1)
        {
        vtkIdType cellIndex = inputList->GetValue(hitId);
        ds->GetCellPoints(cellIndex, idlist);

        vtkIdType npts = vertlist->GetValue(ptr);
        for (vtkIdType v = 0; v < npts; v++)
          {
          vtkIdType idx = vertlist->GetValue(ptr+1+v);
          vtkIdType ptId = idlist->GetId(idx); 
          if (pointMapArray)
            {
            ptId = pointMapArray->GetValue(ptId);
            }
          visverts.insert(ptId);
          }
        }
      }

    //this set is just to eliminate duplicates
    vtkstd::set<vtkIdType>::iterator sit;
    for (sit = visverts.begin(); sit != visverts.end(); sit++)
      {
      outputArray->InsertNextValue(*sit);      
      }

    idlist->Delete();
    }
  else 
    {
    outputArray->SetNumberOfTuples(numHits);

    for (vtkIdType hitId=0; hitId<numHits; hitId++)
      {
      vtkIdType cellIndex = cellMapArray->GetValue(inputList->GetValue(hitId));
      if (global_ids)
        {
        vtkIdType globalId = globalIdsArray->GetValue(cellIndex);
        outputArray->SetValue(hitId, globalId);
        }
      else
        {
        outputArray->SetValue(hitId, cellIndex);
        }
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

