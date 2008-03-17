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

#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkIdTypeArray.h"
#include "vtkIndexBasedBlockFilter.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkIndexBasedBlockSelectionFilter);
vtkCxxRevisionMacro(vtkIndexBasedBlockSelectionFilter, "1.2");
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

  vtkMultiPieceDataSet* datainput = this->BlockFilter->GetPieceToProcess(
    vtkDataObject::GetData(inputVector[1], 0));

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
  input = this->LocateSelection(this->GetCompositeDataSetIndex(), input);

  if (!input || this->StartIndex < 0 || this->EndIndex < 0)
    {
    // Nothing to do, the output must be empty since this process does not have
    // the requested block of data.
    return 1;
    }
  
  vtkInformation* inProperties = input->GetProperties();
  if (inProperties->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::INDICES)
    {
    vtkErrorMacro("This filter can only handle INDEX based selections.");
    return 0;
    }
 
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

  int inv = 0;
  if (!inProperties->Has(vtkSelection::INVERSE()))
    {
    inv = inProperties->Get(vtkSelection::INVERSE());
    }
  output->GetProperties()->Set(vtkSelection::INVERSE(), inv);

  if (!inProperties->Has(vtkSelection::FIELD_TYPE()))
    {
    return 1;
    }

  if (inProperties->Get(vtkSelection::FIELD_TYPE()) != myType)
    {
    // If we are producing points and the input selection is not a point based
    // selection, we treat it as an empty selection. (same is true for cell base
    // selections).
    return 1;
    }
  
  // Eventually we'll do some smart lookup to determine which IDs are selected.
  vtkIdTypeArray* inIds = vtkIdTypeArray::SafeDownCast(
    input->GetSelectionList());
  vtkIdTypeArray* outIds = vtkIdTypeArray::New();
  outIds->SetNumberOfComponents(1);

  vtkIdType numVals = inIds? inIds->GetNumberOfTuples() : 0;
  for (vtkIdType cc=0; cc < numVals; cc++)
    {
    vtkIdType curVal = inIds->GetValue(cc);
    if (this->StartIndex <= curVal && curVal <= this->EndIndex)
      {
      outIds->InsertNextValue(curVal);
      }
    }
  output->SetSelectionList(outIds);
  outIds->Delete();
  return 1;
}

//----------------------------------------------------------------------------
bool vtkIndexBasedBlockSelectionFilter::DetermineBlockIndices(
  vtkMultiPieceDataSet* input)
{
  return this->BlockFilter->DetermineBlockIndices(input, this->StartIndex, this->EndIndex);
}

//----------------------------------------------------------------------------
vtkSelection* vtkIndexBasedBlockSelectionFilter::LocateSelection(
  unsigned int composite_index, vtkSelection* sel)
{
  if (sel && sel->GetContentType() == vtkSelection::SELECTIONS)
    {
    unsigned int numChildren = sel->GetNumberOfChildren();
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* child = sel->GetChild(cc);
      if (child && 
        child->GetProperties()->Has(vtkSelection::COMPOSITE_INDEX()) &&
        static_cast<unsigned int>(child->GetProperties()->Get(vtkSelection::COMPOSITE_INDEX()))
        == composite_index)
        {
        return child;
        }
      }
    return NULL;
    }

  return sel;
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


