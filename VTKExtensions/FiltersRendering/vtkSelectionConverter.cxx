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
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedIntArray.h"

#include <assert.h>
#include <map>
#include <set>

namespace
{
// vtkPVGeometryFilter merges pieces in a vtkMultiPieceDataSet together. This
// code helps us identify the real input composite-id given the element id in
// the merged output.
static unsigned int GetCellPieceOffset(vtkInformation* pieceMetaData, vtkIdType element_id)
{
  if (pieceMetaData)
  {
    vtkInformationIntegerVectorKey* keys[] = { vtkPVGeometryFilter::VERTS_OFFSETS(),
      vtkPVGeometryFilter::LINES_OFFSETS(), vtkPVGeometryFilter::POLYS_OFFSETS(),
      vtkPVGeometryFilter::STRIPS_OFFSETS() };
    for (int kk = 0; kk < 4; kk++)
    {
      int num_values = pieceMetaData->Length(keys[kk]);
      int* values = num_values > 0 ? pieceMetaData->Get(keys[kk]) : nullptr;
      for (int cc = 1; cc < num_values; cc++)
      {
        if (element_id >= values[cc - 1] && element_id < values[cc])
        {
          return cc - 1;
        }
      }
    }
  }
  return 0;
}

static unsigned int GetPointPieceOffset(vtkInformation* pieceMetaData, vtkIdType element_id)
{
  if (pieceMetaData)
  {
    int num_values = pieceMetaData->Length(vtkPVGeometryFilter::POINT_OFFSETS());
    int* values =
      num_values > 0 ? pieceMetaData->Get(vtkPVGeometryFilter::POINT_OFFSETS()) : nullptr;
    for (int cc = 1; cc < num_values; cc++)
    {
      if (element_id >= values[cc - 1] && element_id <= values[cc])
      {
        return cc - 1;
      }
    }
  }
  return 0;
}
};

vtkStandardNewMacro(vtkSelectionConverter);

//----------------------------------------------------------------------------
vtkSelectionConverter::vtkSelectionConverter() = default;

//----------------------------------------------------------------------------
vtkSelectionConverter::~vtkSelectionConverter() = default;

