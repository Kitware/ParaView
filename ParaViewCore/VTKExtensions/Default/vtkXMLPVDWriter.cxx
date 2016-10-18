/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPVDWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPVDWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPDataWriter.h"
#include "vtkXMLPImageDataWriter.h"
#include "vtkXMLPMultiBlockDataWriter.h"
#include "vtkXMLPPolyDataWriter.h"
#include "vtkXMLPRectilinearGridWriter.h"
#include "vtkXMLPStructuredGridWriter.h"
#include "vtkXMLPUnstructuredGridWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkXMLWriter.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

#include <string>
#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPVDWriter);

class vtkXMLPVDWriterInternals
{
public:
  std::vector<vtkSmartPointer<vtkXMLWriter> > Writers;
  std::string FilePath;
  std::string FilePrefix;
  std::vector<std::string> Entries;
  std::string CreatePieceFileName(int index);
};

//----------------------------------------------------------------------------
vtkXMLPVDWriter::vtkXMLPVDWriter()
{
  this->Internal = new vtkXMLPVDWriterInternals;
  this->Piece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteCollectionFileInitialized = 0;
  this->WriteCollectionFile = 0;

  // Setup a callback for the internal writers to report progress.
  this->ProgressObserver = vtkCallbackCommand::New();
  this->ProgressObserver->SetCallback(&vtkXMLPVDWriter::ProgressCallbackFunction);
  this->ProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
vtkXMLPVDWriter::~vtkXMLPVDWriter()
{
  this->ProgressObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "Piece: " << this->Piece << endl;
  os << indent << "WriteCollectionFile: " << this->WriteCollectionFile << endl;
}

//----------------------------------------------------------------------------
int vtkXMLPVDWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // Create writers for each input.
    this->CreateWriters();

    for (int i = 0; i < GetNumberOfInputConnections(0); ++i)
    {
      this->GetWriter(i)->ProcessRequest(request, inputVector, outputVector);
    }
    return 1;
  }
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::SetWriteCollectionFile(int flag)
{
  this->WriteCollectionFileInitialized = 1;
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WriteCollectionFile to "
                << flag);
  if (this->WriteCollectionFile != flag)
  {
    this->WriteCollectionFile = flag;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::AddInputData(vtkDataObject* input)
{
  this->AddInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
int vtkXMLPVDWriter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->SetErrorCode(vtkErrorCode::NoError);

  // Make sure we have a file to write.
  if (!this->Stream && !this->FileName)
  {
    vtkErrorMacro("Writer called with no FileName set.");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
  }

  // We are just starting to write.  Do not call
  // UpdateProgressDiscrete because we want a 0 progress callback the
  // first time.
  this->UpdateProgress(0);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = { 0, 1 };
  this->SetProgressRange(wholeProgressRange, 0, 1);

  // Prepare file prefix for creation of internal file names.
  this->SplitFileName();

  // Decide whether to write the collection file.
  int writeCollection = 0;
  if (this->WriteCollectionFileInitialized)
  {
    writeCollection = this->WriteCollectionFile;
  }
  else if (this->Piece == 0)
  {
    writeCollection = 1;
  }

  float progressRange[2] = { 0, 0 };
  this->GetProgressRange(progressRange);

  // Create the subdirectory for the internal files.
  std::string subdir = this->Internal->FilePath;
  subdir += this->Internal->FilePrefix;
  this->MakeDirectory(subdir.c_str());

  // Write each input.
  int i, j;
  this->DeleteAllEntries();
  for (i = 0; i < this->GetNumberOfInputConnections(0); ++i)
  {
    this->SetProgressRange(progressRange, i, GetNumberOfInputConnections(0) + writeCollection);
    if (vtkXMLWriter* w = this->GetWriter(i))
    {
      // Set the file name.
      std::string fname = this->Internal->CreatePieceFileName(i);
      std::string full = this->Internal->FilePath;
      full += fname;
      w->SetFileName(full.c_str());

      // Write the data.
      w->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);
      w->ProcessRequest(request, inputVector, outputVector);
      w->RemoveObserver(this->ProgressObserver);

      // Create the entry for the collection file.
      std::ostringstream entry_with_warning_C4701;
      entry_with_warning_C4701 << "<DataSet part=\"" << i << "\" file=\"" << fname.c_str() << "\"/>"
                               << ends;
      this->AppendEntry(entry_with_warning_C4701.str().c_str());

      if (w->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
        for (j = 0; j < i; j++)
        {
          fname = this->Internal->CreatePieceFileName(i);
          full = this->Internal->FilePath;
          full += fname;
          vtksys::SystemTools::RemoveFile(full.c_str());
        }
        this->RemoveADirectory(subdir.c_str());
        this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
        vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->DeleteAFile();
        return 0;
      }
    }
  }

  // Write the collection file if requested.
  if (writeCollection)
  {
    this->SetProgressRange(progressRange, this->GetNumberOfInputConnections(0),
      this->GetNumberOfInputConnections(0) + writeCollection);
    return this->WriteCollectionFileIfRequested();
  }

  // We have finished writing.
  this->UpdateProgressDiscrete(1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPVDWriter::WriteData()
{
  // Write the collection file.
  this->StartFile();
  vtkIndent indent = vtkIndent().GetNextIndent();

  // Open the primary element.
  ostream& os = *(this->Stream);
  os << indent << "<" << this->GetDataSetName() << ">\n";

  // Write the set of entries.
  for (std::vector<std::string>::const_iterator i = this->Internal->Entries.begin();
       i != this->Internal->Entries.end(); ++i)
  {
    os << indent.GetNextIndent() << i->c_str() << "\n";
  }

  // Close the primary element.
  os << indent << "</" << this->GetDataSetName() << ">\n";
  return this->EndFile();
}

//----------------------------------------------------------------------------
int vtkXMLPVDWriter::WriteCollectionFileIfRequested()
{
  // Decide whether to write the collection file.
  int writeCollection = 0;
  if (this->WriteCollectionFileInitialized)
  {
    writeCollection = this->WriteCollectionFile;
  }
  else if (this->Piece == 0)
  {
    writeCollection = 1;
  }

  if (writeCollection)
  {
    if (!this->Superclass::WriteInternal())
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::MakeDirectory(const char* name)
{
  if (!vtksys::SystemTools::MakeDirectory(name))
  {
    vtkErrorMacro(<< "Sorry unable to create directory: " << name << endl
                  << "Last systen error was: "
                  << vtksys::SystemTools::GetLastSystemError().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::RemoveADirectory(const char* name)
{
  if (!vtksys::SystemTools::RemoveADirectory(name))
  {
    vtkErrorMacro(<< "Sorry unable to remove a directory: " << name << endl
                  << "Last systen error was: "
                  << vtksys::SystemTools::GetLastSystemError().c_str());
  }
}

//----------------------------------------------------------------------------
const char* vtkXMLPVDWriter::GetDefaultFileExtension()
{
  return "pvd";
}

//----------------------------------------------------------------------------
const char* vtkXMLPVDWriter::GetDataSetName()
{
  return "Collection";
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::CreateWriters()
{
  int i;
  vtkExecutive* exec = this->GetExecutive();

  this->Internal->Writers.resize(this->GetNumberOfInputConnections(0));
  for (i = 0; i < GetNumberOfInputConnections(0); ++i)
  {
    // Create a writer based on the type of this input.

    switch (exec->GetInputData(0, i)->GetDataObjectType())
    {
      case VTK_POLY_DATA:
        if (this->NumberOfPieces > 1)
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPPolyDataWriter") != 0))
          {
            vtkXMLPPolyDataWriter* w = vtkXMLPPolyDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPolyDataWriter") != 0))
          {
            vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;
      case VTK_STRUCTURED_POINTS:
      case VTK_IMAGE_DATA:
        if (this->NumberOfPieces > 1)
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPImageDataWriter") != 0))
          {
            vtkXMLPImageDataWriter* w = vtkXMLPImageDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLImageDataWriter") != 0))
          {
            vtkXMLImageDataWriter* w = vtkXMLImageDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;
      case VTK_UNSTRUCTURED_GRID:
        if (this->NumberOfPieces > 1)
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPUnstructuredGridWriter") !=
              0))
          {
            vtkXMLPUnstructuredGridWriter* w = vtkXMLPUnstructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLUnstructuredGridWriter") !=
              0))
          {
            vtkXMLUnstructuredGridWriter* w = vtkXMLUnstructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;
      case VTK_STRUCTURED_GRID:
        if (this->NumberOfPieces > 1)
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPStructuredGridWriter") !=
              0))
          {
            vtkXMLPStructuredGridWriter* w = vtkXMLPStructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLStructuredGridWriter") != 0))
          {
            vtkXMLStructuredGridWriter* w = vtkXMLStructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;
      case VTK_RECTILINEAR_GRID:
        if (this->NumberOfPieces > 1)
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPRectilinearGridWriter") !=
              0))
          {
            vtkXMLPRectilinearGridWriter* w = vtkXMLPRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLRectilinearGridWriter") !=
              0))
          {
            vtkXMLRectilinearGridWriter* w = vtkXMLRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;

      case VTK_MULTIBLOCK_DATA_SET:
        if (!this->Internal->Writers[i].GetPointer() ||
          (strcmp(this->Internal->Writers[i]->GetClassName(), "vtkXMLPMultiBlockDataWriter") != 0))
        {
          vtkXMLPMultiBlockDataWriter* w = vtkXMLPMultiBlockDataWriter::New();
          this->Internal->Writers[i] = w;
          w->Delete();
        }
        break;
    }

    this->Internal->Writers[i]->SetInputConnection(this->GetInputConnection(0, i));

    // Copy settings to the writer.
    if (vtkXMLWriter* w = this->Internal->Writers[i].GetPointer())
    {
      w->SetDebug(this->GetDebug());
      w->SetByteOrder(this->GetByteOrder());
      w->SetCompressor(this->GetCompressor());
      w->SetBlockSize(this->GetBlockSize());
      w->SetDataMode(this->GetDataMode());
      w->SetEncodeAppendedData(this->GetEncodeAppendedData());
      w->SetHeaderType(this->GetHeaderType());
    }

    // If this is a parallel writer, set the piece information.
    if (vtkXMLPDataWriter* w =
          vtkXMLPDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
    {
      w->SetStartPiece(this->Piece);
      w->SetEndPiece(this->Piece);
      w->SetNumberOfPieces(this->NumberOfPieces);
      w->SetGhostLevel(this->GhostLevel);
      if (this->WriteCollectionFileInitialized)
      {
        w->SetWriteSummaryFile(this->WriteCollectionFile);
      }
      else
      {
        // We tell all piece writers to write summary file. The vtkXMLPDataWriter correctly
        // decides to write out the file only on rank 0.
        w->SetWriteSummaryFile(1);
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPVDWriter::GetWriter(int index)
{
  int size = static_cast<int>(this->Internal->Writers.size());
  if (index >= 0 && index < size)
  {
    return this->Internal->Writers[index].GetPointer();
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::SplitFileName()
{
  std::string fileName = this->FileName;
  std::string name;

  // Split the file name and extension from the path.
  std::string::size_type pos = fileName.find_last_of("/\\");
  if (pos != fileName.npos)
  {
    // Keep the slash in the file path.
    this->Internal->FilePath = fileName.substr(0, pos + 1);
    name = fileName.substr(pos + 1);
  }
  else
  {
    this->Internal->FilePath = "./";
    name = fileName;
  }

  // Split the extension from the file name.
  pos = name.find_last_of(".");
  if (pos != name.npos)
  {
    this->Internal->FilePrefix = name.substr(0, pos);
  }
  else
  {
    this->Internal->FilePrefix = name;

    // Since a subdirectory is used to store the files, we need to
    // change its name if there is no file extension.
    this->Internal->FilePrefix += "_data";
  }
}

//----------------------------------------------------------------------------
const char* vtkXMLPVDWriter::GetFilePrefix()
{
  return this->Internal->FilePrefix.c_str();
}

//----------------------------------------------------------------------------
const char* vtkXMLPVDWriter::GetFilePath()
{
  return this->Internal->FilePath.c_str();
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::ProgressCallbackFunction(
  vtkObject* caller, unsigned long, void* clientdata, void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if (w)
  {
    reinterpret_cast<vtkXMLPVDWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::ProgressCallback(vtkAlgorithm* w)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    w->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::AppendEntry(const char* entry)
{
  this->Internal->Entries.push_back(entry);
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::DeleteAllEntries()
{
  this->Internal->Entries.clear();
}

//----------------------------------------------------------------------------
std::string vtkXMLPVDWriterInternals::CreatePieceFileName(int index)
{
  std::string fname;
  std::ostringstream fn_with_warning_C4701;
  fn_with_warning_C4701 << this->FilePrefix.c_str() << "/" << this->FilePrefix.c_str() << "_"
                        << index << "." << this->Writers[index]->GetDefaultFileExtension() << ends;
  fname = fn_with_warning_C4701.str();
  return fname;
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  int size = static_cast<int>(this->Internal->Writers.size());
  for (int i = 0; i < size; ++i)
  {
    vtkGarbageCollectorReport(collector, this->Internal->Writers[i], "Writer");
  }
}

int vtkXMLPVDWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
