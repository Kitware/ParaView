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
vtkCxxRevisionMacro(vtkKWObject, "1.49");

int vtkKWObjectCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWObject::vtkKWObject()
{
  this->TclName = NULL;
  this->Application = NULL;  
  this->CommandFunction = vtkKWObjectCommand;
}

//----------------------------------------------------------------------------
vtkKWObject::~vtkKWObject()
{
  if (this->TclName)
    {
    delete [] this->TclName;
    }
  this->SetApplication(NULL);
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
}