//----------------------------------------------------------------------------
void vtkSelectionConverter::Convert(vtkSelection* input, vtkSelection* output, int global_ids)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  output->Initialize();
  for (unsigned int i = 0; i < input->GetNumberOfNodes(); ++i)
  {
    vtkInformation* nodeProps = input->GetNode(i)->GetProperties();
    if (!nodeProps->Has(vtkSelectionNode::PROCESS_ID()) ||
      (nodeProps->Get(vtkSelectionNode::PROCESS_ID()) == controller->GetLocalProcessId()))
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
    // At somepoint in future for efficiency reasons, we may want to bring back
    // the support for converting to global ids
    vtkErrorMacro("Global id selection no longer supported.");
    return;
  }

  vtkInformation* inputProperties = inputNode->GetProperties();
  if (inputProperties->Get(vtkSelectionNode::CONTENT_TYPE()) != vtkSelectionNode::INDICES ||
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

  if (!inputProperties->Has(vtkSelectionNode::SOURCE()))
  {
    return;
  }

  vtkIdTypeArray* inputList = vtkIdTypeArray::SafeDownCast(inputNode->GetSelectionList());
  if (!inputList)
  {
    return;
  }

  vtkAlgorithm* geomAlg =
    vtkAlgorithm::SafeDownCast(inputProperties->Get(vtkSelectionNode::SOURCE()));
  if (!geomAlg)
  {
    return;
  }

  vtkDataObject* geometryFilterOutput = geomAlg->GetOutputDataObject(0);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(geometryFilterOutput);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(geometryFilterOutput);
  vtkInformation* pieceMetaData = nullptr;
  bool is_amr = false;
  int amr_index[2] = { 0, 0 };
  if (cd)
  {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    assert(inputProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()));
    ds = this->LocateDataSet(iter, inputProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()));
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
      if (metadata->Has(vtkPVGeometryFilter::POINT_OFFSETS()))
      {
        // vtkPVGeometryFilter combines pieces ina multi-piece together. When it
        // does that, it puts meta-data in the collapsed need to aid in
        // recovering the original piece.
        pieceMetaData = metadata;
      }
    }
    iter->Delete();
  }
  if (!ds)
  {
    return;
  }

  vtkIdType numHits = inputList->GetNumberOfTuples() * inputList->GetNumberOfComponents();

  bool using_cell = false;
  vtkIdTypeArray* originalIdsArray = nullptr;
  if (inputProperties->Get(vtkSelectionNode::FIELD_TYPE()) == vtkSelectionNode::POINT)
  {
    originalIdsArray =
      vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetArray("vtkOriginalPointIds"));
  }
  else
  {
    originalIdsArray =
      vtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetArray("vtkOriginalCellIds"));
    using_cell = true;
  }

  if (!originalIdsArray)
  {
    vtkErrorMacro("Could not locate the original ids arrays. Did the "
                  "geometry not produce them?");
    return;
  }

  // key==piece offset, while value==list of ids for that piece.
  typedef std::map<int, std::set<vtkIdType> > indicesType;
  indicesType indices;

  for (vtkIdType hitId = 0; hitId < numHits; hitId++)
  {
    vtkIdType element_id = inputList->GetValue(hitId);
    if (element_id >= 0 && element_id < originalIdsArray->GetNumberOfTuples())
    {
      vtkIdType original_element_id = originalIdsArray->GetValue(element_id);
      int piece_offset = using_cell ? GetCellPieceOffset(pieceMetaData, element_id)
                                    : GetPointPieceOffset(pieceMetaData, element_id);
      indices[piece_offset].insert(original_element_id);
    }
  }

  if (indices.size() == 0)
  {
    // nothing was selected.
    return;
  }

  indicesType::iterator mapIter;
  for (mapIter = indices.begin(); mapIter != indices.end(); ++mapIter)
  {
    int piece_offset = mapIter->first;
    vtkSelectionNode* outputNode = vtkSelectionNode::New();
    vtkInformation* outputProperties = outputNode->GetProperties();
    if (global_ids)
    {
      // this is not applicable anymore, but leaving it here as a guide in future
      // if we need to bring back the conversion to global-id support.
      // outputProperties->Set( vtkSelectionNode::CONTENT_TYPE(),
      //  vtkSelectionNode::GLOBALIDS);
    }
    else
    {
      outputProperties->Set(
        vtkSelectionNode::CONTENT_TYPE(), inputProperties->Get(vtkSelectionNode::CONTENT_TYPE()));
    }

    outputProperties->Set(
      vtkSelectionNode::FIELD_TYPE(), inputProperties->Get(vtkSelectionNode::FIELD_TYPE()));

    outputProperties->Set(vtkSelectionNode::SOURCE_ID(),
      inputProperties->Get(vtkSelectionSerializer::ORIGINAL_SOURCE_ID()));

    if (inputProperties->Has(vtkSelectionNode::PIXEL_COUNT()))
    {
      outputProperties->Set(
        vtkSelectionNode::PIXEL_COUNT(), inputProperties->Get(vtkSelectionNode::PIXEL_COUNT()));
    }

    if (inputProperties->Has(vtkSelectionNode::PROCESS_ID()))
    {
      outputProperties->Set(
        vtkSelectionNode::PROCESS_ID(), inputProperties->Get(vtkSelectionNode::PROCESS_ID()));
    }

    if (is_amr)
    {
      outputProperties->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), amr_index[0]);
      outputProperties->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), amr_index[1] + piece_offset);
    }
    else if (inputProperties->Has(vtkSelectionNode::COMPOSITE_INDEX()) && cd)
    {
      outputProperties->Set(vtkSelectionNode::COMPOSITE_INDEX(),
        inputProperties->Get(vtkSelectionNode::COMPOSITE_INDEX()) + piece_offset);
    }

    vtkIdTypeArray* outputArray = vtkIdTypeArray::New();
    std::set<vtkIdType>::iterator sit;
    outputArray->SetNumberOfTuples(mapIter->second.size());
    vtkIdType* out_ptr = outputArray->GetPointer(0);
    vtkIdType index = 0;
    for (sit = mapIter->second.begin(); sit != mapIter->second.end(); sit++)
    {
      out_ptr[index] = *sit;
      index++;
    }
    outputNode->SetSelectionList(outputArray);
    outputArray->FastDelete();
    output->AddNode(outputNode);
    outputNode->FastDelete();
  }
}

//----------------------------------------------------------------------------
vtkDataSet* vtkSelectionConverter::LocateDataSet(vtkCompositeDataIterator* iter, unsigned int index)
{
  // FIXME: this needs to be optimized.
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
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

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSelectionConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
