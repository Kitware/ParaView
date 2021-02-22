/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTimerInformation.h"

#include "vtkByteSwap.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

#include <sstream>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTimerInformation);

//----------------------------------------------------------------------------
vtkPVTimerInformation::vtkPVTimerInformation()
{
  this->NumberOfLogs = 0;
  this->Logs = nullptr;
  this->LogThreshold = 0;
}

//----------------------------------------------------------------------------
vtkPVTimerInformation::~vtkPVTimerInformation()
{
  int idx;

  for (idx = 0; idx < this->NumberOfLogs; ++idx)
  {
    if (this->Logs && this->Logs[idx])
    {
      delete[] this->Logs[idx];
      this->Logs[idx] = nullptr;
    }
  }

  if (this->Logs)
  {
    delete[] this->Logs;
    this->Logs = nullptr;
  }
  this->NumberOfLogs = 0;
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 828793 << this->LogThreshold;
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number >> this->LogThreshold;
  if (magic_number != 828793)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::InsertLog(int id, const char* log)
{
  if (id >= this->NumberOfLogs)
  {
    this->Reallocate(id + 1);
  }
  if (this->Logs[id])
  {
    delete[] this->Logs[id];
    this->Logs[id] = nullptr;
  }
  size_t len = strlen(log);
  char* str = new char[len + 1];
  strcpy(str, log);
  this->Logs[id] = str;
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::Reallocate(int num)
{
  int idx;
  char** newLogs;

  if (num == this->NumberOfLogs)
  {
    return;
  }

  if (num < this->NumberOfLogs)
  {
    vtkWarningMacro("Trying to shrink logs from " << this->NumberOfLogs << " to " << num);
    return;
  }

  newLogs = new char*[num];
  for (idx = 0; idx < num; ++idx)
  {
    newLogs[idx] = nullptr;
  }

  // Copy existing logs.
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
  {
    newLogs[idx] = this->Logs[idx];
    this->Logs[idx] = nullptr;
  }

  if (this->Logs)
  {
    delete[] this->Logs;
  }

  this->Logs = newLogs;
  this->NumberOfLogs = num;
}

//----------------------------------------------------------------------------
// This ignores the object, and gets the log from the timer.
void vtkPVTimerInformation::CopyFromObject(vtkObject*)
{
  int length;
  float threshold = this->LogThreshold;

  length = vtkTimerLog::GetNumberOfEvents() * 40;
  if (length > 0)
  {
    std::ostringstream fptr;
    //*fptr << "Hello world !!!\n ()";
    vtkTimerLog::DumpLogWithIndents(&fptr, threshold);
    fptr << ends;
    this->InsertLog(0, fptr.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::CopyFromMessage(unsigned char* msg)
{
  int endianMarker;
  int num, idx;

  memcpy((unsigned char*)&endianMarker, msg, sizeof(int));
  if (endianMarker != 1)
  {
    // Mismatch endian between client and server.
    vtkByteSwap::SwapVoidRange((void*)msg, 2, sizeof(int));
    memcpy((unsigned char*)&endianMarker, msg, sizeof(int));
    if (endianMarker != 1)
    {
      vtkErrorMacro("Could not decode information.");
      return;
    }
  }
  msg += sizeof(int);
  memcpy((unsigned char*)&num, msg, sizeof(int));
  msg += sizeof(int);

  // now get the logs.
  for (idx = 0; idx < num; ++idx)
  {
    char* log;
    size_t length = strlen((const char*)msg);
    log = new char[length + 1];
    memcpy(log, msg, length + 1);
    this->InsertLog(idx, log);
    log = nullptr;
    msg += length + 1;
  }
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::AddInformation(vtkPVInformation* info)
{
  int oldNum;
  int num, idx;
  vtkPVTimerInformation* pdInfo;
  char* log;
  char* copyLog;

  pdInfo = vtkPVTimerInformation::SafeDownCast(info);

  oldNum = this->NumberOfLogs;
  num = pdInfo->GetNumberOfLogs();
  if (num <= 0)
  {
    return;
  }

  for (idx = 0; idx < num; ++idx)
  {
    log = pdInfo->GetLog(idx);
    if (log)
    {
      size_t length = strlen(log);
      copyLog = new char[length + 1];
      memcpy(copyLog, log, length + 1);
      this->InsertLog((idx + oldNum), copyLog);
      copyLog = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->NumberOfLogs;
  int idx;
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
  {
    *css << (const char*)this->Logs[idx];
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int idx;
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
  {
    delete[] this->Logs[idx];
  }

  int numLogs;
  if (!css->GetArgument(0, 0, &numLogs))
  {
    vtkErrorMacro("Error NumberOfLogs from message.");
    return;
  }
  this->Reallocate(numLogs);
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
  {
    char* log;
    if (!css->GetArgument(0, idx + 1, &log))
    {
      vtkErrorMacro("Error parsing LOD geometry memory size from message.");
      return;
    }
    this->Logs[idx] = strcpy(new char[strlen(log) + 1], log);
  }
}

//----------------------------------------------------------------------------
int vtkPVTimerInformation::GetNumberOfLogs()
{
  return this->NumberOfLogs;
}

//----------------------------------------------------------------------------
char* vtkPVTimerInformation::GetLog(int idx)
{
  if (idx < 0 || idx >= this->NumberOfLogs)
  {
    return nullptr;
  }
  return this->Logs[idx];
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfLogs: " << this->NumberOfLogs << endl;
  int idx;
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
  {
    os << indent << "Log " << idx << ": \n";
    if (this->Logs[idx])
    {
      os << this->Logs[idx] << endl;
    }
    else
    {
      os << "NULL\n";
    }
  }
}
