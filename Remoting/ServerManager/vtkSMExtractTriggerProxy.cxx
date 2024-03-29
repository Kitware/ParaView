// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#define vtkSMExtractTriggerProxy_ENABLE_PYTHON 1
#else
#define vtkSMExtractTriggerProxy_ENABLE_PYTHON 0
#endif

#if vtkSMExtractTriggerProxy_ENABLE_PYTHON
#include "vtkPython.h" // must be first include

#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

#include "vtkSMExtractTriggerProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMExtractsController.h"
#include "vtkSMPropertyHelper.h"

#if vtkSMExtractTriggerProxy_ENABLE_PYTHON
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

bool IsActivatedPython(const std::string& script, vtkSMExtractsController* controller)
{
  // Initialize Python is not already initialized.
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;

  vtkSmartPyObject mdlParaView(PyImport_ImportModule("paraview.detail"));
  if (CheckAndFlushPythonErrors() || !mdlParaView)
  {
    return false;
  }

  vtkSmartPyObject load_method(PyUnicode_FromString("module_from_string"));
  vtkSmartPyObject module(PyObject_CallMethodObjArgs(
    mdlParaView, load_method, PyUnicode_FromString(script.c_str()), nullptr));
  if (CheckAndFlushPythonErrors() || !module)
  {
    return false;
  }

  vtkSmartPyObject function(PyUnicode_FromString("is_activated"));
  vtkSmartPyObject self = vtkPythonUtil::GetObjectFromPointer(controller);
  vtkSmartPyObject result(PyObject_CallMethodObjArgs(module, function, self.GetPointer(), nullptr));
  if (CheckAndFlushPythonErrors() || !result)
  {
    return false;
  }

  return PyBool_Check(result) && result.GetPointer() == Py_True;
}

}
#endif

vtkStandardNewMacro(vtkSMExtractTriggerProxy);
//----------------------------------------------------------------------------
vtkSMExtractTriggerProxy::vtkSMExtractTriggerProxy()
{
  this->LastTimeValue = VTK_DOUBLE_MIN;
  this->LastOutputTimeValue = VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------------
vtkSMExtractTriggerProxy::~vtkSMExtractTriggerProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMExtractTriggerProxy::IsActivated(vtkSMExtractsController* controller)
{
  std::string triggerName = this->GetXMLName();
  if (triggerName == "TimeStep")
  {
    const int timestep = controller->GetTimeStep();

    int start_timestep = 0;
    if (vtkSMPropertyHelper(this, "UseStartTimeStep").GetAsInt() == 1)
    {
      start_timestep = vtkSMPropertyHelper(this, "StartTimeStep").GetAsInt();
      if (start_timestep > timestep)
      {
        return false;
      }
    }

    if (vtkSMPropertyHelper(this, "UseEndTimeStep").GetAsInt() == 1 &&
      timestep > vtkSMPropertyHelper(this, "EndTimeStep").GetAsInt())
    {
      return false;
    }

    const int frequency = vtkSMPropertyHelper(this, "Frequency").GetAsInt();
    if ((timestep - start_timestep) % frequency != 0)
    {
      return false;
    }

    return true;
  }
  else if (triggerName == "TimeValue")
  {
    bool doIt = true;
    const double timevalue = controller->GetTime();
    if (timevalue == this->LastOutputTimeValue)
    {
      // this method is called multiple times (during both the RequestDataDescription
      // and DoCoProcessing steps) so we may have already checked for this timestep
      return true;
    }

    if (vtkSMPropertyHelper(this, "UseStartTimeValue").GetAsInt() == 1)
    {
      double start_timevalue = vtkSMPropertyHelper(this, "StartTimeValue").GetAsDouble();
      if (start_timevalue > timevalue)
      {
        doIt = false;
      }
    }

    if (vtkSMPropertyHelper(this, "UseEndTimeValue").GetAsInt() == 1 &&
      timevalue > vtkSMPropertyHelper(this, "EndTimeValue").GetAsDouble())
    {
      doIt = false;
    }

    double sum = 0;
    for (auto it = this->TimeStepLengths.begin(); it != this->TimeStepLengths.end(); it++)
    {
      sum += *it;
    }
    double average = 0;
    if (size_t number = this->TimeStepLengths.size())
    {
      average = sum / number;
    }

    const double length = vtkSMPropertyHelper(this, "Length").GetAsDouble();
    if (this->LastOutputTimeValue == VTK_DOUBLE_MIN && doIt == true)
    {
      this->LastOutputTimeValue = timevalue;
    }
    else if (doIt == true && (timevalue > (this->LastOutputTimeValue + length - .5 * average)))
    {
      this->LastOutputTimeValue = timevalue;
    }
    else
    {
      doIt = false;
    }

    if (this->LastTimeValue != VTK_DOUBLE_MIN)
    {
      this->TimeStepLengths.push_front(timevalue - this->LastTimeValue);
      // keep the number of time step lengths to a small number
      // since we only want to compare to the recent time step
      // lengths. 10 seems reasonable.
      if (this->TimeStepLengths.size() > 10)
      {
        this->TimeStepLengths.pop_back();
      }
    }
    this->LastTimeValue = timevalue;
    return doIt;
  }
  else if (triggerName == "Python")
  {
#if vtkSMExtractTriggerProxy_ENABLE_PYTHON
    return ::IsActivatedPython(vtkSMPropertyHelper(this, "Script").GetAsString(), controller);
#else
    vtkErrorMacro("Python support not enabled. Python trigger will be ignored!");
    return false;
#endif
  }
  else
  {
    vtkErrorMacro("Incorrect trigger type" << this->GetXMLName());
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMExtractTriggerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastTimeValue: " << this->LastTimeValue << endl;
  os << indent << "LastOutputTimeValue: " << this->LastOutputTimeValue << endl;
}
