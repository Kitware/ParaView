/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonInteractiveInterpretor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"
#include "vtkPVPythonInteractiveInterpretor.h"

#include "vtkObjectFactory.h"

#include <vtkstd/string>

class vtkPVPythonInteractiveInterpretor::vtkInternal
{
public:
  PyObject* InteractiveConsole;
};

vtkStandardNewMacro(vtkPVPythonInteractiveInterpretor);
//----------------------------------------------------------------------------
vtkPVPythonInteractiveInterpretor::vtkPVPythonInteractiveInterpretor()
{
  this->Internal = new vtkInternal;
  this->Internal->InteractiveConsole = 0;
}

//----------------------------------------------------------------------------
vtkPVPythonInteractiveInterpretor::~vtkPVPythonInteractiveInterpretor()
{
  if (this->Internal->InteractiveConsole)
    {
    this->MakeCurrent();
    Py_DECREF(this->Internal->InteractiveConsole);
    this->Internal->InteractiveConsole = 0;
    this->ReleaseControl();
    }

  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVPythonInteractiveInterpretor::InitializeInternal()
{
  this->Superclass::InitializeInternal();

  // set up the code.InteractiveConsole instance that we'll use.
  const char* code = 
    "import code\n"
    "__vtkConsole=code.InteractiveConsole(locals())\n";

  // The cast is necessary because PyRun_SimpleString() hasn't always been 
  // const-correct
  PyRun_SimpleString(const_cast<char*>(code));

  // Now get the reference to __vtkConsole and save the pointer.
  PyObject* main_module = PyImport_AddModule((char*)"__main__");
  PyObject* global_dict = PyModule_GetDict(main_module);
  this->Internal->InteractiveConsole = PyDict_GetItemString(
    global_dict, "__vtkConsole");
  if (this->Internal->InteractiveConsole)
    {
    Py_INCREF(this->Internal->InteractiveConsole);
    }
  else
    {
    vtkErrorMacro("Failed to locate the InteractiveConsole object.");
    }
}

//----------------------------------------------------------------------------
bool vtkPVPythonInteractiveInterpretor::Push(const char* const code)
{
  bool ret_value = false;
  if (this->Internal->InteractiveConsole)
    {
    this->MakeCurrent();

    // The embedded python interpreter cannot handle DOS line-endings, see
    // http://sourceforge.net/tracker/?group_id=5470&atid=105470&func=detail&aid=1167922
    vtkstd::string buffer = code ? code : "";
    // replace "\r\n" with "\n"
    vtkstd::string::size_type i = buffer.find("\r\n");
    for(; i != vtkstd::string::npos; i = buffer.find("\r\n", i))
      {
      buffer.replace(i, 2, "\n");
      i++;
      }
    // replace "\r" with "\n"  (sometimes seen on Mac)
    i = buffer.find("\r");
    for(; i != vtkstd::string::npos; i = buffer.find("\r", i))
      {
      buffer.replace(i, 1, "\n");
      i++;
      }

    PyObject *res = PyObject_CallMethod(this->Internal->InteractiveConsole,
      (char*)"push", (char*)"z", buffer.c_str());
    if (res)
      {
      int status = 0;
      if (PyArg_Parse(res, (char*)"i", &status))
        {
        ret_value = (status>0);
        }
      Py_DECREF(res);
      }

    this->ReleaseControl();
    }
  return ret_value;
}

//----------------------------------------------------------------------------
void vtkPVPythonInteractiveInterpretor::ResetBuffer()
{
  if (this->Internal->InteractiveConsole)
    {
    this->MakeCurrent();
    const char* code = "__vtkConsole.resetbuffer()\n";

    // The cast is necessary because PyRun_SimpleString() hasn't always been 
    // const-correct
    PyRun_SimpleString(const_cast<char*>(code));
    this->ReleaseControl();
    }
}

//----------------------------------------------------------------------------
void vtkPVPythonInteractiveInterpretor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


