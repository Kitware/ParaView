/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonPipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkPython.h" // must be first

#include "vtkCPPythonPipeline.h"

#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

extern "C" {
void vtkPVInitializePythonModules();
}

//----------------------------------------------------------------------------
namespace
{
void CatalystInitializePython()
{
  static bool initialized = false;
  if (initialized)
  {
    return;
  }
  initialized = true;

  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();

  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  // initialize Python environment.
  vtkSmartPyObject module(PyImport_ImportModule("paraview.catalyst.detail"));
  if (!module)
  {
    PyErr_Print();
    PyErr_Clear();
  }

  vtkSmartPyObject method(PyString_FromString("InitializePythonEnvironment"));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(module, method, nullptr));
  if (!result)
  {
    PyErr_Print();
    PyErr_Clear();
  }
}
}
//----------------------------------------------------------------------------
vtkCPPythonPipeline::vtkCPPythonPipeline()
{
  CatalystInitializePython();
}

//----------------------------------------------------------------------------
vtkCPPythonPipeline::~vtkCPPythonPipeline()
{
}

//----------------------------------------------------------------------------
void vtkCPPythonPipeline::FixEOL(std::string& str)
{
  const std::string from = "\\n";
  const std::string to = "\\\\n";
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  // sanitize triple quotes while we are at it
  const std::string from2 = "\"\"\"";
  const std::string to2 = "'''";
  start_pos = 0;
  while ((start_pos = str.find(from2, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from2.length(), to2);
    start_pos += to.length();
  }
  return;
}

//----------------------------------------------------------------------------
std::string vtkCPPythonPipeline::GetPythonAddress(void* pointer)
{
  char addressOfPointer[1024];
#ifdef COPROCESSOR_WIN32_BUILD
  sprintf_s(addressOfPointer, "%p", pointer);
#else
  sprintf(addressOfPointer, "%p", pointer);
#endif
  char* aplus = addressOfPointer;
  if ((addressOfPointer[0] == '0') && ((addressOfPointer[1] == 'x') || addressOfPointer[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }

  std::string value = aplus;
  return value;
}
