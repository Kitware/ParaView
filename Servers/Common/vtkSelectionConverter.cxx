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
#include "vtkSelectionSerializer.h"
#include "vtkUnsignedIntArray.h"

#include <vtkstd/map>
#include <vtkstd/set>

vtkStandardNewMacro(vtkSelectionConverter);
vtkCxxRevisionMacro(vtkSelectionConverter, "1.13");

//----------------------------------------------------------------------------
vtkSelectionConverter::vtkSelectionConverter()
{
}

//----------------------------------------------------------------------------
vtkSelectionConverter::~vtkSelectionConverter()
{
}

class vtkSelectionConverter::vtkKeyType
{
public:
  unsigned int HierarchicalIndex[2];
  unsigned int CompositeIndex;

  vtkKeyType()
    {
    this->CompositeIndex = 0;
    this->HierarchicalIndex[0]=0;
    this->HierarchicalIndex[1]=0;
    }
  vtkKeyType(unsigned int ci)
    {
    this->CompositeIndex = ci;
    this->HierarchicalIndex[0]=0;
    this->HierarchicalIndex[1]=0;
    }
  vtkKeyType(unsigned int level, unsigned int index)
    {
    this->CompositeIndex = 0;
    this->HierarchicalIndex[0] = level;
    this->HierarchicalIndex[1] = index;
    }
  bool operator < (const vtkKeyType& other) const
    {
    if (this->CompositeIndex == other.CompositeIndex)
      {
      if (this->HierarchicalIndex[0] == other.HierarchicalIndex[0])
        {
        return this->HierarchicalIndex[1] < other.HierarchicalIndex[1];
        }
      return this->HierarchicalIndex[0] < other.HierarchicalIndex[0];
      }
    return this->CompositeIndex < other.CompositeIndex;
    }
};

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
      if (newOutput->GetNumberOfChildren() > 0)
        {
        for (unsigned int cc=0; cc < newOutput->GetNumberOfChildren(); cc++)
          {
          output->AddChild(newOutput->GetChild(cc));
          }
        }
      else
        {
        output->AddChild(newOutput);
        }
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

  vtkUnsignedIntArray* compositeIndexArray = 0;
  vtkUnsignedIntArray* hierarchicalIndexArray = 0;

  // key == composite index or hierarchical index.
  // value == set of indices.
  // If compositeIndexArray is NULL, then all indicies are put under key==0.
  typedef vtkstd::map<vtkKeyType, vtkstd::set<vtkIdType> > indicesType;
  indicesType indices;

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

    compositeIndexArray = vtkUnsignedIntArray::SafeDownCast(
        ds->GetPointData()->GetArray("vtkCompositeIndex"));
    // compositeIndexArray may not be present at all if the input dataset is not a
    // composite dataset.
    
    hierarchicalIndexArray = vtkUnsignedIntArray::SafeDownCast(
      ds->GetPointData()->GetArray("vtkHierarchicalIndex")); // may be null.

    outputProperties->Set(vtkSelection::FIELD_TYPE(), vtkSelection::POINT);

    vtkIdList *idlist = vtkIdList::New();

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
          vtkIdType originalPtId = ptId;
          if (pointMapArray)
            {
            originalPtId = pointMapArray->GetValue(ptId);
            }

          vtkKeyType key;
          if (hierarchicalIndexArray)
            {
            unsigned int val[2];
            hierarchicalIndexArray->GetTupleValue(ptId, val);
            key = vtkKeyType(val[0], val[1]);
            }
          else if (compositeIndexArray)
            {
            unsigned int composite_index = 0;
            composite_index = compositeIndexArray->GetValue(ptId);
            key = vtkKeyType(composite_index);
            }

          vtkstd::set<vtkIdType>& visverts = indices[key];
          visverts.insert(originalPtId);
          }
        }
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

    compositeIndexArray = vtkUnsignedIntArray::SafeDownCast(
        ds->GetCellData()->GetArray("vtkCompositeIndex"));
    hierarchicalIndexArray = vtkUnsignedIntArray::SafeDownCast(
      ds->GetCellData()->GetArray("vtkHierarchicalIndex")); // may be null.

    for (vtkIdType hitId=0; hitId<numHits; hitId++)
      {
      vtkIdType geomCellId = inputList->GetValue(hitId);
      vtkIdType cellIndex = cellMapArray->GetValue(geomCellId);

      vtkKeyType key;
      if (hierarchicalIndexArray)
        {
        unsigned int val[2];
        hierarchicalIndexArray->GetTupleValue(geomCellId, val);
        key = vtkKeyType(val[0], val[1]);
        }
      else if (compositeIndexArray)
        {
        unsigned int composite_index = 
          compositeIndexArray->GetValue(geomCellId);
        key = vtkKeyType(composite_index);
        }
      vtkstd::set<vtkIdType>& cellindices = indices[key];
      if (global_ids)
        {
        vtkIdType globalId = globalIdsArray->GetValue(cellIndex);
        cellindices.insert(globalId);
        }
      else
        {
        cellindices.insert(cellIndex);
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

  if (indices.size() > 1)
    {
    indicesType::iterator mit;
    for (mit=indices.begin(); mit != indices.end(); ++mit)
      {
      vtkSelection* child = vtkSelection::New();
      child->GetProperties()->Copy(outputProperties, /*deep=*/0);
      if (hierarchicalIndexArray)
        {
        child->GetProperties()->Set(vtkSelection::HIERARCHICAL_LEVEL(),
          mit->first.HierarchicalIndex[0]);
        child->GetProperties()->Set(vtkSelection::HIERARCHICAL_INDEX(),
          mit->first.HierarchicalIndex[1]);
        }
      else if (compositeIndexArray)
        {
        child->GetProperties()->Set(vtkSelection::COMPOSITE_INDEX(),
          mit->first.CompositeIndex);
        }

      vtkIdTypeArray* outputArray = vtkIdTypeArray::New();
      vtkstd::set<vtkIdType> &ids = mit->second;
      vtkstd::set<vtkIdType>::iterator sit;
      outputArray->SetNumberOfTuples(ids.size());
      vtkIdType index=0;
      for (sit = ids.begin(); sit != ids.end(); sit++, index++)
        {
        outputArray->SetValue(index, *sit); 
        }
      child->SetSelectionList(outputArray);
      outputArray->Delete();
  
      output->AddChild(child);
      child->Delete();
      }
    }
  else if (indices.size() == 1)
    {
    vtkIdTypeArray* outputArray = vtkIdTypeArray::New();
    vtkstd::set<vtkIdType> &ids = indices.begin()->second;
    vtkstd::set<vtkIdType>::iterator sit;
    outputArray->SetNumberOfTuples(ids.size());
    vtkIdType index=0;
    for (sit = ids.begin(); sit != ids.end(); sit++, index++)
      {
      outputArray->SetValue(index, *sit); 
      }
    if (hierarchicalIndexArray)
      {
      outputProperties->Set(vtkSelection::HIERARCHICAL_LEVEL(),
        indices.begin()->first.HierarchicalIndex[0]);
      outputProperties->Set(vtkSelection::HIERARCHICAL_INDEX(),
        indices.begin()->first.HierarchicalIndex[1]);
      }
    else if (compositeIndexArray)
      {
      outputProperties->Set(vtkSelection::COMPOSITE_INDEX(),
        indices.begin()->first.CompositeIndex);
      }
    output->SetSelectionList(outputArray);
    outputArray->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

