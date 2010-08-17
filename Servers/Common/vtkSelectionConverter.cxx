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
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedIntArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <assert.h>

vtkStandardNewMacro(vtkSelectionConverter);

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
  output->Initialize();
  for (unsigned int i = 0; i < input->GetNumberOfNodes(); ++i)
    {
    vtkInformation *nodeProps = input->GetNode(i)->GetProperties();
    if (!nodeProps->Has(vtkSelectionNode::PROCESS_ID()) ||
        ( nodeProps->Get(vtkSelectionNode::PROCESS_ID()) ==
          vtkProcessModule::GetProcessModule()->GetPartitionId() )
      )
      {
      this->Convert(input->GetNode(i), output, global_ids);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::Convert(vtkSelectionNode* input, vtkSelection* outputSel,
  int global_ids)
{
  vtkSmartPointer<vtkSelectionNode> output = vtkSmartPointer<vtkSelectionNode>::New();
  vtkInformation* inputProperties =  input->GetProperties();
  vtkInformation* outputProperties = output->GetProperties();

  if (global_ids)
    {
    outputProperties->Set(
      vtkSelectionNode::CONTENT_TYPE(),
      vtkSelectionNode::GLOBALIDS);
    }
  else
    {
    outputProperties->Set(
      vtkSelectionNode::CONTENT_TYPE(),
      inputProperties->Get(vtkSelectionNode::CONTENT_TYPE()));
    }

  if (inputProperties->Get(vtkSelectionNode::CONTENT_TYPE()) !=
    vtkSelectionNode::INDICES ||
    !inputProperties->Has(vtkSelectionNode::FIELD_TYPE()))
    {
    return;
    }

  if (inputProperties->Get(vtkSelectionNode::FIELD_TYPE()) != vtkSelectionNode::CELL &&
    inputProperties->Get(vtkSelectionNode::FIELD_TYPE()) != vtkSelectionNode::POINT)
    {
    // We can only handle point or cell selections.
    return;
    }

  if (!inputProperties->Has(vtkSelectionNode::SOURCE_ID()) ||
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
  id.ID = inputProperties->Get(vtkSelectionNode::SOURCE_ID());
  vtkAlgorithm* geomAlg = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(id));
  if (!geomAlg)
    {
    return;
    }

  vtkDataObject* geometryFilterOutput = geomAlg->GetOutputDataObject(0);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(geometryFilterOutput);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(geometryFilterOutput);
  if (cd)
    {
    assert(inputProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()));
    ds = this->LocateDataSet(cd,
        inputProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()));
    }
  if (!ds)
    {
    return;
    }

  if (global_ids)
    {
    vtkErrorMacro("Global id selection no longer supported.");
    return;
    }

  typedef vtkstd::set<vtkIdType> indicesType;
  indicesType indices;

  vtkIdType numHits = inputList->GetNumberOfTuples() *
    inputList->GetNumberOfComponents();

  if (inputProperties->Get(vtkSelectionNode::FIELD_TYPE()) == vtkSelectionNode::POINT)
    {
    vtkIdTypeArray* pointMapArray = vtkIdTypeArray::SafeDownCast(
      ds->GetPointData()->GetArray("vtkOriginalPointIds"));
    if (!pointMapArray)
      {
      return;
      }

    outputProperties->Set(vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::POINT);

    vtkIdList *idlist = vtkIdList::New();

    //lookup each hit cell in the polygonal shell, and find those of its
    //vertices which were hit. For those lookup vertex id in original data set.
    for (vtkIdType hitId=0; hitId<numHits; hitId++)
      {
      vtkIdType geomPointId = inputList->GetValue(hitId);
      if (geomPointId < 0 || geomPointId >=  pointMapArray->GetNumberOfTuples())
        {
        continue;
        }
      vtkIdType pointId = pointMapArray->GetValue(geomPointId);
      indices.insert(pointId);
      }
    idlist->Delete();
    }
  else
    {
    vtkIdTypeArray* cellMapArray = vtkIdTypeArray::SafeDownCast(
      ds->GetCellData()->GetArray("vtkOriginalCellIds"));
    if (!cellMapArray)
      {
      return;
      }

    for (vtkIdType hitId=0; hitId<numHits; hitId++)
      {
      vtkIdType geomCellId = inputList->GetValue(hitId);
      vtkIdType cellIndex = cellMapArray->GetValue(geomCellId);
      indices.insert(cellIndex);
      }
    }

  outputProperties->Set(
    vtkSelectionNode::SOURCE_ID(),
    inputProperties->Get(vtkSelectionSerializer::ORIGINAL_SOURCE_ID()));

  if (inputProperties->Has(vtkSelectionNode::PROCESS_ID()))
    {
    outputProperties->Set(vtkSelectionNode::PROCESS_ID(),
                          inputProperties->Get(vtkSelectionNode::PROCESS_ID()));
    }

  if (inputProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
    {
    outputProperties->Set(vtkSelectionNode::COMPOSITE_INDEX(),
      inputProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()));
    }

  if (indices.size() > 0)
    {
    vtkIdTypeArray* outputArray = vtkIdTypeArray::New();
    vtkstd::set<vtkIdType>::iterator sit;
    outputArray->SetNumberOfTuples(indices.size());
    vtkIdType* out_ptr = outputArray->GetPointer(0);
    vtkIdType index=0;
    for (sit = indices.begin(); sit != indices.end(); sit++, index++)
      {
      out_ptr[index] = *sit;
      }
    output->SetSelectionList(outputArray);
    outputArray->FastDelete();
    outputSel->AddNode(output);
    //outputSel->Print(cout);
    }
}


//----------------------------------------------------------------------------
vtkDataSet* vtkSelectionConverter::LocateDataSet(
  vtkCompositeDataSet* cd, unsigned int index)
{
  // FIXME: this needs to be optimized.
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(cd->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    if (iter->GetCurrentFlatIndex() < index)
      {
      continue;
      }
    if (iter->GetCurrentFlatIndex() == index)
      {
      return vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      }
    break;
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

