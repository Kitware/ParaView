/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPVAnimationWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPVAnimationWriter.h"

#include "vtkCompleteArrays.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLWriter.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPVAnimationWriter);

//----------------------------------------------------------------------------
class vtkXMLPVAnimationWriterInternals
{
public:
  // The name of the group to which each input belongs.
  typedef std::vector<std::string> InputGroupNamesType;
  InputGroupNamesType InputGroupNames;

  // The part number each input has been assigned in its group.
  typedef std::vector<int> InputPartNumbersType;
  InputPartNumbersType InputPartNumbers;

  // The modified time when each input was last written in a previous
  // animation step.
  typedef std::vector<vtkMTimeType> InputMTimesType;
  InputMTimesType InputMTimes;

  // The number of times each input has changed during this animation
  // sequence.
  typedef std::vector<int> InputChangeCountsType;
  InputChangeCountsType InputChangeCounts;

  // Count the number of parts in each group.
  typedef std::map<std::string, int> GroupMapType;
  GroupMapType GroupMap;

  // Create the file name for the given input during this animation
  // step.
  std::string CreateFileName(int index, const char* prefix, const char* ext);
};

//----------------------------------------------------------------------------
vtkXMLPVAnimationWriter::vtkXMLPVAnimationWriter()
{
  this->Internal = new vtkXMLPVAnimationWriterInternals;
  this->StartCalled = 0;
  this->FinishCalled = 0;
  this->FileNamesCreated = 0;
  this->NumberOfFileNamesCreated = 0;

  vtkMultiProcessController* globalController = vtkMultiProcessController::GetGlobalController();
  if (globalController)
  {
    this->SetNumberOfPieces(globalController->GetNumberOfProcesses());
    this->SetPiece(globalController->GetLocalProcessId());
  }
}

