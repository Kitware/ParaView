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
#include "vtkSelectionNode.h"
#include "vtkInformation.h"
#include "vtkCompositeDataIterator.h"
#include "vtkUnsignedIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiProcessController.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSelectionStreamer);
vtkCxxSetObjectMacro(vtkSelectionStreamer, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkSelectionStreamer::vtkSelectionStreamer()
{
  // port 0 -- vtkSelection
  // port 1 -- vtkDataObject used to detemine what ids constitute a block.
  this->SetNumberOfInputPorts(2);
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkSelectionStreamer::~vtkSelectionStreamer()
{
  this->SetController(0);
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

  vtkTable* inputBlockTable = vtkTable::SafeDownCast(inputDO);

  if(this->GetController() && this->GetController()->GetNumberOfProcesses() > 1)
    {
    // CAUTION
    // if the number of processes is bigger than the block size (1024) the
    // broadcast will be a bottle neck. Instead just send the table to only
    // a subset of processes

    vtkIdType* tableSizes = new vtkIdType[this->GetController()->GetNumberOfProcesses()];
    vtkIdType localSize = inputBlockTable->GetNumberOfRows();
    this->GetController()->AllGather(&localSize, tableSizes, 1);
    int pidSender = 0;
    for(int i=0; i < this->GetController()->GetNumberOfProcesses(); i++)
      {
      if(tableSizes[i] > 0)
        {
        pidSender = i;
        break;
        }
      }
    this->GetController()->GetCommunicator()->Broadcast(inputBlockTable, pidSender);
    delete[] tableSizes;
    }

  if (!inputDO->IsA("vtkCompositeDataSet"))
    {
    vtkSelectionNode* inSel = this->LocateSelection(inputSel);
    if (inSel)
      {
      vtkSmartPointer<vtkSelectionNode> outputNode =
        vtkSmartPointer<vtkSelectionNode>::New();
      this->PassBlock(outputNode, inSel, inputBlockTable);
      output->AddNode(outputNode);
      }
    return 1;
    }

  int myId = this->Controller ? this->Controller->GetLocalProcessId()  :0;

  vtkSmartPointer<vtkCompositeDataSet> input =
    vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkstd::vector<vtkSmartPointer<vtkSelectionNode> > output_nodes;

  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->SkipEmptyNodesOff();
  int cc=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem(), cc++)
    {
    vtkSelectionNode* curSel = this->LocateSelection(iter, inputSel);
    if (!curSel)
      {
      continue;
      }
    vtkSelectionNode* curOutputSel = vtkSelectionNode::New();
    curOutputSel->GetProperties()->Copy(curSel->GetProperties());
    curOutputSel->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), myId);
    bool hit = false;
    if (curSel->GetContentType() == vtkSelectionNode::BLOCKS)
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
      hit |= this->PassBlock(curOutputSel, curSel, inputBlockTable);
      }
    if (hit)
      {
      output_nodes.push_back(curOutputSel);
      }
    curOutputSel->Delete();
    }
  iter->Delete();

  for (unsigned int kk=0; kk < output_nodes.size(); kk++)
    {
    output->AddNode(output_nodes[kk]);
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelectionStreamer::LocateSelection(
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
 
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc = 0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);

    // vtkAttributeDataToTableFilter puts in this meta-data to which aids in
    // determining original composite index.

    vtkInformation* metaData = inputIter->GetCurrentMetaData();

    vtkInformation* properties = node->GetProperties();
    if (properties->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
      metaData->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
      (properties->Get(vtkSelectionNode::COMPOSITE_INDEX()) ==
       metaData->Get(vtkSelectionNode::COMPOSITE_INDEX())))
      {
      return (this->LocateSelection(node) ? node : 0);
      }

    if (properties->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
      properties->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
      metaData->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
      metaData->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
      (metaData->Get(vtkSelectionNode::HIERARCHICAL_LEVEL()) ==
       properties->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())) &&
      (metaData->Get(vtkSelectionNode::HIERARCHICAL_INDEX()) ==
       properties->Get(vtkSelectionNode::HIERARCHICAL_INDEX())))
      {
      return (this->LocateSelection(node) ? node : 0);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkSelectionStreamer::LocateSelection(vtkSelection* sel)
{
  if (!sel)
    {
    return 0;
    }
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    if (this->LocateSelection(sel->GetNode(cc)))
      {
      return sel->GetNode(cc);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
bool vtkSelectionStreamer::LocateSelection(vtkSelectionNode* node)
{
  vtkInformation* properties = node->GetProperties();
  int myId = this->Controller? this->Controller->GetLocalProcessId() : 0;
  if (properties->Has(vtkSelectionNode::PROCESS_ID()) &&
      properties->Get(vtkSelectionNode::PROCESS_ID()) != -1 &&
      properties->Get(vtkSelectionNode::PROCESS_ID()) != myId)
    {
    // input selection process id is not same as this process's id, which means
    // that the input selection is not applicable to this process. Nothing to do
    // in that case.
    return false;
    }
  if (node->GetContentType() != vtkSelectionNode::BLOCKS &&
    node->GetContentType() != vtkSelectionNode::INDICES)
    {
    // only BLOCKS or INDICES based selections are supported.
    return false;
    }
  int selFieldType = node->GetFieldType();
  if (selFieldType == vtkSelectionNode::POINT &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    return true;
    }
  if (selFieldType == vtkSelectionNode::CELL &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    return true;
    }
  if (selFieldType == vtkSelectionNode::VERTEX &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
    {
    return true;
    }
  if (selFieldType == vtkSelectionNode::EDGE &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_EDGES)
    {
    return true;
    }
  if (selFieldType == vtkSelectionNode::ROW &&
    this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSelectionStreamer::PassBlock(vtkSelectionNode* output,
                                     vtkSelectionNode* input,
                                     vtkTable* currentBlockTable)
{
  if (!currentBlockTable)
    return false;

  bool hit = false;
  output->GetProperties()->Copy(input->GetProperties());
  int myId = this->Controller? this->Controller->GetLocalProcessId()  :0;
  output->GetProperties()->Set(vtkSelectionNode::PROCESS_ID(), myId);

  vtkIdTypeArray* idsArray =
      vtkIdTypeArray::SafeDownCast(
          currentBlockTable->GetColumnByName("vtkOriginalIndices"));

  if (input->GetContentType() == vtkSelectionNode::INDICES && idsArray)
    {
    vtkIdTypeArray* outIds = vtkIdTypeArray::New();
    outIds->SetNumberOfComponents(1);
    output->SetSelectionList(outIds);
    outIds->Delete();

    for(vtkIdType i=0;i<idsArray->GetNumberOfTuples();++i)
      {
      vtkIdType curVal = idsArray->GetValue(i);
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
