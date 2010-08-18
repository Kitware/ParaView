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
void vtkSelectionConverter::Convert(
  vtkSelectionNode* inputNode, vtkSelection* output, int global_ids)
{
  if (global_ids)
    {
    // At somepoint in future for effeciency reasons, we may want to bring back
    // the support for converting to global ids
    vtkErrorMacro("Global id selection no longer supported.");
    return;
    }

  vtkInformation* inputProperties =  inputNode->GetProperties();
  if (inputProperties->Get(vtkSelectionNode::CONTENT_TYPE()) !=
    vtkSelectionNode::INDICES || !inputProperties->Has(vtkSelectionNode::FIELD_TYPE()))
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
    inputNode->GetSelectionList());
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
  bool is_amr = false;
  int amr_index[2] = {0, 0};
  if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    assert(inputProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()));
    ds = this->LocateDataSet(iter,
        inputProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()));
    if (ds && iter->HasCurrentMetaData())
      {
      vtkInformation* metadata = iter->GetCurrentMetaData();
      if (metadata->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
        metadata->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()))
        {
        is_amr = true;
        amr_index[0] = metadata->Get(vtkSelectionNode::HIERARCHICAL_LEVEL());
        amr_index[1] = metadata->Get(vtkSelectionNode::HIERARCHICAL_INDEX());
        }
      }
    iter->Delete();
    }
  if (!ds)
    {
    return;
    }

  vtkIdType numHits = inputList->GetNumberOfTuples() *
    inputList->GetNumberOfComponents();

  vtkIdTypeArray* originalIdsArray = NULL;
  if (inputProperties->Get(vtkSelectionNode::FIELD_TYPE()) == vtkSelectionNode::POINT)
    {
    originalIdsArray = vtkIdTypeArray::SafeDownCast(
      ds->GetPointData()->GetArray("vtkOriginalPointIds"));
    }
  else
    {
    originalIdsArray = vtkIdTypeArray::SafeDownCast(
      ds->GetCellData()->GetArray("vtkOriginalCellIds"));
    }

  if (!originalIdsArray)
    {
    vtkErrorMacro("Could not locate the original ids arrays. Did the "
      "geometry not produce them?");
    return;
    }

  vtkSelectionNode* outputNode = vtkSelectionNode::New();
  vtkInformation* outputProperties = outputNode->GetProperties();
  if (global_ids)
    {
    // this is not applicable anymore, but leaving it here as a guide in future
    // if we need to bring back the conversion to global-id support.
    //outputProperties->Set( vtkSelectionNode::CONTENT_TYPE(),
    //  vtkSelectionNode::GLOBALIDS);
    }
  else
    {
    outputProperties->Set(vtkSelectionNode::CONTENT_TYPE(),
      inputProperties->Get(vtkSelectionNode::CONTENT_TYPE()));
    }

  outputProperties->Set(vtkSelectionNode::FIELD_TYPE(),
    inputProperties->Get(vtkSelectionNode::FIELD_TYPE()));

  typedef vtkstd::set<vtkIdType> indicesType;
  indicesType indices;

  for (vtkIdType hitId=0; hitId<numHits; hitId++)
    {
    vtkIdType element_id = inputList->GetValue(hitId);
    if (element_id >= 0 && element_id < originalIdsArray->GetNumberOfTuples())
      {
      vtkIdType original_element_id= originalIdsArray->GetValue(element_id);
      indices.insert(original_element_id);
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

  if (is_amr)
    {
    outputProperties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(),
      amr_index[0]);
    outputProperties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(),
      amr_index[1]);
    }
  else if (inputProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()))
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
    outputNode->SetSelectionList(outputArray);
    outputArray->FastDelete();
    output->AddNode(outputNode);
    outputNode->FastDelete();
    }
  else
    {
    outputNode->Delete();
    }
}


//----------------------------------------------------------------------------
vtkDataSet* vtkSelectionConverter::LocateDataSet(
  vtkCompositeDataIterator* iter,  unsigned int index)
{
  // FIXME: this needs to be optimized.
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

