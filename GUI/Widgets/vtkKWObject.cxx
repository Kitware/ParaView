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
#include "vtkKWTkUtilities.h"
#include "vtkCallbackCommand.h"

#include <ctype.h>
#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWObject);
vtkCxxRevisionMacro(vtkKWObject, "1.58");

vtkCxxSetObjectMacro(vtkKWObject, Application, vtkKWApplication);

//----------------------------------------------------------------------------
vtkKWObject::vtkKWObject()
{
  this->TclName         = NULL;
  this->Application     = NULL;  
  this->CallbackCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWObject::~vtkKWObject()
{
  if (this->CallbackCommand)
    {
    this->RemoveCallbackCommandObservers();
    this->CallbackCommand->Delete();
    this->CallbackCommand = NULL;
    }

  if (this->TclName)
    {
    delete [] this->TclName;
    }

  this->SetApplication(NULL);
}

//----------------------------------------------------------------------------
const char *vtkKWObject::GetTclName()
{
  // If the name is already set the just return it

  if (this->TclName)
    {
    return this->TclName;
    }

  // Otherwise we must register ourselves with Tcl and get a name

  if (!this->GetApplication())
    {
    vtkErrorMacro(
      "Attempt to create a Tcl instance before the application was set!");
    return NULL;
    }

  this->TclName = vtksys::SystemTools::DuplicateString(
    vtkKWTkUtilities::GetTclNameFromPointer(this->GetApplication(), this));

  return this->TclName;
}

//----------------------------------------------------------------------------
const char* vtkKWObject::Script(const char* format, ...)
{
  if (this->GetApplication())
    {
    va_list var_args1, var_args2;
    va_start(var_args1, format);
    va_start(var_args2, format);
    const char* result = vtkKWTkUtilities::EvaluateStringFromArgs(
      this->GetApplication(), format, var_args1, var_args2);
    va_end(var_args1);
    va_end(var_args2);
    return result;
    }

  vtkWarningMacro(
    "Attempt to script a command before the application was set!");
  return NULL;
}


//----------------------------------------------------------------------------
void vtkKWObject::SetObjectMethodCommand(
  char **command, 
  vtkObject *object, 
  const char *method)
{
  if (*command)
    {
    delete [] *command;
    *command = NULL;
    }

  const char *object_name = NULL;
  if (object)
    {
    vtkKWObject *kw_object = vtkKWObject::SafeDownCast(object);
    if (kw_object)
      {
      object_name = kw_object->GetTclName();
      }
    else
      {
      if (!this->GetApplication())
        {
        vtkErrorMacro(
          "Attempt to create a Tcl instance before the application was set!");
        }
      else
        {
        object_name = vtkKWTkUtilities::GetTclNameFromPointer(
          this->GetApplication(), object);
        }
      }
    }

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
void vtkKWObject::InvokeObjectMethodCommand(const char *command)
{
  if (command && *command && this->GetApplication())
    {
    //this->Script("eval %s", command);
    this->Script(command);
    }
}

//----------------------------------------------------------------------------
vtkCallbackCommand* vtkKWObject::GetCallbackCommand()
{
  if (!this->CallbackCommand)
    {
    this->CallbackCommand = vtkCallbackCommand::New();
    }

  this->CallbackCommand->SetClientData(this); 
  this->CallbackCommand->SetCallback(
    vtkKWObject::ProcessCallbackCommandEventsFunction);

  return this->CallbackCommand;
}

//----------------------------------------------------------------------------
void vtkKWObject::AddCallbackCommandObserver(
  vtkObject *object, unsigned long event)
{
  if (object)
    {
    vtkCallbackCommand *command = this->GetCallbackCommand();
    if (command && !object->HasObserver(event, command))
      {
      object->AddObserver(event, command);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWObject::RemoveCallbackCommandObserver(
  vtkObject *object, unsigned long event)
{
  if (object && this->CallbackCommand)
    {
    object->RemoveObservers(event, this->CallbackCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWObject::RemoveCallbackCommandObservers()
{
  if (this->CallbackCommand)
    {
    this->RemoveObserver(this->CallbackCommand);
    this->CallbackCommand->SetClientData(NULL); 
    this->CallbackCommand->SetCallback(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWObject::ProcessCallbackCommandEventsFunction(
  vtkObject *object,
  unsigned long event,
  void *clientdata,
  void *calldata)
{
  // Pass to the virtual ProcessCallbackCommandEvents method

  vtkKWObject* self = static_cast<vtkKWObject*>(clientdata);
  if (self)
    {
    self->ProcessCallbackCommandEvents(object, event, calldata);
    }
}
  
//----------------------------------------------------------------------------
void vtkKWObject::ProcessCallbackCommandEvents(
  vtkObject *vtkNotUsed(caller),
  unsigned long vtkNotUsed(event),
  void *vtkNotUsed(calldata))
{
}
  
//----------------------------------------------------------------------------
void vtkKWObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Application: " << this->GetApplication() << endl;
}
