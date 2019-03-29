/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCSVWriter.h"

#include "vtkAlgorithm.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkAttributeDataToTableFilter.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVMergeTables.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkCSVWriter);
//-----------------------------------------------------------------------------
vtkCSVWriter::vtkCSVWriter()
{
  this->StringDelimiter = 0;
  this->FieldDelimiter = 0;
  this->UseStringDelimiter = true;
  this->SetStringDelimiter("\"");
  this->SetFieldDelimiter(",");
  this->Stream = 0;
  this->FileName = 0;
  this->Precision = 5;
  this->UseScientificNotation = true;
  this->FieldAssociation = 0;
  this->AddMetaData = false;
}

//-----------------------------------------------------------------------------
vtkCSVWriter::~vtkCSVWriter()
{
  this->SetStringDelimiter(0);
  this->SetFieldDelimiter(0);
  this->SetFileName(0);
  delete this->Stream;
}

//-----------------------------------------------------------------------------
int vtkCSVWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCSVWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      controller->GetNumberOfProcesses());
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), controller->GetLocalProcessId());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
bool vtkCSVWriter::OpenFile(bool append)
{
  if (!this->FileName)
  {
    vtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return false;
  }

  vtkDebugMacro(<< "Opening file for writing...");

  ofstream* fptr = append == false ? new ofstream(this->FileName, ios::out)
                                   : new ofstream(this->FileName, ios::out | ios::app);

  if (fptr->fail())
  {
    vtkErrorMacro(<< "Unable to open file: " << this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    delete fptr;
    return false;
  }

  this->Stream = fptr;
  return true;
}

namespace
{
//-----------------------------------------------------------------------------
template <class iterT>
void vtkCSVWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ofstream* stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << iter->GetValue(index + cc);
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex,
  ofstream* stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << writer->GetString(iter->GetValue(index + cc));
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<char>* iter, vtkIdType tupleIndex,
  ofstream* stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << static_cast<int>(iter->GetValue(index + cc));
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<unsigned char>* iter, vtkIdType tupleIndex,
  ofstream* stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << static_cast<int>(iter->GetValue(index + cc));
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
bool SomethingForMeToDo(int myRank, const std::vector<vtkIdType>& numRowsGlobal)
{
  // there's something for me to do if either I have rows or I'm process 0
  // and no process rows in which case I have to write the header
  if (numRowsGlobal[myRank] > 0)
  {
    return true;
  }
  else if (myRank == 0)
  {
    for (size_t i = 1; i < numRowsGlobal.size(); i++)
    {
      if (numRowsGlobal[i] > 0)
      {
        return false; // someone else will write the header info
      }
    }
    return true; // I have to write the header info even though I have no rows
  }
  return false;
}

//-----------------------------------------------------------------------------
bool DoIWriteTheHeader(int myRank, const std::vector<vtkIdType>& numRowsGlobal)
{
  for (int i = 0; i < myRank; i++)
  {
    if (numRowsGlobal[i])
    {
      return false; // someone before me has data and will write the header
    }
  }
  return true; // note if process 0 is here it will write the header
}

//-----------------------------------------------------------------------------
void StartProcessWrite(
  int myRank, const std::vector<vtkIdType>& numRowsGlobal, vtkMultiProcessController* controller)
{
  if (DoIWriteTheHeader(myRank, numRowsGlobal) == false)
  {
    int prevProc = myRank - 1;
    while (static_cast<size_t>(prevProc) > 0 && numRowsGlobal[prevProc] == 0)
    {
      prevProc--;
    }

    int tmp = 0; // just used for the blocking send/receive
    controller->Receive(&tmp, 1, prevProc, 11419);
  }
}

//-----------------------------------------------------------------------------
void EndProcessWrite(
  int myRank, const std::vector<vtkIdType>& numRowsGlobal, vtkMultiProcessController* controller)
{
  int nextProc = myRank + 1;
  while (static_cast<size_t>(nextProc) < numRowsGlobal.size() && numRowsGlobal[nextProc] == 0)
  {
    nextProc++;
  }
  if (static_cast<size_t>(nextProc) < numRowsGlobal.size())
  {
    int tmp = 0; // just used for the blocking send/receive
    controller->Send(&tmp, 1, nextProc, 11419);
  }
}

} // end anonymous namespace

//-----------------------------------------------------------------------------
vtkStdString vtkCSVWriter::GetString(vtkStdString string)
{
  if (this->UseStringDelimiter && this->StringDelimiter)
  {
    vtkStdString temp = this->StringDelimiter;
    temp += string + this->StringDelimiter;
    return temp;
  }
  return string;
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::WriteData()
{
  vtkSmartPointer<vtkTable> table = vtkTable::SafeDownCast(this->GetInput());
  if (table == nullptr)
  {
    vtkNew<vtkAttributeDataToTableFilter> attributeDataToTableFilter;
    attributeDataToTableFilter->SetInputDataObject(this->GetInput());
    attributeDataToTableFilter->SetFieldAssociation(this->FieldAssociation);
    attributeDataToTableFilter->SetAddMetaData(this->AddMetaData);
    attributeDataToTableFilter->Update();
    table = attributeDataToTableFilter->GetOutput();
    if (table == nullptr)
    {
      vtkNew<vtkPVMergeTables> mergeTables;
      mergeTables->SetInputConnection(attributeDataToTableFilter->GetOutputPort());
      mergeTables->Update();
      table = mergeTables->GetOutput();
    }
  }
  this->WriteTable(table);
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::WriteTable(vtkTable* table)
{
  vtkIdType numRows = table->GetNumberOfRows();
  vtkDataSetAttributes* dsa = table->GetRowData();

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int myRank = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();
  std::vector<vtkIdType> numRowsGlobal(numProcs, 0);
  numRowsGlobal[myRank] = numRows;

  controller->AllGather(&numRows, numRowsGlobal.data(), 1);

  if (SomethingForMeToDo(myRank, numRowsGlobal) == false)
  {
    return;
  }

  StartProcessWrite(myRank, numRowsGlobal, controller);
  bool writeHeader = DoIWriteTheHeader(myRank, numRowsGlobal);

  if (!this->OpenFile(!writeHeader))
  {
    EndProcessWrite(myRank, numRowsGlobal, controller); // to make sure we don't have a deadlock
    return;
  }

  std::vector<vtkSmartPointer<vtkArrayIterator> > columnsIters;

  int cc;
  int numArrays = dsa->GetNumberOfArrays();
  bool first = true;
  // Keep track of which arrays to output in columnsIters and write header:
  for (cc = 0; cc < numArrays; cc++)
  {
    vtkAbstractArray* array = dsa->GetAbstractArray(cc);
    for (int comp = 0; comp < array->GetNumberOfComponents(); comp++)
    {
      if (writeHeader)
      {
        if (!first)
        {
          (*this->Stream) << this->FieldDelimiter;
        }
        first = false;
        std::ostringstream array_name;
        array_name << array->GetName();
        if (array->GetNumberOfComponents() > 1)
        {
          array_name << ":" << comp;
        }
        (*this->Stream) << this->GetString(array_name.str());
      }
    }
    vtkArrayIterator* iter = array->NewIterator();
    columnsIters.push_back(iter);
    iter->Delete();
  }
  if (writeHeader)
  {
    (*this->Stream) << "\n";
  }

  // push the floating point precision/notation type.
  if (this->UseScientificNotation)
  {
    (*this->Stream) << std::scientific;
  }

  (*this->Stream) << std::setprecision(this->Precision);

  for (vtkIdType index = 0; index < numRows; index++)
  {
    first = true;
    std::vector<vtkSmartPointer<vtkArrayIterator> >::iterator iter;
    for (iter = columnsIters.begin(); iter != columnsIters.end(); ++iter)
    {
      switch ((*iter)->GetDataType())
      {
        vtkArrayIteratorTemplateMacro(vtkCSVWriterGetDataString(
          static_cast<VTK_TT*>(iter->GetPointer()), index, this->Stream, this, &first));
      }
    }
    (*this->Stream) << "\n";
  }

  this->Stream->close();

  EndProcessWrite(myRank, numRowsGlobal, controller);
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldDelimiter: " << (this->FieldDelimiter ? this->FieldDelimiter : "(none)")
     << endl;
  os << indent << "StringDelimiter: " << (this->StringDelimiter ? this->StringDelimiter : "(none)")
     << endl;
  os << indent << "UseStringDelimiter: " << this->UseStringDelimiter << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "UseScientificNotation: " << this->UseScientificNotation << endl;
  os << indent << "Precision: " << this->Precision << endl;
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  os << indent << "AddMetaData: " << this->AddMetaData << endl;
}
