/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonScriptV2Pipeline.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be first

#include "vtkCPPythonScriptV2Pipeline.h"

#include "vtkCPDataDescription.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

class vtkCPPythonScriptV2Pipeline::vtkInternals
{
public:
  vtkSmartPyObject APIModule;
  vtkSmartPyObject Package;

  bool LoadAPIModule()
  {
    if (this->APIModule)
    {
      return true;
    }

    this->APIModule.TakeReference(PyImport_ImportModule("paraview.catalyst.v2_internals"));
    if (!this->APIModule)
    {
      vtkLogF(ERROR, "Failed to import required Python module 'paraview.catalyst.v2_internals'");
      vtkInternals::FlushErrors();
      return false;
    }
    return true;
  }

  static bool FlushErrors()
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return false;
    }
    return true;
  }
};

vtkStandardNewMacro(vtkCPPythonScriptV2Pipeline);
//----------------------------------------------------------------------------
vtkCPPythonScriptV2Pipeline::vtkCPPythonScriptV2Pipeline()
  : Internals(new vtkCPPythonScriptV2Pipeline::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkCPPythonScriptV2Pipeline::~vtkCPPythonScriptV2Pipeline()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Pipeline::InitializeFromZIP(
  const char* zipfilename, const char* packagename)
{
  // TODO: Use MPI to exchange ZIP package among ranks.
  auto& internals = (*this->Internals);

  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!internals.LoadAPIModule())
  {
    return false;
  }

  vtkSmartPyObject method(PyString_FromString("load_package_from_zip"));
  vtkSmartPyObject archive(PyString_FromString(zipfilename));
  vtkSmartPyObject package(packagename ? PyString_FromString(packagename) : nullptr);
  internals.Package.TakeReference(PyObject_CallMethodObjArgs(
    internals.APIModule, method, archive.GetPointer(), package.GetPointer(), nullptr));
  if (!internals.Package)
  {
    vtkInternals::FlushErrors();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Pipeline::InitializeFromDirectory(const char* path)
{
  auto& internals = (*this->Internals);
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!internals.LoadAPIModule())
  {
    return false;
  }

  vtkSmartPyObject method(PyString_FromString("load_package_from_dir"));
  vtkSmartPyObject archive(PyString_FromString(path));
  internals.Package.TakeReference(
    PyObject_CallMethodObjArgs(internals.APIModule, method, archive.GetPointer(), nullptr));
  if (!internals.Package)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkCPPythonScriptV2Pipeline::InitializeFromScript(const char* pyfilename)
{
  // TODO: Use MPI to exchange py file among ranks.
  auto& internals = (*this->Internals);
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!internals.LoadAPIModule())
  {
    return false;
  }

  vtkSmartPyObject method(PyString_FromString("load_module_from_file"));
  vtkSmartPyObject archive(PyString_FromString(pyfilename));
  internals.Package.TakeReference(
    PyObject_CallMethodObjArgs(internals.APIModule, method, archive.GetPointer(), nullptr));
  if (!internals.Package)
  {
    vtkInternals::FlushErrors();
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptV2Pipeline::RequestDataDescription(vtkCPDataDescription* dataDescription)
{
  auto& internals = (*this->Internals);
  if (!internals.Package || !internals.APIModule)
  {
    return 0;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("request_data_description"));
  vtkSmartPyObject pyarg(vtkPythonUtil::GetObjectFromPointer(dataDescription));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, pyarg.GetPointer(), internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptV2Pipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  auto& internals = (*this->Internals);
  if (!internals.Package || !internals.APIModule)
  {
    return 0;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("co_process"));
  vtkSmartPyObject pyarg(vtkPythonUtil::GetObjectFromPointer(dataDescription));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, pyarg.GetPointer(), internals.Package.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCPPythonScriptV2Pipeline::Finalize()
{
  auto& internals = (*this->Internals);
  if (internals.Package && internals.APIModule)
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject method(PyString_FromString("finalize"));
    vtkSmartPyObject result(PyObject_CallMethodObjArgs(
      internals.APIModule, method, internals.Package.GetPointer(), nullptr));
    if (!result)
    {
      vtkInternals::FlushErrors();
      return 0;
    }
  }
  internals.Package = nullptr;
  return this->Superclass::Finalize();
}

//----------------------------------------------------------------------------
void vtkCPPythonScriptV2Pipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
