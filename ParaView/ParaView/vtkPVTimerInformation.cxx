/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerInformation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVTimerInformation.h"
#include "vtkObjectFactory.h"
#include "vtkDataObject.h"
#include "vtkQuadricClustering.h"
#include "vtkByteSwap.h"
#include "vtkTimerLog.h"
#include "vtkPVApplication.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTimerInformation);
vtkCxxRevisionMacro(vtkPVTimerInformation, "1.3");



//----------------------------------------------------------------------------
vtkPVTimerInformation::vtkPVTimerInformation()
{
  this->NumberOfLogs = 0;
  this->Logs = NULL;
}


//----------------------------------------------------------------------------
vtkPVTimerInformation::~vtkPVTimerInformation()
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
    {
    if (this->Logs && this->Logs[idx])
      {
      delete [] this->Logs[idx];
      this->Logs[idx] = NULL;
      }
    }

  if (this->Logs)
    {
    delete [] this->Logs;
    this->Logs = NULL;
    }
  this->NumberOfLogs = 0;
}


//----------------------------------------------------------------------------
void vtkPVTimerInformation::Reallocate(int num)
{
  int idx;
  char** newLogs;

  if (num <= this->NumberOfLogs)
    {
    vtkErrorMacro("Could not shrink logs.")
    return;
    }

  newLogs = new char*[num];
  for (idx = 0; idx < num; ++idx)
    {
    newLogs[idx] = NULL;
    }

  // Copy existing logs.
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
    {
    newLogs[idx] = this->Logs[idx];
    this->Logs[idx] = NULL;
    }
  
  if (this->Logs)
    {
    delete [] this->Logs;
    }

  this->Logs = newLogs;
  this->NumberOfLogs = num;
} 



//----------------------------------------------------------------------------
// This ignores the object, and gets the log from the timer.
void vtkPVTimerInformation::CopyFromObject(vtkObject* o)
{
  this->Reallocate(1);
  ostrstream *fptr;
  int length;
  char *str;
  vtkPVApplication* pvApp;
  float threshold = 0.001;

  pvApp = vtkPVApplication::SafeDownCast(o);
  if (pvApp)
    {
    threshold = pvApp->GetLogThreshold();
    }
  
  length = vtkTimerLog::GetNumberOfEvents() * 40;
  if (length > 0)
    {
    str = new char [length];
    fptr = new ostrstream(str, length);

    if (fptr->fail())
      {
      vtkErrorMacro(<< "Unable to string stream");
      return;
      }
    else
      {
      //*fptr << "Hello world !!!\n ()";
      vtkTimerLog::DumpLogWithIndents(fptr, threshold);

      length = fptr->pcount();
      str[length] = '\0';

      delete fptr;
      
      }
    this->Logs[0] = str;
    }  
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::CopyFromMessage(unsigned char* msg)
{
  int endianMarker;
  int length, num, idx;

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
  this->Reallocate(num);
  for (idx = 0; idx < num; ++idx)
    {
    char* log;
    length = strlen((const char*)msg);
    log = new char[length+1];
    memcpy(log, msg, length+1);
    this->Logs[idx] = log;
    log = NULL;
    msg += length+1;
    }
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::AddInformation(vtkPVInformation* info)
{
  int oldNum;
  int num, idx;
  vtkPVTimerInformation* pdInfo;
  char* log;
  int length;
  char* copyLog;

  pdInfo = vtkPVTimerInformation::SafeDownCast(info);

  oldNum = this->NumberOfLogs;
  num = pdInfo->GetNumberOfLogs();
  if (num <= 0)
    {
    return;
    }

  this->Reallocate(oldNum+num);
  for (idx = 0; idx < num; ++idx)
    {
    log = pdInfo->GetLog(idx);
    if (log)
      {
      length = strlen(log);
      copyLog = new char[length+1];
      memcpy(copyLog, log, length+1);
      this->Logs[idx+oldNum] = copyLog;
      copyLog = NULL;
      }
    }
}

void vtkPVTimerInformation::CopyToStream(vtkClientServerStream* css) const
{ 
  css->Reset();
  *css << vtkClientServerStream::Reply
       << this->NumberOfLogs;
  int idx;
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
    {
    *css << (const char*)this->Logs[idx];
    }
  *css << vtkClientServerStream::End;
}


//----------------------------------------------------------------------------
void
vtkPVTimerInformation::CopyFromStream(const vtkClientServerStream* css)
{ 
  int idx;
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
    {
    delete []this->Logs[idx];
    }
    
  if(!css->GetArgument(0, 0, &this->NumberOfLogs))
    {
    vtkErrorMacro("Error NumberOfLogs from message.");
    return;
    }
  this->Reallocate(this->NumberOfLogs);
  for (idx = 0; idx < this->NumberOfLogs; ++idx)
    {
    char* log;
    if(!css->GetArgument(0, idx+1, &log))
      {
      vtkErrorMacro("Error parsing LOD geometry memory size from message.");
      return;
      }
    this->Logs[idx] = strcpy(new char[strlen(log)+1], log);
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
    return NULL;
    }
  return this->Logs[idx];
}

//----------------------------------------------------------------------------
void vtkPVTimerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

  



