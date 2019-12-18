/*=========================================================================

  Program:   ParaView
  Module:    vtkAnnotateAttributeDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // has to be first!

#include "vtkAnnotateAttributeDataFilter.h"

#include "vtkDataObject.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
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

vtkStandardNewMacro(vtkAnnotateAttributeDataFilter);
//----------------------------------------------------------------------------
vtkAnnotateAttributeDataFilter::vtkAnnotateAttributeDataFilter()
{
  this->ArrayName = nullptr;
  this->Prefix = nullptr;
  this->ElementId = 0;
  this->ProcessId = 0;
  this->SetArrayAssociation(vtkDataObject::ROW);
}

//----------------------------------------------------------------------------
vtkAnnotateAttributeDataFilter::~vtkAnnotateAttributeDataFilter()
{
  this->SetArrayName(nullptr);
  this->SetPrefix(nullptr);
}

//----------------------------------------------------------------------------
void vtkAnnotateAttributeDataFilter::EvaluateExpression()
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  bool evaluate_locally = (controller == nullptr) ||
    (controller->GetLocalProcessId() == this->ProcessId) || (this->ProcessId == -1);

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
  vtkSmartPyObject fname(PyString_FromString("execute_on_attribute_data"));

  // call `paraview.detail.annotation.execute_on_attribute_data(self)`
  vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(modAnnotation, fname.GetPointer(),
    self.GetPointer(), (evaluate_locally ? Py_True : Py_False), nullptr));

  CheckAndFlushPythonErrors();

  // we don't check the return val since if the call fails on one rank, we may
  // end up with deadlocks so we do the reduction no matter what.
  (void)retVal;

  if (controller && controller->GetNumberOfProcesses() > 1 && this->ProcessId != -1)
  {
    vtkMultiProcessStream stream2;
    if (this->ProcessId == controller->GetLocalProcessId())
    {
      stream2 << (this->GetComputedAnnotationValue() ? this->GetComputedAnnotationValue() : "");
    }
    controller->Broadcast(stream2, this->ProcessId);
    std::string val;
    stream2 >> val;
    this->SetComputedAnnotationValue(val.c_str());
  }
}

//----------------------------------------------------------------------------
void vtkAnnotateAttributeDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(none)") << endl;
  os << indent << "Prefix: " << (this->Prefix ? this->Prefix : "(none)") << endl;
  os << indent << "ElementId: " << this->ElementId << endl;
  os << indent << "ProcessId: " << this->ProcessId << endl;
}
