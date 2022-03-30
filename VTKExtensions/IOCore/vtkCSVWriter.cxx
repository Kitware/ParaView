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
#include "vtkDoubleArray.h"
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
#include "vtksys/SystemTools.hxx"

#include <sstream>
#include <vector>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCSVWriter);

//-----------------------------------------------------------------------------
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
  this->WriteAllTimeSteps = false;
  this->WriteAllTimeStepsSeparately = false;
  this->FileNameSuffix = nullptr;
  this->Precision = 5;
  this->UseScientificNotation = true;
  this->FieldAssociation = 0;
  this->AddMetaData = false;
  this->AddTimeStep = false;
  this->AddTime = false;
  this->CurrentTimeIndex = 0;
  this->NumberOfTimeSteps = 0;
  this->TimeValues = nullptr;
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
  this->SetFileNameSuffix(nullptr);
  if (this->TimeValues)
  {
    this->TimeValues->Delete();
    this->TimeValues = nullptr;
  }
}

//-----------------------------------------------------------------------------
int vtkCSVWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkCSVWriter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCSVWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->TimeValues)
  {
    this->TimeValues = vtkDoubleArray::New();
    vtkInformation* info = inputVector[0]->GetInformationObject(0);
    double* data = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int len = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeValues->SetNumberOfValues(len);
    if (data)
    {
      std::copy(data, data + len, this->TimeValues->GetPointer(0));
    }
  }
  if (this->TimeValues && this->WriteAllTimeSteps)
  {
    if (this->TimeValues->GetPointer(0))
    {
      double timeReq = this->TimeValues->GetValue(this->CurrentTimeIndex);
      inputVector[0]->GetInformationObject(0)->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    }
  }
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    (this->Controller ? this->Controller->GetNumberOfProcesses() : 1));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    (this->Controller ? this->Controller->GetLocalProcessId() : 0));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCSVWriter::RequestData(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    return 1;
  }

  // is this the first request
  if (this->CurrentTimeIndex == 0 && this->WriteAllTimeSteps)
  {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  this->WriteData();

  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps)
  {
    this->CurrentTimeIndex = 0;
    if (this->WriteAllTimeSteps)
    {
      // Tell the pipeline to stop looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
    }
  }

  return this->GetErrorCode() == vtkErrorCode::NoError ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkCSVWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }
  else if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

namespace
{
//-----------------------------------------------------------------------------
template <class iterT>
void vtkCSVWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ostream& stream, vtkCSVWriter* writer, bool& first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if (!first)
    {
      stream << writer->GetFieldDelimiter();
    }
    first = false;
    if ((index + cc) < iter->GetNumberOfValues())
    {
      stream << iter->GetValue(index + cc);
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex,
  ostream& stream, vtkCSVWriter* writer, bool& first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if (!first)
    {
      stream << writer->GetFieldDelimiter();
    }
    first = false;
    if ((index + cc) < iter->GetNumberOfValues())
    {
      stream << writer->GetString(iter->GetValue(index + cc));
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<char>* iter, vtkIdType tupleIndex,
  ostream& stream, vtkCSVWriter* writer, bool& first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if (!first)
    {
      stream << writer->GetFieldDelimiter();
    }
    first = false;
    if ((index + cc) < iter->GetNumberOfValues())
    {
      stream << static_cast<int>(iter->GetValue(index + cc));
    }
  }
}

//-----------------------------------------------------------------------------
template <>
void vtkCSVWriterGetDataString(vtkArrayIteratorTemplate<unsigned char>* iter, vtkIdType tupleIndex,
  ostream& stream, vtkCSVWriter* writer, bool& first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex * numComps;
  for (int cc = 0; cc < numComps; cc++)
  {
    if ((index + cc) < iter->GetNumberOfValues())
    {
      if (!first)
      {
        stream << writer->GetFieldDelimiter();
      }
      first = false;
      stream << static_cast<int>(iter->GetValue(index + cc));
    }
    else
    {
      if (!first)
      {
        stream << writer->GetFieldDelimiter();
      }
      first = false;
    }
  }
}

} // end anonymous namespace

