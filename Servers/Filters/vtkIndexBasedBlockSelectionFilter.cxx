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
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkExecutive.h"

vtkStandardNewMacro(vtkIndexBasedBlockSelectionFilter);
vtkCxxRevisionMacro(vtkIndexBasedBlockSelectionFilter, "1.1");
vtkCxxSetObjectMacro(vtkIndexBasedBlockSelectionFilter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIndexBasedBlockSelectionFilter::vtkIndexBasedBlockSelectionFilter()
{
  // port 0 -- vtkSelection
  // port 1 -- vtkDataSet used to detemine what ids constitute a block.
  this->SetNumberOfInputPorts(2);

  this->Block = 0;
  this->BlockSize = 1024;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->StartIndex= -1;
  this->EndIndex= -1;
  this->FieldType = POINT;
}

//----------------------------------------------------------------------------
vtkIndexBasedBlockSelectionFilter::~vtkIndexBasedBlockSelectionFilter()
{
  this->SetController(0);
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
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockSelectionFilter::RequestData(vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector*)
{
  vtkSelection* output = this->GetOutput();
  output->Clear();

  vtkInformation* outProperties = output->GetProperties();

  int myId = this->Controller? this->Controller->GetLocalProcessId()  :0;
  outProperties->Set(vtkSelection::PROCESS_ID(), myId);
  output->SetContentType(vtkSelection::INDICES);

  int myType = (this->FieldType == vtkIndexBasedBlockSelectionFilter::POINT)? 
    vtkSelection::POINT : vtkSelection::CELL;
  outProperties->Set(vtkSelection::FIELD_TYPE(), myType);

  if (this->FieldType == FIELD)
    {
    // nothing to do.
    return 1;
    }

  // Do communication and decide which processes pass what data through.
  if (!this->DetermineBlockIndices())
    {
    return 0;
    }

  vtkSelection* input = vtkSelection::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
  vtkInformation* inProperties = input->GetProperties();

  if (this->StartIndex < 0 || this->EndIndex < 0)
    {
    // Nothing to do, the output must be empty since this process does not have
    // the requested block of data.
    return 1;
    }
  
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
bool vtkIndexBasedBlockSelectionFilter::DetermineBlockIndices()
{
  vtkIdType blockStartIndex = this->Block*this->BlockSize;
  vtkIdType blockEndIndex = blockStartIndex + this->BlockSize - 1;

  vtkDataSet* input = vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));

  vtkIdType numFields;
  switch (this->FieldType)
    {
  case CELL:
    numFields = input->GetNumberOfCells();
    break;

  case POINT:
  default:
    numFields = input->GetNumberOfPoints();
    }

  int numProcs = this->Controller? this->Controller->GetNumberOfProcesses():1;
  if (numProcs<=1)
    {
    this->StartIndex = blockStartIndex;
    this->EndIndex = (blockEndIndex < numFields)? blockEndIndex : numFields;
    // cout  << "Delivering : " << this->StartIndex << " --> " << this->EndIndex << endl;
    return true;
    }

  int myId = this->Controller->GetLocalProcessId();

  vtkIdType* gathered_data = new vtkIdType[numProcs];

  // cout << myId<< ": numFields: " << numFields<<endl;
  vtkCommunicator* comm = this->Controller->GetCommunicator();
  if (!comm->AllGather(&numFields, gathered_data, 1))
    {
    vtkErrorMacro("Failed to gather data from all processes.");
    return false;
    }

  vtkIdType mydataStartIndex=0;
  for (int cc=0; cc < myId; cc++)
    {
    mydataStartIndex += gathered_data[cc];
    }
  vtkIdType mydataEndIndex = mydataStartIndex + numFields - 1;

  if ((mydataStartIndex < blockStartIndex && mydataEndIndex < blockStartIndex) || 
    (mydataStartIndex > blockEndIndex))
    {
    // Block doesn't overlap the data we have at all.
    this->StartIndex = -1;
    this->EndIndex = -1;
    }
  else
    {
    vtkIdType startIndex = (mydataStartIndex < blockStartIndex)?
      blockStartIndex : mydataStartIndex;
    vtkIdType endIndex = (blockEndIndex < mydataEndIndex)?
      blockEndIndex : mydataEndIndex;

    this->StartIndex = startIndex - mydataStartIndex;
    this->EndIndex = endIndex - mydataStartIndex;
    }

  // cout << myId <<  ": Delivering : " << this->StartIndex << " --> " 
  // << this->EndIndex << endl;
  return true;
}


//----------------------------------------------------------------------------
void vtkIndexBasedBlockSelectionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


