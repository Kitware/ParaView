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
#include "vtkXMLHyperTreeGridWriter.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLPDataWriter.h"
#include "vtkXMLPImageDataWriter.h"
#include "vtkXMLPMultiBlockDataWriter.h"
#include "vtkXMLPPolyDataWriter.h"
#include "vtkXMLPRectilinearGridWriter.h"
#include "vtkXMLPStructuredGridWriter.h"
#include "vtkXMLPTableWriter.h"
#include "vtkXMLPUnstructuredGridWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkXMLTableWriter.h"
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
  std::string CreatePieceFileName(int index, bool addTimeIndex, int currentTimeIndex);
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
  this->CurrentTimeIndex = 0;
  this->WriteAllTimeSteps = 0;
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
  os << indent << "WriteAllTimeSteps: " << this->WriteAllTimeSteps << endl;
  os << indent << "WriteCollectionFile: " << this->WriteCollectionFile << endl;
}

//----------------------------------------------------------------------------
int vtkXMLPVDWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    // Create writers for each input first
    this->CreateWriters();

    for (int i = 0; i < GetNumberOfInputConnections(0); ++i)
    {
      if (this->GetWriter(i))
      {
        this->GetWriter(i)->ProcessRequest(request, inputVector, outputVector);
      }
    }
    // call the typical RequestUpdateExtent method
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }
  else if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->DeleteAllEntries();
    this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkXMLPVDWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->WriteAllTimeSteps && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* timeSteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double timeReq = timeSteps[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }
  return 1;
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

  double timeIndexValue = 0;
  bool includeTimeIndexValue = false;
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (this->WriteAllTimeSteps && inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* timeSteps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    timeIndexValue = timeSteps[this->CurrentTimeIndex];
    includeTimeIndexValue = true;
  }

  // Write each input.
  for (int i = 0; i < this->GetNumberOfInputConnections(0); ++i)
  {
    this->SetProgressRange(progressRange, i, GetNumberOfInputConnections(0) + writeCollection);
    if (vtkXMLWriter* w = this->GetWriter(i))
    {
      // Set the file name.
      std::string fname = this->Internal->CreatePieceFileName(
        i, (this->WriteAllTimeSteps != 0 && this->NumberOfTimeSteps > 1), this->CurrentTimeIndex);
      std::string full = this->Internal->FilePath;
      full += fname;
      w->SetFileName(full.c_str());

      // Write the data.
      w->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);
      w->ProcessRequest(request, inputVector, outputVector);
      w->RemoveObserver(this->ProgressObserver);

      // Create the entry for the collection file.
      std::ostringstream entry_with_warning_C4701;
      entry_with_warning_C4701 << "<DataSet";
      if (includeTimeIndexValue)
      {
        entry_with_warning_C4701 << " timestep=\"" << timeIndexValue << "\"";
      }
      entry_with_warning_C4701 << " part=\"" << i << "\" file=\"" << fname.c_str() << "\"/>"
                               << ends;
      this->AppendEntry(entry_with_warning_C4701.str().c_str());

      if (w->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
      {
        for (int j = 0; j < i; j++)
        {
          fname = this->Internal->CreatePieceFileName(j,
            (this->WriteAllTimeSteps != 0 && this->NumberOfTimeSteps > 1), this->CurrentTimeIndex);
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

  // We have finished writing.
  this->UpdateProgressDiscrete(1);
  int retVal = 1;

  // we need to set the CONTINUE_EXECUTING request here since the
  // piece writer ProcessRequest() removes the CONTINUE_EXECUTING flag
  if (this->WriteAllTimeSteps && this->NumberOfTimeSteps > 1)
  {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps || this->WriteAllTimeSteps == 0)
  {
    // Write the collection file if requested.
    if (writeCollection)
    {
      this->SetProgressRange(progressRange, this->GetNumberOfInputConnections(0),
        this->GetNumberOfInputConnections(0) + writeCollection);
      retVal = this->WriteCollectionFileIfRequested();
    }

    this->CurrentTimeIndex = 0;
    if (this->WriteAllTimeSteps && this->NumberOfTimeSteps > 1)
    {
      // Tell the pipeline to stop looping.
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
    }
  }

  return retVal;
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
                  << "Last system error was: "
                  << vtksys::SystemTools::GetLastSystemError().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkXMLPVDWriter::RemoveADirectory(const char* name)
{
  if (!vtksys::SystemTools::RemoveADirectory(name))
  {
    vtkErrorMacro(<< "Sorry unable to remove a directory: " << name << endl
                  << "Last system error was: "
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
            (this->Internal->Writers[i]->IsA("vtkXMLPPolyDataWriter")))
          {
            vtkXMLPPolyDataWriter* w = vtkXMLPPolyDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLPolyDataWriter")))
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
            (this->Internal->Writers[i]->IsA("vtkXMLPImageDataWriter")))
          {
            vtkXMLPImageDataWriter* w = vtkXMLPImageDataWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLImageDataWriter")))
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
            (this->Internal->Writers[i]->IsA("vtkXMLPUnstructuredGridWriter")))
          {
            vtkXMLPUnstructuredGridWriter* w = vtkXMLPUnstructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLUnstructuredGridWriter")))
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
            (this->Internal->Writers[i]->IsA("vtkXMLPStructuredGridWriter")))
          {
            vtkXMLPStructuredGridWriter* w = vtkXMLPStructuredGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLStructuredGridWriter")))
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
            (this->Internal->Writers[i]->IsA("vtkXMLPRectilinearGridWriter")))
          {
            vtkXMLPRectilinearGridWriter* w = vtkXMLPRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLRectilinearGridWriter")))
          {
            vtkXMLRectilinearGridWriter* w = vtkXMLRectilinearGridWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;

      case VTK_MULTIBLOCK_DATA_SET:
        if (!this->Internal->Writers[i].GetPointer() ||
          (this->Internal->Writers[i]->IsA("vtkXMLPMultiBlockDataWriter")))
        {
          vtkXMLPMultiBlockDataWriter* w = vtkXMLPMultiBlockDataWriter::New();
          this->Internal->Writers[i] = w;
          w->Delete();
        }
        break;

      case VTK_TABLE:
        if (this->NumberOfPieces > 1)
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLPTableWriter")))
          {
            vtkXMLPTableWriter* w = vtkXMLPTableWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        else
        {
          if (!this->Internal->Writers[i].GetPointer() ||
            (this->Internal->Writers[i]->IsA("vtkXMLTableWriter")))
          {
            vtkXMLTableWriter* w = vtkXMLTableWriter::New();
            this->Internal->Writers[i] = w;
            w->Delete();
          }
        }
        break;

      case VTK_HYPER_TREE_GRID:
        if (!this->Internal->Writers[i].GetPointer() ||
          (this->Internal->Writers[i]->IsA("vtkXMLHyperTreeGridWriter")))
        {
          vtkXMLHyperTreeGridWriter* w = vtkXMLHyperTreeGridWriter::New();
          this->Internal->Writers[i] = w;
          w->Delete();
        }
        break;

      default:
        vtkErrorMacro("Unsupported data type: " << exec->GetInputData(0, i)->GetClassName());
        return;
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
    if (vtkXMLPDataWriter* pDataWriter =
          vtkXMLPDataWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
    {
      pDataWriter->SetStartPiece(this->Piece);
      pDataWriter->SetEndPiece(this->Piece);
      pDataWriter->SetNumberOfPieces(this->NumberOfPieces);
      pDataWriter->SetGhostLevel(this->GhostLevel);
      if (this->WriteCollectionFileInitialized)
      {
        pDataWriter->SetWriteSummaryFile(this->WriteCollectionFile);
      }
      else
      {
        // We tell all piece writers to write summary file. The vtkXMLPDataWriter correctly
        // decides to write out the file only on rank 0.
        pDataWriter->SetWriteSummaryFile(1);
      }
    }
    else if (vtkXMLPTableWriter* pTableWriter =
               vtkXMLPTableWriter::SafeDownCast(this->Internal->Writers[i].GetPointer()))
    {
      pTableWriter->SetStartPiece(this->Piece);
      pTableWriter->SetEndPiece(this->Piece);
      pTableWriter->SetNumberOfPieces(this->NumberOfPieces);
      pTableWriter->SetGhostLevel(this->GhostLevel);
      if (this->WriteCollectionFileInitialized)
      {
        pTableWriter->SetWriteSummaryFile(this->WriteCollectionFile);
      }
      else
      {
        // We tell all piece writers to write summary file. The vtkXMLPDataWriter correctly
        // decides to write out the file only on rank 0.
        pTableWriter->SetWriteSummaryFile(1);
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
  return nullptr;
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
  pos = name.find_last_of('.');
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
std::string vtkXMLPVDWriterInternals::CreatePieceFileName(
  int index, bool addTimeIndex, int currentTimeIndex)
{
  std::string fname;
  std::ostringstream fn_with_warning_C4701;
  fn_with_warning_C4701 << this->FilePrefix.c_str() << "/" << this->FilePrefix.c_str() << "_"
                        << index;
  if (addTimeIndex)
  {
    fn_with_warning_C4701 << "_" << currentTimeIndex;
  }
  fn_with_warning_C4701 << "." << this->Writers[index]->GetDefaultFileExtension() << ends;
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
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}