//----------------------------------------------------------------------------
vtkXMLPVAnimationWriter::~vtkXMLPVAnimationWriter()
{
  delete this->Internal;
  this->DeleteFileNames();
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->GetNumberOfInputConnections(0) > 0)
  {
    os << indent << "Input Detail:\n";
    vtkIndent nextIndent = indent.GetNextIndent();
    int i;
    for (i = 0; i < this->GetNumberOfInputConnections(0); ++i)
    {
      os << nextIndent << i << ": group \"" << this->Internal->InputGroupNames[i].c_str()
         << "\" part " << this->Internal->InputPartNumbers[i] << "\n";
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::AddInputInternal(const char* group)
{
  // Find the part number for this input.
  int partNum = 0;
  vtkXMLPVAnimationWriterInternals::GroupMapType::iterator s = this->Internal->GroupMap.find(group);
  if (s != this->Internal->GroupMap.end())
  {
    partNum = s->second++;
  }
  else
  {
    vtkXMLPVAnimationWriterInternals::GroupMapType::value_type v(group, 1);
    this->Internal->GroupMap.insert(v);
  }
  this->Internal->InputPartNumbers.push_back(partNum);

  // Add the group name for this input.
  this->Internal->InputGroupNames.push_back(group);

  // Allocate the mtime table entry for this input.
  this->Internal->InputMTimes.push_back(0);

  // Allocate the change count entry for this input.
  this->Internal->InputChangeCounts.push_back(0);
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::AddRepresentation(vtkAlgorithm* repr, const char* groupname)
{
  vtkPVDataRepresentation* pvrepr = vtkPVDataRepresentation::SafeDownCast(repr);
  if (repr)
  {
    vtkCompleteArrays* complete_arrays = vtkCompleteArrays::New();
    complete_arrays->SetInputData(pvrepr->GetRenderedDataObject(0));
    this->AddInputConnection(complete_arrays->GetOutputPort());
    this->AddInputInternal(groupname);
    complete_arrays->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::RemoveAllRepresentations()
{
  this->RemoveAllInputs();
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::Start()
{
  // Do not allow double-start.
  if (this->StartCalled)
  {
    vtkErrorMacro("Cannot call Start() twice before calling Finish().");
    return;
  }

  // Make sure we have a file name.
  if (!this->FileName || !this->FileName[0])
  {
    vtkErrorMacro("No FileName has been set.");
    return;
  }

  // Initialize input change tables.
  int i;
  for (i = 0; i < this->GetNumberOfInputConnections(0); ++i)
  {
    this->Internal->InputMTimes[i] = 0;
    this->Internal->InputChangeCounts[i] = 0;
  }

  // Clear the animation entries from any previous run.
  this->DeleteAllEntries();

  // Clear the file names from any previous run.
  this->DeleteFileNames();

  // Split the file name into a directory and file prefix.
  this->SplitFileName();

  // Create a writer for each input.
  this->CreateWriters();

  // Create the subdirectory for the internal files.
  std::string subdir = this->GetFilePath();
  subdir += this->GetFilePrefix();
  this->MakeDirectory(subdir.c_str());

  this->StartCalled = 1;
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::WriteTime(double time)
{
  if (!this->StartCalled)
  {
    vtkErrorMacro("Must call Start() before WriteTime().");
    return;
  }

  // Consider every input.
  int i;
  vtkExecutive* exec = this->GetExecutive();

  for (i = 0; i < this->GetNumberOfInputConnections(0); ++i)
  {
    vtkDataObject* dataObject = exec->GetInputData(0, i);
    // Make sure the pipeline mtime is up to date.
    exec->UpdateInformation();

    // If the input has been modified since the last animation step,
    // increment its file number.
    int changed = 0;

    if (vtkStreamingDemandDrivenPipeline::SafeDownCast(
          this->GetInputAlgorithm(0, i)->GetExecutive())
          ->GetPipelineMTime() > this->Internal->InputMTimes[i])
    {
      this->Internal->InputMTimes[i] = vtkStreamingDemandDrivenPipeline::SafeDownCast(
                                         this->GetInputAlgorithm(0, i)->GetExecutive())
                                         ->GetPipelineMTime();
      changed = 1;
    }

    if (dataObject->GetInformation()->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      changed = 1;
    }

    if (changed)
    {
      this->Internal->InputChangeCounts[i] += 1;
    }

    // Create this animation entry.
    vtkXMLWriter* writer = this->GetWriter(i);
    std::string fname =
      this->Internal->CreateFileName(i, this->GetFilePrefix(), writer->GetDefaultFileExtension());
    std::ostringstream entry_with_warning_C4701;
    entry_with_warning_C4701 << "<DataSet timestep=\"" << time << "\" group=\""
                             << this->Internal->InputGroupNames[i].c_str() << "\" part=\""
                             << this->Internal->InputPartNumbers[i] << "\" file=\"" << fname.c_str()
                             << "\"/>" << ends;
    this->AppendEntry(entry_with_warning_C4701.str().c_str());

    // Write this step's file if its input has changed.
    if (changed)
    {
      std::string fullName = this->GetFilePath();
      fullName += fname;
      writer->SetFileName(fullName.c_str());
      this->AddFileName(fullName.c_str());
      writer->Write();
      if (writer->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        break;
      }
    }
  }

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeleteFiles();
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVAnimationWriter::Finish()
{
  if (!this->StartCalled)
  {
    vtkErrorMacro("Must call Start() before Finish().");
    return;
  }

  this->StartCalled = 0;
  this->FinishCalled = 1;

  // Just write the output file with the current set of entries.
  this->WriteInternal();

  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    this->DeleteFiles();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPVAnimationWriter::WriteInternal()
{
  if (!this->FinishCalled)
  {
    vtkErrorMacro("Do not call Write() directly.  Call Finish() instead.");
    return 0;
  }

  this->FinishCalled = 0;

  // Write the animation file.
  return this->WriteCollectionFileIfRequested();
}

//----------------------------------------------------------------------------
std::string vtkXMLPVAnimationWriterInternals::CreateFileName(
  int index, const char* prefix, const char* ext)
{
  // Start with the directory and file name prefix.
  std::ostringstream fn_with_warning_C4701;
  fn_with_warning_C4701 << prefix << "/" << prefix << "_";

  // Add the group name.
  fn_with_warning_C4701 << this->InputGroupNames[index].c_str();

  // Construct the part/time portion.  Add a part number if there is
  // more than one part in this group.
  char pt[100];
  if (this->GroupMap[this->InputGroupNames[index]] > 1)
  {
    sprintf(pt, "P%02dT%04d", this->InputPartNumbers[index], this->InputChangeCounts[index] - 1);
  }
  else
  {
    sprintf(pt, "T%04d", this->InputChangeCounts[index] - 1);
  }
  fn_with_warning_C4701 << pt;

  // Add the file extension.
  fn_with_warning_C4701 << "." << ext << ends;

  // Return the result.
  std::string fname = fn_with_warning_C4701.str();
  return fname;
}

void vtkXMLPVAnimationWriter::AddFileName(const char* fileName)
{
  int size = this->NumberOfFileNamesCreated;
  char** newFileNameList = new char*[size];

  int i;
  for (i = 0; i < size; i++)
  {
    newFileNameList[i] = new char[strlen(this->FileNamesCreated[i]) + 1];
    strcpy(newFileNameList[i], this->FileNamesCreated[i]);
    delete[] this->FileNamesCreated[i];
  }
  delete[] this->FileNamesCreated;

  this->FileNamesCreated = new char*[size + 1];

  for (i = 0; i < size; i++)
  {
    this->FileNamesCreated[i] = new char[strlen(newFileNameList[i]) + 1];
    strcpy(this->FileNamesCreated[i], newFileNameList[i]);
    delete[] newFileNameList[i];
  }
  delete[] newFileNameList;

  this->FileNamesCreated[size] = new char[strlen(fileName) + 1];
  strcpy(this->FileNamesCreated[size], fileName);
  this->NumberOfFileNamesCreated++;
}

void vtkXMLPVAnimationWriter::DeleteFileNames()
{
  int i;
  if (this->FileNamesCreated)
  {
    for (i = 0; i < this->NumberOfFileNamesCreated; i++)
    {
      delete[] this->FileNamesCreated[i];
    }
    delete[] this->FileNamesCreated;
    this->FileNamesCreated = 0;
  }
  this->NumberOfFileNamesCreated = 0;
}

void vtkXMLPVAnimationWriter::DeleteFiles()
{
  for (int i = 0; i < this->NumberOfFileNamesCreated; i++)
  {
    this->DeleteAFile(this->FileNamesCreated[i]);
  }
  this->DeleteAFile(this->FileName);
  std::string subdir = this->GetFilePath();
  subdir += this->GetFilePrefix();
  this->RemoveADirectory(subdir.c_str());
}
