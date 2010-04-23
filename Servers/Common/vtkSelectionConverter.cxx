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

#include <vtkstd/map>
#include <vtkstd/set>

vtkStandardNewMacro(vtkSelectionConverter);

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

#if 0
  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkInformation *childProps = input->GetChild(i)->GetProperties();
    if (!childProps->Has(vtkSelectionNode::PROCESS_ID()) ||
        ( childProps->Get(vtkSelectionNode::PROCESS_ID()) ==
          vtkProcessModule::GetProcessModule()->GetPartitionId() )
      )
      {
      vtkSelection* newOutput = vtkSelectionNode::New();
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
#endif

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

  vtkDataSet* ds = vtkDataSet::SafeDownCast(
    geomAlg->GetOutputDataObject(0));
  if (!ds)
    {
    return;
    }

  if (global_ids)
    {
    vtkErrorMacro("Global id selection no longer supported.");
    return;
    }

  // NOTE: these are cell-data arrays.
  vtkUnsignedIntArray* compositeIndexArray = vtkUnsignedIntArray::SafeDownCast(
    ds->GetCellData()->GetArray("vtkCompositeIndex"));
  // compositeIndexArray may not be present at all if the input dataset is not a
  // composite dataset.
  vtkUnsignedIntArray* amrLevelArray = vtkUnsignedIntArray::SafeDownCast(
    ds->GetCellData()->GetArray("vtkAMRLevel")); // may be null.
  vtkUnsignedIntArray* amrIndexArray = vtkUnsignedIntArray::SafeDownCast(
    ds->GetCellData()->GetArray("vtkAMRIndex")); // may be null.

  // key == composite index or hierarchical index.
  // value == set of indices.
  // If compositeIndexArray is NULL, then all indicies are put under key==0.
  typedef vtkstd::map<vtkKeyType, vtkstd::set<vtkIdType> > indicesType;
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

      // Composite index information is available only on cells, hence, 
      // for each point we find the first cell it belongs to and then lookup the
      // composite index.
      ds->GetPointCells(geomPointId, idlist);
      vtkIdType geomCellId = 0;
      if (idlist->GetNumberOfIds() > 0)
        {
        geomCellId = idlist->GetId(0);
        }
      vtkKeyType key;
      if (amrLevelArray && amrIndexArray)
        {
        unsigned int val[2];
        amrLevelArray->GetTupleValue(geomCellId, &val[0]);
        amrIndexArray->GetTupleValue(geomCellId, &val[1]);
        key = vtkKeyType(val[0], val[1]);
        }
      else if (compositeIndexArray)
        {
        unsigned int composite_index = 0;
        composite_index = compositeIndexArray->GetValue(geomCellId);
        key = vtkKeyType(composite_index);
        }
      vtkstd::set<vtkIdType>& visverts = indices[key];
      visverts.insert(pointId);
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

      vtkKeyType key;
      if (amrLevelArray && amrIndexArray)
        {
        unsigned int val[2];
        amrLevelArray->GetTupleValue(geomCellId, &val[0]);
        amrIndexArray->GetTupleValue(geomCellId, &val[1]);
        key = vtkKeyType(val[0], val[1]);
        }
      else if (compositeIndexArray)
        {
        unsigned int composite_index =
          compositeIndexArray->GetValue(geomCellId);
        key = vtkKeyType(composite_index);
        }
      vtkstd::set<vtkIdType>& cellindices = indices[key];
      cellindices.insert(cellIndex);
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

  if (indices.size() > 1)
    {
    indicesType::iterator mit;
    for (mit=indices.begin(); mit != indices.end(); ++mit)
      {
      vtkSelectionNode* child = vtkSelectionNode::New();
      child->GetProperties()->Copy(outputProperties, /*deep=*/0);
      if (amrLevelArray && amrIndexArray)
        {
        child->GetProperties()->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(),
          mit->first.HierarchicalIndex[0]);
        child->GetProperties()->Set(vtkSelectionNode::HIERARCHICAL_INDEX(),
          mit->first.HierarchicalIndex[1]);
        }
      else if (compositeIndexArray)
        {
        child->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(),
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

      outputSel->AddNode(child);
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
    if (amrLevelArray && amrIndexArray)
      {
      outputProperties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(),
        indices.begin()->first.HierarchicalIndex[0]);
      outputProperties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(),
        indices.begin()->first.HierarchicalIndex[1]);
      }
    else if (compositeIndexArray)
      {
      outputProperties->Set(vtkSelectionNode::COMPOSITE_INDEX(),
        indices.begin()->first.CompositeIndex);
      }
    output->SetSelectionList(outputArray);
    outputArray->Delete();
    outputSel->AddNode(output);
    }
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

