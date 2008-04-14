/*=========================================================================

  Program:   ParaView
  Module:    vtkIndexBasedBlockSelectionFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIndexBasedBlockSelectionFilter.h"

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkIdTypeArray.h"
#include "vtkIndexBasedBlockFilter.h"
#include "vtkInformation.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedIntArray.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkIndexBasedBlockSelectionFilter);
vtkCxxRevisionMacro(vtkIndexBasedBlockSelectionFilter, "1.6");
//----------------------------------------------------------------------------
vtkIndexBasedBlockSelectionFilter::vtkIndexBasedBlockSelectionFilter()
{
  // port 0 -- vtkSelection
  // port 1 -- vtkDataSet used to detemine what ids constitute a block.
  this->SetNumberOfInputPorts(2);

  this->BlockFilter = vtkIndexBasedBlockFilter::New();

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->StartIndex= -1;
  this->EndIndex= -1;
}

//----------------------------------------------------------------------------
vtkIndexBasedBlockSelectionFilter::~vtkIndexBasedBlockSelectionFilter()
{
  this->SetController(0);

  this->BlockFilter->Delete();
  this->BlockFilter = 0;
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::SetBlockSize(vtkIdType size)
{
  this->BlockFilter->SetBlockSize(size);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkIndexBasedBlockSelectionFilter::GetBlockSize()
{
  return this->BlockFilter->GetBlockSize();
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::SetBlock(vtkIdType block)
{
  this->BlockFilter->SetBlock(block);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkIndexBasedBlockSelectionFilter::GetBlock()
{
  return this->BlockFilter->GetBlock();
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::SetCompositeDataSetIndex(unsigned int index)
{
  this->BlockFilter->SetCompositeDataSetIndex(index);
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkIndexBasedBlockSelectionFilter::GetCompositeDataSetIndex()
{
  return this->BlockFilter->GetCompositeDataSetIndex();
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::SetFieldType(int type)
{
  this->BlockFilter->SetFieldType(type);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockSelectionFilter::GetFieldType()
{
  return this->BlockFilter->GetFieldType();
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::SetController(vtkMultiProcessController* contr)
{
  vtkSetObjectBodyMacro(Controller, vtkMultiProcessController, contr);
  this->BlockFilter->SetController(contr);
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockSelectionFilter::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }

  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockSelectionFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkSelection* output = vtkSelection::GetData(outputVector, 0);
  output->Clear();

  vtkInformation* outProperties = output->GetProperties();

  int myId = this->Controller? this->Controller->GetLocalProcessId()  :0;
  outProperties->Set(vtkSelection::PROCESS_ID(), myId);
  output->SetContentType(vtkSelection::INDICES);

  int fieldType = this->GetFieldType();
  int myType = (fieldType == vtkIndexBasedBlockFilter::POINT)?
    vtkSelection::POINT : vtkSelection::CELL;
  outProperties->Set(vtkSelection::FIELD_TYPE(), myType);

  if (fieldType == vtkIndexBasedBlockFilter::FIELD)
    {
    // nothing to do.
    return 1;
    }

  vtkDataObject* actualDataInput = vtkDataObject::GetData(inputVector[1], 0);
  vtkMultiPieceDataSet* datainput =
    this->BlockFilter->GetPieceToProcess(actualDataInput);

  if (!datainput)
    {
    return 1;
    }

  // Do communication and decide which processes pass what data through.
  if (!this->DetermineBlockIndices(datainput))
    {
    return 0;
    }

  vtkSelection* input = vtkSelection::GetData(inputVector[0], 0);
  input = this->LocateSelection(myType,
    this->GetCompositeDataSetIndex(), input, actualDataInput);

  if (!input || this->StartIndex < 0 || this->EndIndex < 0)
    {
    // Nothing to do, the output must be empty since this process does not have
    // the requested block of data.
    return 1;
    }

  return this->RequestDataInternal(input, output, datainput);
}

//----------------------------------------------------------------------------
bool vtkIndexBasedBlockSelectionFilter::DetermineBlockIndices(
  vtkMultiPieceDataSet* input)
{
  return this->BlockFilter->DetermineBlockIndices(input,
    this->StartIndex, this->EndIndex);
}

//----------------------------------------------------------------------------
vtkSelection* vtkIndexBasedBlockSelectionFilter::LocateSelection(
  int fieldType,
  unsigned int composite_index, vtkSelection* sel, vtkDataObject* input)
{
  if (!input->IsA("vtkCompositeDataSet"))
    {
    if (sel->GetContentType() == vtkSelection::SELECTIONS)
      {
      unsigned int numChildren = sel->GetNumberOfChildren();
      for (unsigned int cc=0; cc < numChildren; cc++)
        {
        vtkSelection* child = sel->GetChild(cc);
        if (child && child->GetFieldType() == fieldType)
          {
          return child;
          }
        }
      }
    else if (sel->GetFieldType() == fieldType)
      {
      return sel;
      }
    return 0;
    }


  unsigned int level = 0;
  unsigned int indexMin = 0;
  unsigned int indexMax = 0;
  bool hierarchical_index_valid = false;
  if (input->IsA("vtkHierarchicalBoxDataSet"))
    {
    hierarchical_index_valid = true;
    // Convert the composite_index to hierarchical index.
    vtkHierarchicalBoxDataIterator* iter =
      vtkHierarchicalBoxDataIterator::SafeDownCast(
        static_cast<vtkHierarchicalBoxDataSet*>(input)->NewIterator());
    iter->VisitOnlyLeavesOff();
    for (iter->InitTraversal();
      !iter->IsDoneWithTraversal()  && (iter->GetCurrentFlatIndex() <= composite_index);
      iter->GoToNextItem())
      {
      if (iter->GetCurrentFlatIndex() == composite_index)
        {
        level = iter->GetCurrentLevel();
        vtkMultiPieceDataSet* levelPieces = vtkMultiPieceDataSet::SafeDownCast(
          iter->GetCurrentDataObject());
        if (levelPieces)
          {
          indexMin = 0;
          indexMax = levelPieces->GetNumberOfPieces()-1;
          }
        else
          {
          indexMin = iter->GetCurrentIndex();
          indexMax = indexMin;
          }
        break;
        }
      }
    iter->Delete();
    }

  // input is a vtkCompositeDataSet.

  vtkstd::vector<vtkSelection*> selections;

  if (sel && sel->GetContentType() == vtkSelection::SELECTIONS)
    {
    unsigned int numChildren = sel->GetNumberOfChildren();
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* child = sel->GetChild(cc);
      if (!child)
        {
        continue;
        }
      vtkInformation* properties = child->GetProperties();
      if (child->GetFieldType() == fieldType &&
        properties->Has(vtkSelection::COMPOSITE_INDEX()) &&
        static_cast<unsigned int>(properties->Get(vtkSelection::COMPOSITE_INDEX()))
        == composite_index)
        {
        return child;
        }
      else if (hierarchical_index_valid &&
        child->GetFieldType() == fieldType &&
        properties->Has(vtkSelection::HIERARCHICAL_LEVEL()) &&
        properties->Has(vtkSelection::HIERARCHICAL_INDEX()) &&
        static_cast<unsigned int>(properties->Get(vtkSelection::HIERARCHICAL_LEVEL())) == level &&
        static_cast<unsigned int>(properties->Get(vtkSelection::HIERARCHICAL_INDEX())) >= indexMin &&
        static_cast<unsigned int>(properties->Get(vtkSelection::HIERARCHICAL_INDEX())) <= indexMax)
        {
        selections.push_back(child);
        }
      }

    if (hierarchical_index_valid && selections.size() > 0)
      {
      vtkSelection* compSel = vtkSelection::New();
      compSel->SetContentType(vtkSelection::SELECTIONS);
      for (unsigned int cc=0; cc < selections.size(); cc++)
        {
        compSel->AddChild(selections[cc]);
        }
      this->Temporary.TakeReference(compSel);
      return this->Temporary;
      }

    return NULL;
    }

  return sel;
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockSelectionFilter::RequestDataInternal(vtkSelection* input,
  vtkSelection* output, vtkMultiPieceDataSet* pieces)
{
  unsigned int numPieces = pieces->GetNumberOfPieces();
  if (numPieces == 1)
    {
    return this->RequestDataInternal(this->StartIndex, this->EndIndex,
      input, output);
    }

  int fieldType = this->GetFieldType();

  vtkstd::vector<vtkIdType> pieceOffsets;
  vtkIdType offset = 0;
  for (unsigned int cc=0; cc < numPieces; cc++)
    {
    pieceOffsets.push_back(offset);
    vtkDataSet* piece = pieces->GetPiece(cc);
    if (piece)
      {
      if (fieldType == vtkIndexBasedBlockFilter::CELL)
        {
        offset += piece->GetCellData()->GetNumberOfTuples();
        }
      else
        {
        offset += piece->GetNumberOfPoints();
        }
      }
    }

  vtkstd::vector<vtkSelection*> selections;
  if (input->GetContentType() == vtkSelection::SELECTIONS)
    {
    for (unsigned int kk=0; kk < input->GetNumberOfChildren(); kk++)
      {
      selections.push_back(input->GetChild(kk));
      }
    }
  else
    {
    selections.push_back(input);
    }

  vtkstd::vector<vtkSmartPointer<vtkSelection> > outSelections;

  vtkstd::vector<vtkSelection*>::iterator iter;
  for (iter = selections.begin(); iter != selections.end(); iter++)
    {
    vtkSmartPointer<vtkSelection> outChild = vtkSmartPointer<vtkSelection>::New();
    if ((*iter)->GetProperties()->Has(vtkSelection::HIERARCHICAL_INDEX()))
      {
      unsigned int hi = (*iter)->GetProperties()->Get(vtkSelection::HIERARCHICAL_INDEX());
      vtkIdType pieceOffset = pieceOffsets[hi];
      vtkIdType startIndex = (pieceOffset > this->StartIndex? 0 : this->StartIndex - pieceOffset);
      vtkIdType endIndex = (pieceOffset > this->StartIndex? this->EndIndex-pieceOffset: this->EndIndex);
      if (!this->RequestDataInternal(startIndex, endIndex, *iter, outChild))
        {
        return 0;
        }
      if (outChild->GetContentType() == vtkSelection::INDICES)
        {
        outSelections.push_back(outChild);
        }
      }
    }

  if (outSelections.size() == 1)
    {
    output->ShallowCopy(outSelections[0]);
    }
  else if (outSelections.size() > 1)
    {
    output->SetContentType(vtkSelection::SELECTIONS);
    for (unsigned int cc=0; cc < outSelections.size(); cc++)
      {
      output->AddChild(outSelections[cc]);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockSelectionFilter::RequestDataInternal(
  vtkIdType startIndex, vtkIdType endIndex,
  vtkSelection* input,
  vtkSelection* output)
{
  if (startIndex > endIndex)
    {
    // nothing to do.
    return 1;
    }

  vtkInformation* inProperties = input->GetProperties();

  if ((inProperties->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::INDICES) &&
    (inProperties->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::BLOCKS))
    {
    return 1;
    }

  int myId = this->Controller? this->Controller->GetLocalProcessId()  :0;
  // cout << myId << ": In PID = " <<  inProperties->Get(vtkSelection::PROCESS_ID()) << endl;
  if (inProperties->Has(vtkSelection::PROCESS_ID()) &&
      inProperties->Get(vtkSelection::PROCESS_ID()) != -1 &&
      inProperties->Get(vtkSelection::PROCESS_ID()) != myId)
    {
    // input selection process id is not same as this process's id, which means
    // that the input selection is not applicable to this process. Nothing to do
    // in that case.
    return 1;
    }

  if (inProperties->Get(vtkSelection::CONTENT_TYPE()) == vtkSelection::BLOCKS)
    {
    // If input selection is of type vtkSelection::BLOCKS, 
    // the output contains the current composite block if it is selected in the
    // input.
    output->GetProperties()->Copy(inProperties);
    if (input->GetSelectionList()->LookupValue(
        vtkVariant(this->GetCompositeDataSetIndex())) != -1)
      {
      vtkUnsignedIntArray* selList = vtkUnsignedIntArray::New();
      selList->SetNumberOfTuples(1);
      selList->SetValue(0, this->GetCompositeDataSetIndex());
      output->SetSelectionList(selList);
      selList->Delete();
      }
    return 1;
    }

  int fieldType = this->GetFieldType();
  int myType = (fieldType == vtkIndexBasedBlockFilter::POINT)?
    vtkSelection::POINT : vtkSelection::CELL;

  if (inProperties->Get(vtkSelection::FIELD_TYPE()) != myType)
    {
    // If we are producing points and the input selection is not a point based
    // selection, we treat it as an empty selection. (same is true for cell base
    // selections).
    return 1;
    }
  output->GetProperties()->Copy(input->GetProperties());

  // Eventually we'll do some smart lookup to determine which IDs are selected.
  vtkIdTypeArray* inIds = vtkIdTypeArray::SafeDownCast(
    input->GetSelectionList());

  vtkIdTypeArray* outIds = vtkIdTypeArray::New();
  outIds->SetNumberOfComponents(1);
  vtkIdType numVals = inIds? inIds->GetNumberOfTuples() : 0;
  for (vtkIdType cc=0; cc < numVals; cc++)
    {
    vtkIdType curVal = inIds->GetValue(cc);
    if (startIndex <= curVal && curVal <= endIndex)
      {
      outIds->InsertNextValue(curVal);
      }
    }
  output->SetSelectionList(outIds);
  outIds->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


