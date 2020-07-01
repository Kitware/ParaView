/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be first

#include "vtkSMCinemaExtractWriterProxy.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSMExtractsController.h"
#include "vtkSmartPyObject.h"

class vtkSMCinemaExtractWriterProxy::vtkInternals
{
public:
  vtkSmartPyObject APIModule;

  bool LoadAPIModule()
  {
    if (this->APIModule)
    {
      return true;
    }
    this->APIModule.TakeReference(PyImport_ImportModule("paraview.detail.cinema_extracts_writer"));
    if (!this->APIModule)
    {
      vtkLogF(
        ERROR, "Failed to import required Python module 'paraview.detail.cinema_extracts_writer'");
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

vtkStandardNewMacro(vtkSMCinemaExtractWriterProxy);
//----------------------------------------------------------------------------
vtkSMCinemaExtractWriterProxy::vtkSMCinemaExtractWriterProxy()
  : Internals(new vtkSMCinemaExtractWriterProxy::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMCinemaExtractWriterProxy::~vtkSMCinemaExtractWriterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMCinemaExtractWriterProxy::Write(vtkSMExtractsController* extractor)
{
  auto& internals = (*this->Internals);
  if (!extractor->CreateImageExtractsOutputDirectory())
  {
    return false;
  }

  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (!internals.LoadAPIModule())
  {
    return false;
  }

  vtkSmartPyObject method(PyString_FromString("write"));
  vtkSmartPyObject arg0(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject arg1(vtkPythonUtil::GetObjectFromPointer(extractor));
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(
    internals.APIModule, method, arg0.GetPointer(), arg1.GetPointer(), nullptr));
  if (!result)
  {
    vtkInternals::FlushErrors();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMCinemaExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
