/*=========================================================================

  Program:   ParaView
  Module:    vtkIndexBasedBlockFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIndexBasedBlockFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkExecutive.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkIndexBasedBlockFilter);
vtkCxxRevisionMacro(vtkIndexBasedBlockFilter, "1.1");
vtkCxxSetObjectMacro(vtkIndexBasedBlockFilter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIndexBasedBlockFilter::vtkIndexBasedBlockFilter()
{
  this->Block = 0;
  this->BlockSize = 1024;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->StartIndex= -1;
  this->EndIndex= -1;
  this->FieldType = POINT_DATA_FIELD;
}

//----------------------------------------------------------------------------
vtkIndexBasedBlockFilter::~vtkIndexBasedBlockFilter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkIndexBasedBlockFilter::RequestData(vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector*)
{
  // Do communication and decide which processes pass what data through.
  if (!this->DetermineBlockIndices())
    {
    return 0;
    }

  if (this->StartIndex < 0 || this->EndIndex < 0)
    {
    // Nothing to do, the output must be empty since this process does not have
    // the requested block of data.
    return 1;
    }
  
  vtkDataSet* input = vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
  vtkTable* output = this->GetOutput();

  vtkFieldData* inFD = 0;
  switch (this->FieldType)
    {

  case DATA_OBJECT_FIELD:
    inFD = input->GetFieldData();
    break;

  case CELL_DATA_FIELD:
    inFD = input->GetCellData();
    break;

  case POINT_DATA_FIELD:
  default:
    inFD = input->GetPointData();
    break;
    }

  vtkFieldData* outFD = vtkFieldData::New();
  outFD->CopyStructure(inFD);
  outFD->SetNumberOfTuples(this->EndIndex-this->StartIndex+1);

  vtkIdType inIndex, outIndex;
  for (inIndex=this->StartIndex, outIndex=0; inIndex <= this->EndIndex; ++inIndex, ++outIndex)
    {
    outFD->SetTuple(outIndex, inIndex, inFD);
    }

  output->SetFieldData(outFD);
  outFD->Delete();
  return 1;
}

//----------------------------------------------------------------------------
bool vtkIndexBasedBlockFilter::DetermineBlockIndices()
{
  vtkIdType blockStartIndex = this->Block*this->BlockSize;
  vtkIdType blockEndIndex = blockStartIndex + this->BlockSize - 1;

  vtkDataSet* input = vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));

  vtkIdType numFields;
  switch (this->FieldType)
    {
  case CELL_DATA_FIELD:
    numFields = input->GetNumberOfCells();
    break;

  case DATA_OBJECT_FIELD:
    numFields = input->GetFieldData()->GetNumberOfTuples();
    break;

  case POINT_DATA_FIELD:
  default:
    numFields = input->GetNumberOfPoints();
    }

  int numProcs = this->Controller? this->Controller->GetNumberOfProcesses():1;
  if (numProcs<=1)
    {
    this->StartIndex = blockStartIndex;
    this->EndIndex = (blockEndIndex < numFields)? blockEndIndex :
      (numFields - this->StartIndex - 1);
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

  // cout << myId <<  ": Delivering : " << this->StartIndex << " --> " << this->EndIndex << endl;
  return true;
}

//----------------------------------------------------------------------------
void vtkIndexBasedBlockFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Block: " << this->Block << endl;
  os << indent << "BlockSize: " << this->BlockSize << endl;
  os << indent << "FieldType: " << this->FieldType << endl;
}


