#include "vtkEnsembleDataReader.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationVector.h"

#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtkVariant.h"
#include "vtkStdString.h"

#include "vtkDelimitedTextReader.h"

#include <vector>
#include <cassert>
#include <algorithm>
#include <functional>
#include <ctype.h>

struct vtkEnsembleDataReaderInternal
{
  vtkEnsembleDataReaderInternal() : LastRecordedFileName(0) {}
  ~vtkEnsembleDataReaderInternal() { delete LastRecordedFileName; }

  void RecordFileName(const char *fileName)
  {
    delete LastRecordedFileName;
    size_t length = strlen(fileName);
    LastRecordedFileName = new char[length + 1];
    strncpy(LastRecordedFileName, fileName, length);
  }

  std::vector<vtkSmartPointer<vtkAlgorithm> > Readers;
  std::vector<vtkStdString> FilePaths;
  vtkSmartPointer<vtkTable> MetaData;
  char *LastRecordedFileName;
};

vtkStandardNewMacro(vtkEnsembleDataReader);

vtkEnsembleDataReader::vtkEnsembleDataReader()
  : FileName(0)
  , CurrentMember(0)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internal = new vtkEnsembleDataReaderInternal;
}

vtkEnsembleDataReader::~vtkEnsembleDataReader()
{
  delete this->Internal;
}

int vtkEnsembleDataReader::GetNumberOfMembers() const
{
  assert(this->Internal->Readers.size() ==
         this->Internal->FilePaths.size()); // Sanity check
  return static_cast<int>(this->Internal->Readers.size());
}

vtkStdString vtkEnsembleDataReader::GetFilePath(const int member) const
{
  int count = static_cast<int>(this->Internal->FilePaths.size());
  if (member < 0 || member >= count)
    {
      //vtkWarningMacro("Requested member outside of range.");
    return "";
    }
  return this->Internal->FilePaths[member];
}

bool vtkEnsembleDataReader::SetReader(const int rowIndex, vtkAlgorithm *reader)
{
  int count = static_cast<int>(this->Internal->Readers.size());
  if (rowIndex < 0 || rowIndex >= count)
    {
    vtkWarningMacro("Tried to set out of range reader.");
    return false;
    }
  this->Internal->Readers[rowIndex] = reader;
  this->Modified();
  return true;
}

