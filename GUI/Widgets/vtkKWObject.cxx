/*=========================================================================

  Module:    vtkKWObject.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWObject.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkTclUtil.h"

#include <stdarg.h>
#include <ctype.h>

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWObject );
vtkCxxRevisionMacro(vtkKWObject, "1.48");

int vtkKWObjectCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWObject::vtkKWObject()
{
  this->TclName = NULL;
  this->Application = NULL;  
  this->CommandFunction = vtkKWObjectCommand;
  this->TraceInitialized = 0;
  this->TraceReferenceObject = NULL;
  this->TraceReferenceCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWObject::~vtkKWObject()
{
  if (this->TclName)
    {
    delete [] this->TclName;
    }
  this->SetApplication(NULL);
  this->SetTraceReferenceObject(NULL);
  this->SetTraceReferenceCommand(NULL);
}


//----------------------------------------------------------------------------
const char *vtkKWObject::GetTclName()
{
  // is the name is already set the just return it
  if (this->TclName)
    {
    return this->TclName;
    }

  // otherwise we must register ourselves with tcl and get a name
  if (!this->GetApplication())
    {
    vtkErrorMacro("attempt to create Tcl instance before application was set!");
    return NULL;
    }

  vtkTclGetObjectFromPointer(this->GetApplication()->GetMainInterp(), 
                             (void *)this, "vtkKWObject");
  this->TclName = kwsys::SystemTools::DuplicateString(
    this->GetApplication()->GetMainInterp()->result);
  return this->TclName;
}

//----------------------------------------------------------------------------
const char* vtkKWObject::Script(const char* format, ...)
{
  if(this->GetApplication())
    {
    va_list var_args1, var_args2;
    va_start(var_args1, format);
    va_start(var_args2, format);
    const char* result =
      this->GetApplication()->ScriptInternal(format, var_args1, var_args2);
    va_end(var_args1);
    va_end(var_args2);
    return result;
    }
  else
    {
    vtkWarningMacro("Attempt to script a command without a KWApplication.");
    return 0;
    }
}


//----------------------------------------------------------------------------
int vtkKWObject::GetIntegerResult(vtkKWApplication *app)
{
  if (app)
    {
    return atoi(Tcl_GetStringResult(app->GetMainInterp()));
    }
  return 0;
}

float vtkKWObject::GetFloatResult(vtkKWApplication *app)
{
  if (app)
    {
    return atof(Tcl_GetStringResult(app->GetMainInterp()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWObject::SetApplication (vtkKWApplication* arg)
{  
  vtkDebugMacro(<< this->GetClassName() 
  << " (" << this << "): setting " << "Application" " to " << arg ); 
  if (this->Application != arg) 
    { 
    if (this->Application != NULL) 
      { 
      this->Application->UnRegister(this); 
      this->Application = 0;
      }
    this->Application = arg;
    if (this->Application != NULL)
      {
      this->Application->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkKWObject::InitializeTrace(ofstream* file)
{
  int stateFlag = 0;
  int dummyInit = 0;
  int* pInit;

  if(this->GetApplication() == NULL)
    {
    return 0;
    }

  // Special logic for state files.
  // The issue is that this KWObject can only keep track of initialization
  // for one file, and I do not like any possible solutions to extend this.
  if (file == NULL || file == this->GetApplication()->GetTraceFile())
    { // Tracing:  Keep track of initialization.
    file = this->GetApplication()->GetTraceFile();
    pInit = &(this->TraceInitialized);
    }
  else
    { // Saving state: Ignore trace initialization.
    stateFlag = 1;
    pInit = &(dummyInit);
    }

  // There is no need to do anything if there is no trace file.
  if (file == NULL)
    {
    return 0;
    }
  if (*pInit)
    {
    return 1;
    }
  if (this->TraceReferenceObject && this->TraceReferenceCommand)
    {
    if (this->TraceReferenceObject->InitializeTrace(file))
      {
      *file << "set kw(" << this->GetTclName() << ") [$kw(" 
            << this->TraceReferenceObject->GetTclName() << ") "
            << this->TraceReferenceCommand << "]" << endl;
      *pInit = 1;
      }
    }

  // Hack to get state working.
  if (stateFlag)
    {  // Tracing relies on sources being initialized outside of this call.
    return 1;
    }

  return *pInit;
}  

//----------------------------------------------------------------------------
void vtkKWObject::AddTraceEntry(const char *format, ...)
{
  if ( !this->GetApplication() )
    {
    return;
    }

  ofstream *os;

  os = this->GetApplication()->GetTraceFile();
  if (os == NULL)
    {
    return;
    }

  if (this->InitializeTrace(os) == 0)
    {
    return;
    }

  char event[1600];
  char* buffer = event;
  
  va_list ap;
  va_start(ap, format);
  int length = this->EstimateFormatLength(format, ap);
  va_end(ap);
  
  if(length > 1599)
    {
    buffer = new char[length+1];
    }
  
  va_list var_args;
  va_start(var_args, format);
  vsprintf(buffer, format, var_args);
  va_end(var_args);
  
  *os << buffer << endl;
  
  if(buffer != event)
    {
    delete [] buffer;
    }
}

//----------------------------------------------------------------------------
int vtkKWObject::EstimateFormatLength(const char* format, va_list ap)
{
  if (!format)
    {
    return 0;
    }

  // Quick-hack attempt at estimating the length of the string.
  // Should never under-estimate.
  
  // Start with the length of the format string itself.
  int length = strlen(format);
  
  // Increase the length for every argument in the format.
  const char* cur = format;
  while(*cur)
    {
    if(*cur++ == '%')
      {
      // Skip "%%" since it doesn't correspond to a va_arg.
      if(*cur != '%')
        {
        while(!int(isalpha(*cur)))
          {
          ++cur;
          }
        switch (*cur)
          {
          case 's':
            {
            // Check the length of the string.
            char* s = va_arg(ap, char*);
            if(s)
              {
              length += strlen(s);
              }
            } break;
          case 'e':
          case 'f':
          case 'g':
            {
            // Assume the argument contributes no more than 64 characters.
            length += 64;
            
            // Eat the argument.
            static_cast<void>(va_arg(ap, double));
            } break;
          default:
            {
            // Assume the argument contributes no more than 64 characters.
            length += 64;
            
            // Eat the argument.
            static_cast<void>(va_arg(ap, int));
            } break;
          }
        }
      
      // Move past the characters just tested.
      ++cur;
      }
    }
  
  return length;
}

//----------------------------------------------------------------------------
void vtkKWObject::SetObjectMethodCommand(
  char **command, 
  vtkKWObject *object, 
  const char *method)
{
  if (*command)
    {
    delete [] *command;
    *command = NULL;
    }

  const char *object_name = object ? object->GetTclName() : NULL;

  size_t object_len = object_name ? strlen(object_name) + 1 : 0;
  size_t method_len = method ? strlen(method) : 0;

  *command = new char[object_len + method_len + 1];
  if (object_name && method)
    {
    sprintf(*command, "%s %s", object_name, method);
    }
  else if (object_name)
    {
    sprintf(*command, "%s", object_name);
    }
  else if (method)
    {
    sprintf(*command, "%s", method);
    }

  (*command)[object_len + method_len] = '\0';
}

//----------------------------------------------------------------------------
void vtkKWObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Application: " << this->GetApplication() << endl;
  os << indent << "TraceInitialized: " << this->GetTraceInitialized() << endl;
  os << indent << "TraceReferenceCommand: " 
     << (this->TraceReferenceCommand?this->TraceReferenceCommand:"none") 
     << endl;
  os << indent << "TraceReferenceObject: " 
     << this->GetTraceReferenceObject() << endl;
}
