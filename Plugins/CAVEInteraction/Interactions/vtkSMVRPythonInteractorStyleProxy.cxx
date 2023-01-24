/*=========================================================================

   Program: ParaView
   Module:  vtkSMVRPythonInteractorStyleProxy.cxx

   Copyright (c) Kitware, Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

==========================================================================*/
/**************************************************************************/
/*                                                                        */
/* Style for the head tracking interface -- vtkSMVRPythonInteractorStyleProxy  */
/*                                                                        */
/* NOTES:                                                                 */
/*    * The simplest of interface styles -- simply maps head tracking     */
/*        data to the eye location.                                       */
/*                                                                        */
/*    * It is expected that the RenderView EyeTransformMatrix is the      */
/*        property that will be connected to the head tracker.            */
/*                                                                        */
/**************************************************************************/
#include "vtkPython.h" // must be first

#include "vtkSMVRPythonInteractorStyleProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPyObject.h"
#include "vtkVRQueue.h"

#include "vtksys/FStream.hxx"

#include <algorithm>
#include <sstream>

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

class vtkSMVRPythonInteractorStyleProxy::Internal
{
public:
  const char* ModuleName = "paraview.detail";
  vtkSmartPyObject Module;
  vtkSmartPyObject PythonObject;
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRPythonInteractorStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRPythonInteractorStyleProxy::vtkSMVRPythonInteractorStyleProxy()
  : Superclass()
{
  this->Internals = new Internal();
  this->FileName = nullptr;
}

// ----------------------------------------------------------------------------
vtkSMVRPythonInteractorStyleProxy::~vtkSMVRPythonInteractorStyleProxy()
{
  this->FileName = nullptr;
  delete this->Internals;
}

// ----------------------------------------------------------------------------
bool vtkSMVRPythonInteractorStyleProxy::ReadPythonFile(const char* path, std::string& contents)
{
  std::string line;
  vtksys::ifstream myfile(path);
  contents.clear();
  if (!myfile.is_open())
  {
    return false;
  }

  while (getline(myfile, line))
  {
    contents.append(line).append("\n");
  }
  myfile.close();

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::SetPythonObject(void* obj)
{
  vtkPythonScopeGilEnsurer gilEnsurer;

  this->Internals->PythonObject = static_cast<PyObject*>(obj);

  if (this->Internals->PythonObject)
  {
    vtkSmartPyObject fname(PyString_FromString("Initialize"));
    vtkSmartPyObject vtkself(vtkPythonUtil::GetObjectFromPointer(this));
    PyObject_CallMethodObjArgs(this->Internals->PythonObject.GetPointer(), fname.GetPointer(),
      vtkself.GetPointer(), nullptr);
    CheckAndFlushPythonErrors();
    this->InvokeEvent(vtkSMVRInteractorStyleProxy::INTERACTOR_STYLE_REQUEST_CONFIGURE);
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::RecreateVTKObjects()
{
  this->ReloadPythonFile();
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::ReloadPythonFile()
{
  if (this->FileName == nullptr)
  {
    return;
  }

  std::string fileContents;
  if (!this->ReadPythonFile(this->FileName, fileContents))
  {
    vtkWarningMacro(<< "Unable to open " << this->FileName << " for reading");
  }

  // Initialize Python is not already initialized.
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;

  // Import Module --------------------------------------------------------
  vtkSmartPyObject importedModule(PyImport_ImportModule(this->Internals->ModuleName));
  if (!importedModule || CheckAndFlushPythonErrors())
  {
    return;
  }

  // Setup Module with user given script
  vtkSmartPyObject load_method(PyString_FromString("module_from_string"));
  this->Internals->Module.TakeReference(PyObject_CallMethodObjArgs(
    importedModule, load_method, PyString_FromString(fileContents.c_str()), nullptr));

  if (!this->Internals->Module || CheckAndFlushPythonErrors())
  {
    vtkErrorMacro("'Python' module_from_string failed to load");
    return;
  }

  // call the create_interactor_style() method
  vtkSmartPyObject create_method(PyString_FromString("create_interactor_style"));
  vtkSmartPyObject styleObject(
    PyObject_CallMethodObjArgs(this->Internals->Module, create_method, nullptr, nullptr));
  CheckAndFlushPythonErrors();

  this->SetPythonObject(styleObject);
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::UpdateVTKObjects()
{
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("FileName"));
  const char* expr = svp->GetElement(0);
  if (*expr)
  {
    this->SetFileName(expr);
  }

  this->ReloadPythonFile();
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::InvokeHandler(const char* mname, const vtkVREvent& event)
{
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!this->Internals->PythonObject)
  {
    vtkWarningMacro("No python object!");
    return;
  }

  vtkSmartPyObject fname(PyString_FromString(mname));
  vtkSmartPyObject vtkself(vtkPythonUtil::GetObjectFromPointer(this));

  unsigned int eventType = event.eventType;
  std::string role;

  switch (eventType)
  {
    case TRACKER_EVENT:
    {
      role = this->GetTrackerRole(event.name);
      Py_ssize_t numElements = 16, i = 0;
      unsigned int j = 0;
      vtkSmartPyObject pyrole(PyUnicode_FromString(role.c_str()));
      vtkSmartPyObject pysensor(PyLong_FromLong(event.data.tracker.sensor));
      PyObject* pymatrix = PyList_New(numElements);
      for (; i < numElements; ++i, ++j)
      {
        PyList_SetItem(pymatrix, i, PyFloat_FromDouble(event.data.tracker.matrix[j]));
      }
      vtkSmartPyObject retVal(
        PyObject_CallMethodObjArgs(this->Internals->PythonObject.GetPointer(), fname.GetPointer(),
          vtkself.GetPointer(), pyrole.GetPointer(), pysensor.GetPointer(), pymatrix, nullptr));
    }
    break;
    case ANALOG_EVENT:
    {
      role = this->GetAnalogRole(event.name);
      Py_ssize_t i = 0;
      unsigned int numElements = event.data.analog.num_channels, j = 0;
      vtkSmartPyObject pyrole(PyUnicode_FromString(role.c_str()));
      vtkSmartPyObject pynumchan(PyLong_FromLong(event.data.analog.num_channels));
      PyObject* pychannels = PyList_New(numElements);
      for (; j < numElements; ++i, ++j)
      {
        PyList_SetItem(pychannels, i, PyFloat_FromDouble(event.data.analog.channel[j]));
      }
      vtkSmartPyObject retVal(
        PyObject_CallMethodObjArgs(this->Internals->PythonObject.GetPointer(), fname.GetPointer(),
          vtkself.GetPointer(), pyrole.GetPointer(), pynumchan.GetPointer(), pychannels, nullptr));
    }
    break;
    case BUTTON_EVENT:
    {
      role = this->GetButtonRole(event.name);
      vtkSmartPyObject pyrole(PyUnicode_FromString(role.c_str()));
      vtkSmartPyObject pybutton(PyLong_FromLong(event.data.button.button));
      vtkSmartPyObject pystate(PyLong_FromLong(event.data.button.state));
      vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(this->Internals->PythonObject.GetPointer(),
        fname.GetPointer(), vtkself.GetPointer(), pyrole.GetPointer(), pybutton.GetPointer(),
        pystate.GetPointer(), nullptr));
    }
    break;
    default:
      vtkWarningMacro(<< "Unrecognized event type: " << eventType);
      break;
  }

  CheckAndFlushPythonErrors();
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::HandleTracker(const vtkVREvent& event)
{
  this->InvokeHandler("HandleTracker", event);
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::HandleAnalog(const vtkVREvent& event)
{
  this->InvokeHandler("HandleAnalog", event);
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::HandleButton(const vtkVREvent& event)
{
  this->InvokeHandler("HandleButton", event);
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMVRPythonInteractorStyleProxy::SaveConfiguration()
{
  vtkPVXMLElement* elt = vtkSMVRInteractorStyleProxy::SaveConfiguration();

  vtkSMStringVectorProperty* svp;

  // Save the FileName
  svp = vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("FileName"));
  const char* fileName = svp->GetElement(0);

  vtkPVXMLElement* fileNameElt = vtkPVXMLElement::New();
  fileNameElt->SetName("FileName");
  fileNameElt->AddAttribute("value", fileName);
  elt->AddNestedElement(fileNameElt);
  fileNameElt->FastDelete();

  return elt;
}

// ----------------------------------------------------------------------------
bool vtkSMVRPythonInteractorStyleProxy::Configure(
  vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  bool result = true;

  for (unsigned int neCount = 0; neCount < child->GetNumberOfNestedElements(); neCount++)
  {
    vtkPVXMLElement* element = child->GetNestedElement(neCount);
    if (element && element->GetName())
    {
      if (strcmp(element->GetName(), "FileName") == 0)
      {
        const char* value = element->GetAttributeOrDefault("value", nullptr);
        if (value && *value)
        {
          vtkSMStringVectorProperty* svp =
            vtkSMStringVectorProperty::SafeDownCast(this->GetProperty("FileName"));
          if (svp->SetElement(0, value) == 0)
          {
            vtkWarningMacro(<< "Invalid FileName property value: " << value);
            result = false;
          }
        }
      }
    }
  }

  if (result)
  {
    this->UpdateVTKObjects();
    result = vtkSMVRInteractorStyleProxy::Configure(child, locator);
  }

  return result;
}
