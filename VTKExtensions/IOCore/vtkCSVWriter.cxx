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

#include "vtksys/FStream.hxx"

#include <numeric>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkCSVWriter);
vtkCxxSetObjectMacro(vtkCSVWriter, Controller, vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkCSVWriter::vtkCSVWriter()
{
  this->StringDelimiter = nullptr;
  this->FieldDelimiter = nullptr;
  this->UseStringDelimiter = true;
  this->SetStringDelimiter("\"");
  this->SetFieldDelimiter(",");
  this->FileName = nullptr;
  this->Precision = 5;
  this->UseScientificNotation = true;
  this->FieldAssociation = 0;
  this->AddMetaData = false;
  this->AddTime = false;
  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkCSVWriter::~vtkCSVWriter()
{
  this->SetController(nullptr);
  this->SetStringDelimiter(nullptr);
  this->SetFieldDelimiter(nullptr);
  this->SetFileName(nullptr);
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
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      (this->Controller ? this->Controller->GetNumberOfProcesses() : 1));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      (this->Controller ? this->Controller->GetLocalProcessId() : 0));
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

namespace
{
//-----------------------------------------------------------------------------
template <class iterT>
void vtkCSVWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ostream& stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if (!(*first))
    {
      stream << writer->GetFieldDelimiter();
    }
    *first = false;
    if ((index + cc) < iter->GetNumberOfValues())
    {
      stream << iter->GetValue(index + cc);
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex,
  ostream& stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if (!(*first))
    {
      stream << writer->GetFieldDelimiter();
    }
    (*first) = false;
    if ((index + cc) < iter->GetNumberOfValues())
    {
      stream << writer->GetString(iter->GetValue(index + cc));
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<char>* iter, vtkIdType tupleIndex,
  ostream& stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if (!(*first))
    {
      stream << writer->GetFieldDelimiter();
    }
    (*first) = false;
    if ((index + cc) < iter->GetNumberOfValues())
    {
      stream << static_cast<int>(iter->GetValue(index + cc));
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<unsigned char>* iter, vtkIdType tupleIndex,
  ostream& stream, vtkCSVWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        stream << writer->GetFieldDelimiter();
      }
      *first = false;
      stream << static_cast<int>(iter->GetValue(index + cc));
    }
    else
    {
      if (*first == false)
      {
        stream << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

} // end anonymous namespace

class vtkCSVWriter::CSVFile
{
  vtksys::ofstream Stream;
  std::vector<std::pair<std::string, int> > ColumnInfo;
  double Time = vtkMath::Nan();

public:
  CSVFile(double time)
    : Time(time)
  {
  }

  int Open(const char* filename)
  {
    if (!filename)
    {
      return vtkErrorCode::NoFileNameError;
    }

    this->Stream.open(filename, ios::out);
    if (this->Stream.fail())
    {
      return vtkErrorCode::CannotOpenFileError;
    }
    return vtkErrorCode::NoError;
  }

  void WriteHeader(vtkTable* table, vtkCSVWriter* self)
  {
    this->WriteHeader(table->GetRowData(), self);
  }

  void WriteHeader(vtkDataSetAttributes* dsa, vtkCSVWriter* self)
  {
    bool add_delimiter = false;
    if (!vtkMath::IsNan(this->Time))
    {
      // add a time column.
      this->Stream << "Time";
      add_delimiter = true;
    }
    for (int cc = 0, numArrays = dsa->GetNumberOfArrays(); cc < numArrays; ++cc)
    {
      auto array = dsa->GetAbstractArray(cc);
      const int num_comps = array->GetNumberOfComponents();

      // save order of arrays written out in header
      this->ColumnInfo.push_back(std::make_pair(std::string(array->GetName()), num_comps));

      for (int comp = 0; comp < num_comps; ++comp)
      {
        if (add_delimiter)
        {
          // add separator for all but the very first column
          this->Stream << self->GetFieldDelimiter();
        }
        add_delimiter = true;

        std::ostringstream array_name;
        array_name << array->GetName();
        if (array->GetNumberOfComponents() > 1)
        {
          array_name << ":" << comp;
        }
        this->Stream << self->GetString(array_name.str());
      }
    }
    this->Stream << "\n";

    // push the floating point precision/notation type.
    if (self->GetUseScientificNotation())
    {
      this->Stream << std::scientific;
    }

    this->Stream << std::setprecision(self->GetPrecision());
  }

  void WriteData(vtkTable* table, vtkCSVWriter* self)
  {
    this->WriteData(table->GetRowData(), self);
  }

  void WriteData(vtkDataSetAttributes* dsa, vtkCSVWriter* self)
  {
    std::vector<vtkSmartPointer<vtkArrayIterator> > columnsIters;
    for (const auto& cinfo : this->ColumnInfo)
    {
      auto array = dsa->GetAbstractArray(cinfo.first.c_str());
      if (array->GetNumberOfComponents() != cinfo.second)
      {
        vtkErrorWithObjectMacro(self, "Mismatched components for '" << array->GetName() << "'!");
      }
      vtkArrayIterator* iter = array->NewIterator();
      columnsIters.push_back(iter);
      iter->FastDelete();
    }

    const auto num_tuples = dsa->GetNumberOfTuples();
    for (vtkIdType cc = 0; cc < num_tuples; ++cc)
    {
      bool first_column = true;
      if (!vtkMath::IsNan(this->Time))
      {
        // add a time column.
        this->Stream << this->Time;
        first_column = false;
      }

      for (auto& iter : columnsIters)
      {
        switch (iter->GetDataType())
        {
          vtkArrayIteratorTemplateMacro(vtkCSVWriterGetDataString(
            static_cast<VTK_TT*>(iter.GetPointer()), cc, this->Stream, self, &first_column));
        }
      }
      this->Stream << "\n";
    }
  }

private:
  CSVFile(const CSVFile&) = delete;
  void operator=(const CSVFile&) = delete;
};

//-----------------------------------------------------------------------------
std::string vtkCSVWriter::GetString(std::string string)
{
  if (this->UseStringDelimiter && this->StringDelimiter)
  {
    std::string temp = this->StringDelimiter;
    temp += string + this->StringDelimiter;
    return temp;
  }
  return string;
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::WriteData()
{
  auto input = this->GetInput();

  double time = vtkMath::Nan();
  if (this->AddTime && input && input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    time = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
  }

  vtkSmartPointer<vtkTable> table = vtkTable::SafeDownCast(input);
  if (table == nullptr)
  {
    vtkNew<vtkAttributeDataToTableFilter> attributeDataToTableFilter;
    attributeDataToTableFilter->SetInputDataObject(input);
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
  assert(table != nullptr);

  auto controller = this->Controller;
  if (controller == nullptr ||
    (controller->GetNumberOfProcesses() == 1 && controller->GetLocalProcessId() == 0))
  {
    vtkCSVWriter::CSVFile file(time);
    int error_code = file.Open(this->FileName);
    if (error_code == vtkErrorCode::NoError)
    {
      file.WriteHeader(table, this);
      file.WriteData(table, this);
    }
    this->SetErrorCode(error_code);
    return;
  }

  const int myRank = controller->GetLocalProcessId();
  const int numRanks = controller->GetNumberOfProcesses();
  if (myRank > 0)
  {
    int error_code{ vtkErrorCode::NoError };
    controller->Broadcast(&error_code, 1, 0);
    if (error_code != vtkErrorCode::NoError)
    {
      this->SetErrorCode(error_code);
      return;
    }

    vtkIdType row_count = table->GetNumberOfRows();
    controller->Gather(&row_count, nullptr, 1, 0);
    if (row_count > 0)
    {
      vtkNew<vtkTable> clone;
      auto cloneRD = clone->GetRowData();
      cloneRD->CopyAllOn();
      cloneRD->CopyAllocate(table->GetRowData(), /*sze=*/1);
      cloneRD->CopyData(table->GetRowData(), 0, 1, 0);

      // send clone first so the root can determine which arrays to save to the
      // output file consistently.
      controller->Send(clone, 0, 88020);
    }

    // BARRIER
    controller->Barrier();

    if (row_count > 0)
    {
      controller->Send(table, 0, 88021);
    }
    controller->Broadcast(&error_code, 1, 0);
    this->SetErrorCode(error_code);
  }
  else
  {
    vtkCSVWriter::CSVFile file(time);
    int error_code = file.Open(this->FileName);
    controller->Broadcast(&error_code, 1, 0);
    if (error_code != vtkErrorCode::NoError)
    {
      this->SetErrorCode(error_code);
      return;
    }

    const vtkIdType row_count = table->GetNumberOfRows();
    std::vector<vtkIdType> global_row_counts(numRanks, 0);
    controller->Gather(&row_count, &global_row_counts[0], 1, 0);

    // build field list to determine which columns to write.
    vtkDataSetAttributes::FieldList columns;
    for (int rank = 0; rank < numRanks; ++rank)
    {
      if (global_row_counts[rank] > 0)
      {
        if (rank == 0)
        {
          columns.IntersectFieldList(table->GetRowData());
        }
        else
        {
          vtkNew<vtkTable> emptytable;
          controller->Receive(emptytable, vtkMultiProcessController::ANY_SOURCE, 88020);
          columns.IntersectFieldList(emptytable->GetRowData());
        }
      }
    }

    // BARRIER
    controller->Barrier();

    // now write the real data.
    vtkNew<vtkDataSetAttributes> tmp;
    tmp->CopyAllOn();
    columns.CopyAllocate(tmp, vtkDataSetAttributes::PASSDATA, /*sz=*/1, 0);

    // first write headers.
    file.WriteHeader(tmp, this);

    for (int rank = 0; rank < numRanks; ++rank)
    {
      if (global_row_counts[rank] > 0)
      {
        if (rank == 0)
        {
          file.WriteData(table, this);
        }
        else
        {
          vtkNew<vtkTable> remote_table;
          controller->Receive(remote_table.Get(), rank, 88021);
          assert(remote_table->GetNumberOfRows() > 0);
          file.WriteData(remote_table, this);
        }
      }
    }

    error_code = vtkErrorCode::NoError;
    controller->Broadcast(&error_code, 1, 0);
    this->SetErrorCode(error_code);
  }
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
  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (none)" << endl;
  }
}
