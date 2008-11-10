/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionStreamer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionStreamer.h"

#include "vtkObjectFactory.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSelection.h"
#include "vtkInformation.h"
#include "vtkCompositeDataIterator.h"
#include "vtkUnsignedIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiProcessController.h"

vtkStandardNewMacro(vtkSelectionStreamer);
vtkCxxRevisionMacro(vtkSelectionStreamer, "1.1");
//----------------------------------------------------------------------------
vtkSelectionStreamer::vtkSelectionStreamer()
{
  // port 0 -- vtkSelection
  // port 1 -- vtkDataObject used to detemine what ids constitute a block.
  this->SetNumberOfInputPorts(2);
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
}

//----------------------------------------------------------------------------
vtkSelectionStreamer::~vtkSelectionStreamer()
{
}

//----------------------------------------------------------------------------
int vtkSelectionStreamer::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
    {
    return this->Superclass::FillInputPortInformation(port, info);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectionStreamer::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectionStreamer::RequestData(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkSelection* inputSel = vtkSelection::GetData(inputVector[0], 0);
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[1], 0);
  vtkSelection* output = vtkSelection::GetData(outputVector, 0);

  vtkstd::vector<vtkstd::pair<vtkIdType, vtkIdType> > indices;
  if (!this->DetermineIndicesToPass(inputDO, indices))
    {
    return 0;
    }

  if (!inputDO->IsA("vtkCompositeDataSet"))
    {
    vtkSelection* inSel = this->LocateSelection(inputSel);
    if (inSel)
      {
      this->PassBlock(output, inSel,
        indices[0].first, indices[0].second);
      }
    return 1;
    }

  int myId = this->Controller? this->Controller->GetLocalProcessId()  :0;
  output->GetProperties()->Set(vtkSelection::PROCESS_ID(), myId);

  vtkSmartPointer<vtkCompositeDataSet> input =
    vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkstd::vector<vtkSmartPointer<vtkSelection> > output_selections;

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->SkipEmptyNodesOff();
  int cc=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem(), cc++)
    {
    vtkIdType curOffset = indices[cc].first;
    vtkIdType curCount = indices[cc].second;
    if (curCount > 0)
      {
      vtkSelection* curSel = this->LocateSelection(iter, inputSel);
      if (!curSel)
        {
        continue;
        }
      vtkSelection* curOutputSel = vtkSelection::New();
      curOutputSel->GetProperties()->Copy(curSel->GetProperties());
      curOutputSel->GetProperties()->Set(vtkSelection::PROCESS_ID(), myId);
      bool hit = false;
      if (curSel->GetContentType() == vtkSelection::BLOCKS)
        {
        // BLOCK selection, pass if the current block is selected.
        if (curSel->GetSelectionList()->LookupValue(
            vtkVariant(iter->GetCurrentFlatIndex())) != -1)
          {
          vtkUnsignedIntArray* selList = vtkUnsignedIntArray::New();
          selList->SetNumberOfTuples(1);
          selList->SetValue(0, iter->GetCurrentFlatIndex());
          curOutputSel->SetSelectionList(selList);
          selList->Delete();
          hit = true;
          }
        }
      else 
        {
        hit |= this->PassBlock(curOutputSel, curSel, curOffset, curCount);
        }
      if (hit)
        {
        output_selections.push_back(curOutputSel);
        }
      curOutputSel->Delete();
      }
    }
  iter->Delete();

  if (output_selections.size() == 1)
    {
    output->ShallowCopy(output_selections[0]);
    }
  else if (output_selections.size() > 1)
    {
    output->SetContentType(vtkSelection::SELECTIONS);
    for (unsigned int cc=0; cc < output_selections.size(); cc++)
      {
      output->AddChild(output_selections[cc]);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelectionStreamer::LocateSelection(
  vtkCompositeDataIterator* inputIter, vtkSelection* sel)
{
  if (!sel || !inputIter || !inputIter->HasCurrentMetaData())
    {
    return 0;
    }

  // input is a composite dataset (a composite of vtkTable elements).
  // It is possible that the selection identifies leaves in the composite
  // dataset using CompositeIndex or using the HierarchicalLevel/Index (if the
  // original input before converting to tables was a
  // vtkHierarchicalBoxDataSet). 
 
  if (sel->GetContentType() == vtkSelection::SELECTIONS)
    {
    unsigned int numChildren = sel->GetNumberOfChildren();
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* located = this->LocateSelection(inputIter, sel->GetChild(cc));
      if (located)
        {
        return located;
        }
      }
    return 0;
    }

  // vtkAttributeDataToTableFilter puts in this meta-data to which aids in
  // determining original composite index.

  vtkInformation* metaData = inputIter->GetCurrentMetaData();

  vtkInformation* properties = sel->GetProperties();
  if (properties->Has(vtkSelection::COMPOSITE_INDEX()) &&
    metaData->Has(vtkSelection::COMPOSITE_INDEX()) &&
    (properties->Get(vtkSelection::COMPOSITE_INDEX()) ==
     metaData->Get(vtkSelection::COMPOSITE_INDEX())))
    {
    return this->LocateSelection(sel);
    }

  if (properties->Has(vtkSelection::HIERARCHICAL_LEVEL()) &&
    properties->Has(vtkSelection::HIERARCHICAL_INDEX()) &&
    metaData->Has(vtkSelection::HIERARCHICAL_LEVEL()) &&
    metaData->Has(vtkSelection::HIERARCHICAL_INDEX()) &&
    (metaData->Get(vtkSelection::HIERARCHICAL_LEVEL()) ==
     properties->Get(vtkSelection::HIERARCHICAL_LEVEL())) &&
    (metaData->Get(vtkSelection::HIERARCHICAL_INDEX()) ==
     properties->Get(vtkSelection::HIERARCHICAL_INDEX())))
    {
    return this->LocateSelection(sel);
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelectionStreamer::LocateSelection(vtkSelection* sel)
{
  if (!sel)
    {
    return 0;
    }

  if (sel->GetContentType() == vtkSelection::SELECTIONS)
    {
    unsigned int numChildren = sel->GetNumberOfChildren();
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* located = this->LocateSelection(sel->GetChild(cc)); 
      if (located)
        {
        return located;
        }
      }
    return 0;
    }

  vtkInformation* properties = sel->GetProperties();
  int myId = this->Controller? this->Controller->GetLocalProcessId() : 0;
  if (properties->Has(vtkSelection::PROCESS_ID()) &&
      properties->Get(vtkSelection::PROCESS_ID()) != -1 &&
      properties->Get(vtkSelection::PROCESS_ID()) != myId)
    {
    // input selection process id is not same as this process's id, which means
    // that the input selection is not applicable to this process. Nothing to do
    // in that case.
    return 0;
    }

  if (sel->GetContentType() != vtkSelection::BLOCKS &&
    sel->GetContentType() != vtkSelection::INDICES)
    {
    // only BLOCKS or INDICES based selections are supported.
    return 0;
    }

  int selFieldType = sel->GetFieldType();
  if (selFieldType == vtkSelection::POINT &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    return sel;
    }

  if (selFieldType == vtkSelection::CELL &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    return sel;
    }

  if (selFieldType == vtkSelection::VERTEX &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
    {
    return sel;
    }

  if (selFieldType == vtkSelection::EDGE &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_EDGES)
    {
    return sel;
    }

  if (selFieldType == vtkSelection::ROW &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
    return sel;
    }

  return 0;
}

//----------------------------------------------------------------------------
bool vtkSelectionStreamer::PassBlock(vtkSelection* output, vtkSelection* input,
  vtkIdType offset, vtkIdType count)
{
  bool hit = false;
  output->GetProperties()->Copy(input->GetProperties());
  int myId = this->Controller? this->Controller->GetLocalProcessId()  :0;
  output->GetProperties()->Set(vtkSelection::PROCESS_ID(), myId);
  if (input->GetContentType() == vtkSelection::INDICES)
    {
    vtkIdTypeArray* outIds = vtkIdTypeArray::New();
    outIds->SetNumberOfComponents(1);
    output->SetSelectionList(outIds);
    outIds->Delete();
    for (vtkIdType cc=0; cc < count; cc++)
      {
      vtkIdType curVal = offset + cc;
      if (input->GetSelectionList()->LookupValue(vtkVariant(curVal)) != -1)
        {
        outIds->InsertNextValue(curVal);
        hit = true;
        }
      }
    }
  return hit;
}

//----------------------------------------------------------------------------
void vtkSelectionStreamer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


