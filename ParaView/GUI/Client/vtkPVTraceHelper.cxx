/*=========================================================================

  Module:    vtkPVTraceHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTraceHelper.h"

#include "vtkPVApplication.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>
#include <stdarg.h>

#define vtkPVTraceHelper_RefCountReferenceHelper 0

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTraceHelper );
vtkCxxRevisionMacro(vtkPVTraceHelper, "1.6");

#if vtkPVTraceHelper_RefCountReferenceHelper
vtkCxxSetObjectMacro(vtkPVTraceHelper, ReferenceHelper,
                     vtkPVTraceHelper);
#endif

//----------------------------------------------------------------------------
vtkPVTraceHelper::vtkPVTraceHelper()
{
  this->Initialized      = 0;
  this->StateInitialized = 0;
  this->TraceObject      = NULL;
  this->ReferenceHelper  = NULL;
  this->ReferenceCommand = NULL;
  this->ObjectName       = NULL;
  this->ObjectNameState  = vtkPVTraceHelper::ObjectNameStateUninitialized;
}

//----------------------------------------------------------------------------
vtkPVTraceHelper::~vtkPVTraceHelper()
{
  this->SetTraceObject(NULL);
  this->SetReferenceHelper(NULL);
  this->SetReferenceCommand(NULL);
  this->SetObjectName(NULL);
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::SetTraceObject(vtkKWObject* _arg)
{
  if (this->TraceObject == _arg) 
    { 
    return;
    }

  this->TraceObject = _arg;
  this->Modified();
} 

//----------------------------------------------------------------------------
#if !(vtkPVTraceHelper_RefCountReferenceHelper)
void vtkPVTraceHelper::SetReferenceHelper(vtkPVTraceHelper* _arg)
{
  if (this->ReferenceHelper == _arg) 
    { 
    return;
    }

  this->ReferenceHelper = _arg;
  this->Modified();
} 
#endif

//----------------------------------------------------------------------------
ofstream* vtkPVTraceHelper::GetFile()
{
  if (this->TraceObject)
    {
    vtkPVApplication *pvapp = vtkPVApplication::SafeDownCast(
      this->TraceObject->GetApplication());
    if (pvapp)
      {
      return pvapp->GetTraceFile();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkPVTraceHelper::Initialize()
{
  return this->Initialize(NULL);
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::SetStateInitialized(int init)
{
  if (this->ReferenceHelper)
    {
    this->ReferenceHelper->SetStateInitialized(init);
    }
  this->StateInitialized = init;
}

//----------------------------------------------------------------------------
int vtkPVTraceHelper::Initialize(ofstream* file)
{
  int state_flag = 0;
  int *init;

  if (!this->TraceObject)
    {
    return 0;
    }

  // Special logic for state files.
  // The issue is that this KWObject can only keep track of initialization
  // for one file, and I do not like any possible solutions to extend this.

  ofstream *os = this->GetFile();
  if (file == NULL || file == os)
    { 
    // Tracing:  Keep track of initialization.
    file = os;
    init = &(this->Initialized);
    }
  else
    { 
    // Saving state: Ignore trace initialization; keep track of state
    // initialization.
    state_flag = 1;
    init = &(this->StateInitialized);
    }

  // There is no need to do anything if there is no trace file.

  if (file == NULL)
    {
    return 0;
    }
  
  // No need to init

  if (*init)
    {
    return 1;
    }

  // Init

  if (this->ReferenceHelper && this->ReferenceCommand)
    {
    if (this->ReferenceHelper->GetTraceObject() && 
        this->ReferenceHelper->Initialize(file))
      {
      *file << "set kw(" << this->TraceObject->GetTclName() << ") [$kw(" 
            << this->ReferenceHelper->GetTraceObject()->GetTclName() << ") "
            << this->ReferenceCommand << "]" << endl;
      *init = 1;
      return 1;
      }
    }

  // Hack to get state working.

  if (state_flag)
    {  
    // Tracing relies on sources being initialized outside of this call.
    return 1;
    }

  return *init;
}  

//----------------------------------------------------------------------------
void vtkPVTraceHelper::OutputEntryInternal(
  ostream *os, int estimated_length, const char *format, va_list ap)
{
  if (os == NULL || estimated_length <= 0 || !format)
    {
    return;
    }

  char event[1600];
  char *buffer = event;
  if(estimated_length > 1599)
    {
    buffer = new char[estimated_length + 1];
    }
  
  vsprintf(buffer, format, ap);
  *os << buffer << endl;
  
  if (buffer != event)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::OutputEntry(ostream *os, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  int estimated_length = vtksys::SystemTools::EstimateFormatLength(format, ap);
  va_end(ap);
  
  va_list var_args;
  va_start(var_args, format);
  vtkPVTraceHelper::OutputEntryInternal(
    os, estimated_length, format, var_args);
  va_end(var_args);
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::OutputSimpleEntry(ostream *os, const char *trace)
{
  vtkPVTraceHelper::OutputEntry(os, trace);
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::AddEntry(const char *format, ...)
{
  // Initialize

  ofstream *os = this->GetFile();
  if (!os || this->Initialize(os) == 0)
    {
    return;
    }

  // Trace

  va_list ap;
  va_start(ap, format);
  int estimated_length = vtksys::SystemTools::EstimateFormatLength(format, ap);
  va_end(ap);
  
  va_list var_args;
  va_start(var_args, format);
  vtkPVTraceHelper::OutputEntryInternal(
    os, estimated_length, format, var_args);
  va_end(var_args);
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::AddSimpleEntry(const char *trace)
{
  this->AddEntry(trace);
}

//----------------------------------------------------------------------------
void vtkPVTraceHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Initialized: " << this->GetInitialized() << endl;
  os << indent << "StateInitialized: " << this->GetStateInitialized() << endl;
  os << indent << "TraceObject: " << this->GetTraceObject() << endl;
  os << indent << "ReferenceHelper: " 
     << this->GetReferenceHelper() << endl;
  os << indent << "ReferenceCommand: " 
     << (this->ReferenceCommand ? this->ReferenceCommand : "None") 
     << endl;
  os << indent << "ObjectName: " 
     << (this->ObjectName ? this->ObjectName : "NULL") << endl;
  os << indent << "ObjectNameState: " << this->ObjectNameState << endl;
}