class vtkCSVWriter::CSVFile
{
  vtksys::ofstream Stream;
  std::vector<std::pair<std::string, int>> ColumnInfo;
  int TimeStep = -1;
  double Time = vtkMath::Nan();

public:
  CSVFile(int timeStep, double time)
    : TimeStep(timeStep)
    , Time(time)
  {
  }

  enum class OpenMode
  {
    Write,
    Append
  };

  int Open(const char* filename, OpenMode mode)
  {
    if (!filename)
    {
      return vtkErrorCode::NoFileNameError;
    }
    if (OpenMode::Write == mode)
    {
      this->Stream.open(filename, ios::out);
    }
    else // (OpenMode::Append == mode)
    {
      this->Stream.open(filename, ios::app);
    }
    if (this->Stream.fail())
    {
      return vtkErrorCode::CannotOpenFileError;
    }
    return vtkErrorCode::NoError;
  }

  void WriteHeader(vtkTable* table, vtkCSVWriter* self, OpenMode mode)
  {
    this->WriteHeader(table->GetRowData(), self, mode);
  }

  void WriteHeader(vtkDataSetAttributes* dsa, vtkCSVWriter* self, OpenMode mode)
  {
    if (OpenMode::Write == mode)
    {
      bool add_delimiter = false;
      if (this->TimeStep >= 0)
      {
        this->Stream << "TimeStep";
        add_delimiter = true;
      }
      if (!vtkMath::IsNan(this->Time))
      {
        if (add_delimiter)
        {
          // add separator for all but the very first column
          this->Stream << self->GetFieldDelimiter();
        }
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
    }
    else // (OpenMode::Append == mode)
    {
      for (int cc = 0, numArrays = dsa->GetNumberOfArrays(); cc < numArrays; ++cc)
      {
        auto array = dsa->GetAbstractArray(cc);
        const int num_comps = array->GetNumberOfComponents();

        // save order of arrays written out in header
        this->ColumnInfo.push_back(std::make_pair(std::string(array->GetName()), num_comps));
      }
    }
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
    std::vector<vtkSmartPointer<vtkArrayIterator>> columnsIters;
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
      if (this->TimeStep >= 0)
      {
        this->Stream << this->TimeStep;
        first_column = false;
      }
      if (!vtkMath::IsNan(this->Time))
      {
        if (!first_column)
        {
          this->Stream << self->GetFieldDelimiter();
        }
        // add a time column.
        this->Stream << this->Time;
        first_column = false;
      }

      for (auto& iter : columnsIters)
      {
        switch (iter->GetDataType())
        {
          vtkArrayIteratorTemplateMacro(vtkCSVWriterGetDataString(
            static_cast<VTK_TT*>(iter.GetPointer()), cc, this->Stream, self, first_column));
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
static bool SuffixValidation(char* fileNameSuffix)
{
  std::string suffix(fileNameSuffix);
  // Only allow this format: ABC%.Xd
  // ABC is an arbitrary string which may or may not exist
  // % and d must exist and d must be the last char
  // . and X may or may not exist, X must be an integer if it exists
  if (suffix.empty() || suffix[suffix.size() - 1] != 'd')
  {
    return false;
  }
  std::string::size_type lastPercentage = suffix.find_last_of('%');
  if (lastPercentage == std::string::npos)
  {
    return false;
  }
  if (suffix.size() - lastPercentage > 2 && !isdigit(suffix[lastPercentage + 1]) &&
    suffix[lastPercentage + 1] != '.')
  {
    return false;
  }
  for (std::string::size_type i = lastPercentage + 2; i < suffix.size() - 1; ++i)
  {
    if (!isdigit(suffix[i]))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::WriteData()
{
  if (!this->FileName)
  {
    return;
  }
  auto input = this->GetInput();

  // define filename
  std::ostringstream filename;
  if (this->WriteAllTimeSteps && this->WriteAllTimeStepsSeparately && this->NumberOfTimeSteps > 1)
  {
    const std::string path = vtksys::SystemTools::GetFilenamePath(this->FileName);
    const std::string filenameNoExt =
      vtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);
    const std::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
    if (this->FileNameSuffix && SuffixValidation(this->FileNameSuffix))
    {
      // Print this->CurrentTimeIndex to a string using this->FileNameSuffix as format
      char suffix[100];
      snprintf(suffix, 100, this->FileNameSuffix, this->CurrentTimeIndex);
      if (!path.empty())
      {
        filename << path << "/";
      }
      filename << filenameNoExt << suffix << extension;
    }
    else
    {
      vtkErrorMacro("Invalid file suffix:" << (this->FileNameSuffix ? this->FileNameSuffix : "null")
                                           << ". Expected valid % format specifiers!");
      return;
    }
  }
  else
  {
    filename << this->FileName;
  }

  int timeStep = -1;
  if (this->AddTimeStep && input && input->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    if (this->WriteAllTimeSteps)
    {
      timeStep = this->CurrentTimeIndex;
    }
    else
    {
      const double time = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
      for (vtkIdType i = 0, max = this->TimeValues->GetNumberOfValues(); i < max; ++i)
      {
        if (this->TimeValues->GetValue(i) == time)
        {
          timeStep = i;
          break;
        }
      }
    }
  }
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
    vtkCSVWriter::CSVFile file(timeStep, time);
    CSVFile::OpenMode openMode =
      this->WriteAllTimeSteps && !this->WriteAllTimeStepsSeparately && this->CurrentTimeIndex > 0
      ? CSVFile::OpenMode::Append
      : CSVFile::OpenMode::Write;
    int error_code = file.Open(filename.str().c_str(), openMode);
    if (error_code == vtkErrorCode::NoError)
    {
      file.WriteHeader(table, this, openMode);
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
    vtkCSVWriter::CSVFile file(timeStep, time);
    CSVFile::OpenMode openMode =
      this->WriteAllTimeSteps && !this->WriteAllTimeStepsSeparately && this->CurrentTimeIndex > 0
      ? CSVFile::OpenMode::Append
      : CSVFile::OpenMode::Write;
    int error_code = file.Open(filename.str().c_str(), openMode);
    controller->Broadcast(&error_code, 1, 0);
    if (error_code != vtkErrorCode::NoError)
    {
      this->SetErrorCode(error_code);
      return;
    }

    const vtkIdType row_count = table->GetNumberOfRows();
    std::vector<vtkIdType> global_row_counts(numRanks, 0);
    controller->Gather(&row_count, global_row_counts.data(), 1, 0);

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
    file.WriteHeader(tmp, this, openMode);

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

  // the writer can be used for multiple timesteps
  // and the array is re-created at each use.
  // except when writing multiple timesteps
  if (!this->WriteAllTimeSteps && this->TimeValues)
  {
    this->TimeValues->Delete();
    this->TimeValues = nullptr;
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
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "WriteAllTimeSteps " << (this->WriteAllTimeSteps ? "Yes" : "No") << endl;
  os << indent << "WriteAllTimeStepsSeparately "
     << (this->WriteAllTimeStepsSeparately ? "Yes" : "No") << endl;
  os << indent << "FileNameSuffix: " << (this->FileNameSuffix ? this->FileNameSuffix : "(none)")
     << endl;
  os << indent << "UseScientificNotation: " << this->UseScientificNotation << endl;
  os << indent << "Precision: " << this->Precision << endl;
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  os << indent << "AddMetaData: " << (this->AddMetaData ? "Yes" : "No") << endl;
  os << indent << "AddTimeStep: " << (this->AddTimeStep ? "Yes" : "No") << endl;
  os << indent << "AddTime: " << (this->AddTime ? "Yes" : "No") << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "CurrentTimeIndex: " << this->CurrentTimeIndex << endl;
  os << indent << "TimeValues " << (this->TimeValues ? this->TimeValues->GetName() : "(none)")
     << endl;
  if (this->Controller)
  {
    os << indent << "Controller: " << this->Controller << endl;
  }
  else
  {
    os << indent << "Controller: (none)" << endl;
  }
}
