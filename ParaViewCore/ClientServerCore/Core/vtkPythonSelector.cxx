/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be the first thing that's included

#include "vtkPythonSelector.h"
#include "vtkPythonUtil.h"

#include "vtkCompositeDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPyObject.h"

#include <cassert>
#include <sstream>

vtkStandardNewMacro(vtkPythonSelector);

//----------------------------------------------------------------------------
vtkPythonSelector::vtkPythonSelector()
{
}

//----------------------------------------------------------------------------
vtkPythonSelector::~vtkPythonSelector()
{
}

//----------------------------------------------------------------------------
bool vtkPythonSelector::ComputeSelectedElements(vtkDataObject* input, vtkDataObject* output)
{
  assert(input != nullptr);
  assert(output != nullptr);
  assert(this->Node != nullptr);

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject psModule;
  psModule.TakeReference(PyImport_ImportModule("paraview.detail.python_selector"));
  if (!psModule)
  {
    vtkWarningMacro("Failed to import 'paraview.python_selector'");
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return false;
    }
  }

  vtkSmartPyObject functionName(PyString_FromString("execute"));
  vtkSmartPyObject inputObj(vtkPythonUtil::GetObjectFromPointer(input));
  vtkSmartPyObject nodeObj(vtkPythonUtil::GetObjectFromPointer(this->Node));
  vtkSmartPyObject arrayNameObj(PyString_FromString(this->InsidednessArrayName.c_str()));
  vtkSmartPyObject outputObj(vtkPythonUtil::GetObjectFromPointer(output));

  vtkSmartPyObject retVal(
    PyObject_CallMethodObjArgs(psModule, functionName.GetPointer(), inputObj.GetPointer(),
      nodeObj.GetPointer(), arrayNameObj.GetPointer(), outputObj.GetPointer(), nullptr));
  if (!retVal)
  {
    vtkWarningMacro("Could not invoke 'python_selector.execute()'");
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkPythonSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
