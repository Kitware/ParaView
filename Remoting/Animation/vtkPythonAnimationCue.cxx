/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPython.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPythonAnimationCue.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

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

struct vtkPythonAnimationCue::pqInternals
{
  const char* ModuleName = "paraview.detail";
  vtkSmartPyObject Module;
  vtkTimeStamp ScriptModifiedTime;
  vtkTimeStamp ModuleUpdateTime;
};

vtkStandardNewMacro(vtkPythonAnimationCue);
//----------------------------------------------------------------------------
vtkPythonAnimationCue::vtkPythonAnimationCue()
  : Internals(new vtkPythonAnimationCue::pqInternals)
{
  this->Enabled = true;
  this->AddObserver(
    vtkCommand::StartAnimationCueEvent, this, &vtkPythonAnimationCue::HandleStartCueEvent);
  this->AddObserver(
    vtkCommand::AnimationCueTickEvent, this, &vtkPythonAnimationCue::HandleTickEvent);
  this->AddObserver(
    vtkCommand::EndAnimationCueEvent, this, &vtkPythonAnimationCue::HandleEndCueEvent);
}

//----------------------------------------------------------------------------
vtkPythonAnimationCue::~vtkPythonAnimationCue()
{
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::HandleStartCueEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // Initialize Python is not already initialized.
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!this->Internals->Module ||
    this->Internals->ModuleUpdateTime < this->Internals->ScriptModifiedTime)
  {
    // Import Module --------------------------------------------------------
    vtkSmartPyObject importedModule(PyImport_ImportModule(this->Internals->ModuleName));
    if (!importedModule || CheckAndFlushPythonErrors())
    {
      return;
    }

    // Setup Module with user given script
    vtkSmartPyObject load_method(PyString_FromString("module_from_string"));
    this->Internals->Module.TakeReference(PyObject_CallMethodObjArgs(
      importedModule, load_method, PyString_FromString(Script.c_str()), nullptr));

    if (!this->Internals->Module || CheckAndFlushPythonErrors())
    {
      std::cerr << "'Python' module_from_string failed to load" << std::endl;
      return;
    }
    this->Internals->ModuleUpdateTime.Modified();
  }

  // call wrapper_start_cue ---------------------------------------------------
  vtkSmartPyObject start_method(PyString_FromString("start_cue"));
  vtkSmartPyObject self = vtkPythonUtil::GetObjectFromPointer(this);
  vtkSmartPyObject resultMethod(
    PyObject_CallMethodObjArgs(this->Internals->Module, start_method, self.GetPointer(), nullptr));

  CheckAndFlushPythonErrors();
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::HandleTickEvent()
{
  if (!this->Enabled || !this->Internals->Module)
  {
    return;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("tick"));
  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject result(
    PyObject_CallMethodObjArgs(this->Internals->Module, method, self.GetPointer(), nullptr));

  CheckAndFlushPythonErrors();
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::HandleEndCueEvent()
{
  if (!this->Enabled || !this->Internals->Module)
  {
    return;
  }

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject method(PyString_FromString("end_cue"));
  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject result(
    PyObject_CallMethodObjArgs(this->Internals->Module, method, self.GetPointer(), nullptr));

  CheckAndFlushPythonErrors();
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "Script: " << this->Script << endl;
}

//----------------------------------------------------------------------------
void vtkPythonAnimationCue::SetScript(const std::string& script)
{
  if (this->Script != script)
  {
    this->Script = script;
    this->Internals->ScriptModifiedTime.Modified();
  }
}

//----------------------------------------------------------------------------
std::string vtkPythonAnimationCue::GetScript() const
{
  return this->Script;
}
