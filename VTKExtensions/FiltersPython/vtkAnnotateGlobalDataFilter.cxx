/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateGlobalDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // has to be first!

#include "vtkAnnotateGlobalDataFilter.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

#include <string>
#include <vtksys/SystemTools.hxx>

namespace
{
bool CheckAndFlushPythonErrors()
{
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return true;
  }
  return false;
}
}

vtkStandardNewMacro(vtkAnnotateGlobalDataFilter);
//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::vtkAnnotateGlobalDataFilter()
{
  this->Prefix = 0;
  this->Postfix = 0;
  this->FieldArrayName = 0;
  this->Format = 0;
  this->SetFormat("%7.5g");
  this->SetArrayAssociation(vtkDataObject::FIELD);
}

//----------------------------------------------------------------------------
vtkAnnotateGlobalDataFilter::~vtkAnnotateGlobalDataFilter()
{
  this->SetPrefix(0);
  this->SetPostfix(0);
  this->SetFieldArrayName(0);
}

//----------------------------------------------------------------------------
void vtkAnnotateGlobalDataFilter::EvaluateExpression()
{
  // ensure Python is initialized (safe to call many times)
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject modAnnotation(PyImport_ImportModule("paraview.detail.annotation"));

  CheckAndFlushPythonErrors();

  if (!modAnnotation)
  {
    vtkErrorMacro("Failed to import `paraview.detail.annotation` module.");
    return;
  }

  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject fname(PyString_FromString("execute_on_global_data"));

  // call `paraview.detail.annotation.execute_on_global_data(self)`
  vtkSmartPyObject retVal(
    PyObject_CallMethodObjArgs(modAnnotation, fname.GetPointer(), self.GetPointer(), nullptr));

  CheckAndFlushPythonErrors();

  // at some point we may want to check retval
  (void)retVal;
}

//----------------------------------------------------------------------------
void vtkAnnotateGlobalDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldArrayName: " << (this->FieldArrayName ? this->FieldArrayName : "(none)")
     << endl;
  os << indent << "Prefix: " << (this->Prefix ? this->Prefix : "(none)") << endl;
  os << indent << "Postfix: " << (this->Postfix ? this->Postfix : "(none)") << endl;
}