int vtkEnsembleDataReader::ProcessRequest(vtkInformation *request,
                                          vtkInformationVector **inputVector,
                                          vtkInformationVector *outputVector)
{
  char *fileName = this->GetFileName();
  if (fileName == NULL)
    {
    vtkWarningMacro(<< "FileName must be set");
    return 0;
    }

  // Read meta data if it hasn't already been read or if FileName has changed
  if (!this->Internal->MetaData ||
      (this->Internal->LastRecordedFileName &&
      strcmp(fileName, this->Internal->LastRecordedFileName) != 0))
    {
    // Read the meta data
    if (!this->ReadMetaData())
      {
      return 0;
      }

    // Update the recorded file name
    this->Internal->RecordFileName(fileName);
    }

  vtkAlgorithm *currentReader = this->GetCurrentReader();
  if (currentReader)
    {
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
      {
      currentReader->UpdateDataObject();
      vtkDataObject *rOutput = currentReader->GetOutputDataObject(0);
      if (rOutput)
        {
          vtkDataObject *output = rOutput->NewInstance();
          outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
                                                     output);
          output->Delete();
        }
      return 1;
      }
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
      {
      // Just keep metadata internal
      //if (this->Internal->MetaData)
      //{
      //outputVector->GetInformationObject(0)->Set(META_DATA(), this->MetaData);
      //}

      // Call RequestInformation on all readers as they may initialize data
      // structures there. Note that this has to be done here because current
      // reader can be changed with a pipeline request which does not cause
      // REQUEST_INFORMATION to happen again.
      std::vector<vtkSmartPointer<vtkAlgorithm> >::iterator iter =
        this->Internal->Readers.begin();
      std::vector<vtkSmartPointer<vtkAlgorithm> >::iterator end =
        this->Internal->Readers.end();
      for (; iter != end; ++iter)
        {
        int result = (*iter)->ProcessRequest(request, inputVector, outputVector);
        if (!result)
          {
          return result;
          }
        }
      return 1;
      }
    return currentReader->ProcessRequest(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkEnsembleDataReader::FillOutputPortInformation(int, vtkInformation *info)
{
  assert(info);
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

// string trimmin'
static void ltrim(std::string &s)
{
  s.erase(s.begin(),
          std::find_if(s.begin(),
            s.end(),
            std::not1(std::ptr_fun<int, int>(isspace))));
}
static void rtrim(std::string &s)
{
  s.erase(std::find_if(s.rbegin(),
                       s.rend(),
                       std::not1(std::ptr_fun<int, int>(isspace))).base(),
          s.end());
}
static void trim(std::string &s)
{
  ltrim(s);
  rtrim(s);
}

vtkAlgorithm *vtkEnsembleDataReader::GetCurrentReader()
{
  if (this->CurrentMember < 0 ||
      this->CurrentMember >= this->GetNumberOfMembers())
    {
    return 0;
    }
  return this->Internal->Readers[this->CurrentMember];
}

bool vtkEnsembleDataReader::ReadMetaData()
{
  if (!this->FileName)
    {
    return false;
    }

  // Read CSV -- Just pick sensible defaults for now
  vtkNew<vtkDelimitedTextReader> csvReader;
  assert(csvReader.GetPointer());
  csvReader->SetFieldDelimiterCharacters(",");
  csvReader->SetHaveHeaders(false);
  csvReader->SetDetectNumericColumns(true);
  csvReader->SetFileName(FileName);
  csvReader->Update();

  // Initialize a reader for each row in the CSV
  vtkTable *table = csvReader->GetOutput();
  assert(table);
  this->Internal->MetaData = table;

  // Clear then resize the internal containers
  this->Internal->FilePaths.clear();
  this->Internal->Readers.clear();
  vtkIdType rowCount = table->GetNumberOfRows();
  this->Internal->FilePaths.resize(rowCount);
  this->Internal->Readers.resize(rowCount, NULL);

  // Read the file names
  for (int r = 0; r < rowCount; ++r)
    {
    vtkVariantArray *row = table->GetRow(r);
    assert(row);
    vtkIdType valueCount = row->GetNumberOfValues();
    for (int c = 0; c < valueCount; ++c)
      {
      vtkVariant variant = row->GetValue(c);

      // For now, the last column contains a file path.
      if (c == valueCount - 1)
        {
        vtkStdString filePath = variant.ToString();
        trim(filePath); // fileName may have leading & trailing whitespace

        // Internal file paths should be relative to the .pve file
        vtkStdString pvePath = vtkStdString(FileName);
        size_t lastSlash = pvePath.find_last_of('/');
        if (lastSlash == std::string::npos)
        {
          // Windows
          lastSlash = pvePath.find_last_of('\\');
        }
        if (lastSlash == std::string::npos)
          {
          this->Internal->FilePaths[r] = filePath;
          }
        else
          {
          vtkStdString directory = pvePath.substr(0, lastSlash + 1);
          this->Internal->FilePaths[r] = directory + filePath;
          }
        }
      }
    }
  return true;
}

void vtkEnsembleDataReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // File name
  os << indent << "FileName: ";
  if (this->FileName)
    {
    os << this->FileName << endl;
    }
  else
    {
    os << "(NULL)" << endl;
    }

  // Current member
  os << indent << "Current member: " << this->CurrentMember << endl;

  // Meta data
  if (this->Internal->MetaData)
    {
    this->Internal->MetaData->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "(NULL)" << endl;
    }
}
