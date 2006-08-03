/*=========================================================================

  Program:   ParaView
  Module:    vtkPVWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVWriter.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkKWMessageDialog.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVApplication.h"
#include "vtkPVReaderModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkSMPart.h"
#include <vtkstd/string>
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVWriter);
vtkCxxRevisionMacro(vtkPVWriter, "1.32");

//----------------------------------------------------------------------------
vtkPVWriter::vtkPVWriter()
{
  this->InputClassName = 0;
  this->WriterClassName = 0;
  this->Description = 0;
  this->Parallel = 0;
  this->DataModeMethod = 0;
  this->Extensions = vtkVector<const char*>::New();
  this->Iterator = this->Extensions->NewIterator();
  this->SupportsTime = 0;
}

//----------------------------------------------------------------------------
vtkPVWriter::~vtkPVWriter()
{
  this->SetInputClassName(0);
  this->SetWriterClassName(0);
  this->SetDescription(0);
  this->SetDataModeMethod(0);
  this->Extensions->Delete();
  this->Iterator->Delete();
}

//----------------------------------------------------------------------------
void vtkPVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputClassName: " 
     << (this->InputClassName?this->InputClassName:"(none)") << endl;
  os << indent << "WriterClassName: " 
     << (this->WriterClassName?this->WriterClassName:"(none)") << endl;
  os << indent << "Description: " 
     << (this->Description?this->Description:"(none)") << endl;
  os << indent << "Parallel: " << this->Parallel << endl;
  os << indent << "DataModeMethod: " 
     << (this->DataModeMethod?this->DataModeMethod:"(none)") << endl;
  os << indent << "SupportsTime: " << this->SupportsTime
     << endl;
}

//----------------------------------------------------------------------------
int vtkPVWriter::CanWriteData(vtkDataObject* data, int parallel, int numParts)
{
  if (data == NULL)
    {
    return 0;
    }
  if (parallel && !this->Parallel)
    {
    return 0;
    }
  return ((numParts == 1) && data->IsA(this->InputClassName));
}

//----------------------------------------------------------------------------
int vtkPVWriter::CanWriteFile(const char* fname)
{
  char* ext = this->ExtractExtension(fname);
  int matches = 0;

  // Check if the file name matches any of our extensions.
  for(this->Iterator->GoToFirstItem();
      !this->Iterator->IsDoneWithTraversal() && !matches;
      this->Iterator->GoToNextItem())
    {
    const char* val = 0;
    this->Iterator->GetData(val);
    if(ext && strcmp(ext, val) == 0)
      {
      matches = 1;
      }
    }
  delete[] ext;
  return matches;
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVWriter::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
void vtkPVWriter::Write(const char* fileName, vtkPVSource* pvs,
                        int numProcs, int ghostLevel, int timeSeries)
{
  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(pvs);

  if( rm && timeSeries )
    {
    vtkstd::string name = fileName;
    vtkstd::string::size_type pos = name.find_last_of(".");
    vtkstd::string base = name.substr(0, pos);
    vtkstd::string ext = name.substr(pos);
    int n = rm->GetNumberOfTimeSteps();
    char buf[100];
    for(int i=0; i < n; ++i)
      {
      sprintf(buf, "T%03d", i);
      name = base;
      name += buf;
      name += ext;
      rm->SetRequestedTimeStep(i);
      if (!this->WriteOneFile(name.c_str(), pvs, numProcs, ghostLevel))
        {
        return;
        }
      }
    }
  else
    {
    this->WriteOneFile(fileName, pvs, numProcs, ghostLevel);
    }
}

//----------------------------------------------------------------------------
int vtkPVWriter::WriteOneFile(const char* fileName, vtkPVSource* pvs,
                              int numProcs, int ghostLevel)
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerID portID = pvs->GetPart()->GetAlgorithmOutputID();
  int success = 1;

  // Create the writer and configure it.
  vtkClientServerStream stream;
  vtkClientServerID writerID = 
    pm->NewStreamObject(this->WriterClassName, stream);
  stream << vtkClientServerStream::Invoke
         << writerID << "SetFileName" << fileName
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << writerID << "SetInputConnection" << portID
         << vtkClientServerStream::End;
  if (this->DataModeMethod)
    {
    stream << vtkClientServerStream::Invoke
           << writerID << this->DataModeMethod
           << vtkClientServerStream::End;
    }

  if(this->Parallel)
    {
    if (numProcs > 1)
      {
      vtkClientServerID ca_id = 
        pm->NewStreamObject("vtkCompleteArrays", stream);

      stream << vtkClientServerStream::Invoke
             << ca_id << "SetInputConnection" << portID
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << ca_id << "GetOutputPort" << 0
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << writerID 
             << "SetInputConnection" 
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      pm->DeleteStreamObject(ca_id, stream);
      }
    stream << vtkClientServerStream::Invoke
           << writerID << "SetGhostLevel" << ghostLevel
           << vtkClientServerStream::End;
    if (strstr(this->WriterClassName, "XMLP"))
      {
      stream << vtkClientServerStream::Invoke
             << writerID << "SetNumberOfPieces" << numProcs
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "GetPartitionId"
             << vtkClientServerStream::End
             << vtkClientServerStream::Invoke
             << writerID << "SetStartPiece" << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "GetPartitionId"
             << vtkClientServerStream::End
             << vtkClientServerStream::Invoke
             << writerID << "SetEndPiece" << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
  
      // Tell each process's writer whether it should write the summary
      // file.  This assumes that the writer is a vtkXMLWriter.  When we
      // add more writers, we will need a separate writer module.
      vtkClientServerID helperID = 
        pm->NewStreamObject("vtkPVSummaryHelper", stream);
      stream << vtkClientServerStream::Invoke
             << helperID << "SetWriter" << writerID
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "GetController"
             << vtkClientServerStream::End
             << vtkClientServerStream::Invoke
             << helperID << "SetController" << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << helperID << "SynchronizeSummaryFiles"
             << vtkClientServerStream::End;
      pm->DeleteStreamObject(helperID, stream);
      }
    }

  // Write the data.
  stream << vtkClientServerStream::Invoke
         << writerID << "Write"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << writerID << "GetErrorCode"
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),  
    vtkProcessModule::DATA_SERVER, stream);
  int retVal;
  if(pm->GetLastResult(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &retVal) &&
    retVal == vtkErrorCode::OutOfDiskSpaceError)
    {
    vtkKWMessageDialog::PopupMessage(
      pvApp, pvApp->GetMainWindow(),
      "Write Error", "There is insufficient disk space to save this data. "
      "The file(s) already written will be deleted.");
    success = 0;
    }

  // Cleanup.
  pm->DeleteStreamObject(writerID, stream);
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),  
    vtkProcessModule::DATA_SERVER, stream);
  return success;
}

//----------------------------------------------------------------------------
void vtkPVWriter::AddExtension(const char* ext)
{
  this->Extensions->AppendItem(ext);
}

//----------------------------------------------------------------------------
vtkIdType vtkPVWriter::GetNumberOfExtensions()
{
  return this->Extensions->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkPVWriter::GetExtension(vtkIdType i)
{
  const char* result = 0;
  if(this->Extensions->GetItem(i, result) != VTK_OK) { result = 0; }
  return result;
}

//----------------------------------------------------------------------------
char* vtkPVWriter::ExtractExtension(const char* fname)
{
  const char* ext = strrchr(fname, '.');
  if (!ext || strlen(ext) < 1)
    {
    return 0;
    }
  size_t len = strlen(ext);
  char* copy = new char[len+1];
  strcpy(copy, ext);
  // Replace all spaces with \0. Extensions should not
  // contain spaces.
  for (size_t i=len-1; i>0; i--)
    {
    if (copy[i] == ' ')
      {
      copy[i] = '\0';
      }
    }
  return copy;
}
