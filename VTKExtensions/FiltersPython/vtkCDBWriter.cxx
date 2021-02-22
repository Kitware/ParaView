/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCDBWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"

#include "vtkCDBWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#include "vtkTable.h"

static bool CheckError()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  PyObject* exception = PyErr_Occurred();
  if (exception)
  {
    PyErr_Print();
    PyErr_Clear();
    return true;
  }
  return false;
}

vtkStandardNewMacro(vtkCDBWriter);
//----------------------------------------------------------------------------
vtkCDBWriter::vtkCDBWriter()
  : Path(nullptr)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkCDBWriter::~vtkCDBWriter()
{
  this->SetPath(nullptr);
}

//----------------------------------------------------------------------------
void vtkCDBWriter::Write()
{
  this->Modified();
  this->Update();
}

//----------------------------------------------------------------------------
int vtkCDBWriter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Path)
  {
    return 0;
  }

  auto input = vtkTable::GetData(inputVector[0], 0);

  // ensure Python interp is initialized.
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject module(PyImport_ImportModule("paraview.detail.cdbwriter"));
  if (::CheckError() || !module)
  {
    vtkErrorMacro("Failed to import required Python module 'paraview.detail.cdbwriter'.");
    return 0;
  }

  vtkSmartPyObject py_callable(PyObject_GetAttrString(module, "write"));
  vtkSmartPyObject py_fname(PyString_FromString(this->Path));
  vtkSmartPyObject py_table(vtkPythonUtil::GetObjectFromPointer(input));
  vtkSmartPyObject py_result(PyObject_CallFunctionObjArgs(
    py_callable, py_fname.GetPointer(), py_table.GetPointer(), nullptr));
  if (::CheckError() || !py_result)
  {
    return 0;
  }
  return py_result == Py_True ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkCDBWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Path: " << (this->Path ? this->Path : "(nullptr)") << endl;
}
