// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#define vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON                                              \
  (VTK_MODULE_ENABLE_VTK_WrappingPythonCore && VTK_MODULE_ENABLE_VTK_PythonInterpreter)

#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
#include "vtkPython.h" // must be first
#endif

#include "vtkSMVRPythonInteractorStyleProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkVRQueue.h"

#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

#include "vtksys/FStream.hxx"

#include <algorithm>
#include <sstream>

#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
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
#endif

class vtkSMVRPythonInteractorStyleProxy::Internal
{
public:
  const char* ModuleName = "paraview.detail";
#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  vtkSmartPyObject Module;
  vtkSmartPyObject PythonObject;
#endif
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRPythonInteractorStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRPythonInteractorStyleProxy::vtkSMVRPythonInteractorStyleProxy()
{
  this->Internals = new Internal;
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
#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  vtkPythonScopeGilEnsurer gilEnsurer;

  this->Internals->PythonObject = static_cast<PyObject*>(obj);

  if (this->Internals->PythonObject)
  {
    vtkSmartPyObject fname(PyUnicode_FromString("Initialize"));
    vtkSmartPyObject vtkself(vtkPythonUtil::GetObjectFromPointer(this));
    PyObject_CallMethodObjArgs(this->Internals->PythonObject.GetPointer(), fname.GetPointer(),
      vtkself.GetPointer(), nullptr);
    CheckAndFlushPythonErrors();
    this->InvokeEvent(Superclass::INTERACTOR_STYLE_REQUEST_CONFIGURE);
  }
#endif
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

#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  // Initialize Python is not already initialized.
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;

  // Import Module --------------------------------------------------------
  vtkSmartPyObject importedModule(PyImport_ImportModule(this->Internals->ModuleName));
  if (CheckAndFlushPythonErrors() || !importedModule)
  {
    vtkErrorMacro("'Python' failed to import module " << this->Internals->ModuleName);
    return;
  }

  // Setup Module with user given script
  vtkSmartPyObject load_method(PyUnicode_FromString("module_from_string"));
  this->Internals->Module.TakeReference(PyObject_CallMethodObjArgs(
    importedModule, load_method, PyUnicode_FromString(fileContents.c_str()), nullptr));

  if (CheckAndFlushPythonErrors() || !this->Internals->Module)
  {
    vtkErrorMacro("'Python' module_from_string failed to load");
    return;
  }

  // call the create_interactor_style() method
  vtkSmartPyObject create_method(PyUnicode_FromString("create_interactor_style"));
  vtkSmartPyObject styleObject(
    PyObject_CallMethodObjArgs(this->Internals->Module, create_method, nullptr, nullptr));
  if (CheckAndFlushPythonErrors() || !styleObject)
  {
    vtkErrorMacro("'Python' unable to construct interactor style");
    return;
  }

  this->SetPythonObject(styleObject);
#else
  vtkWarningMacro(<< "Python not enabled");
#endif
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
#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  vtkPythonScopeGilEnsurer gilEnsurer;

  if (!this->Internals->PythonObject)
  {
    vtkWarningMacro("No python object!");
    return;
  }

  vtkSmartPyObject fname(PyUnicode_FromString(mname));
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
    case VALUATOR_EVENT:
    {
      Py_ssize_t i = 0;
      unsigned int numElements = event.data.valuator.num_channels, j = 0;
      vtkSmartPyObject pynumchan(PyLong_FromLong(event.data.valuator.num_channels));
      PyObject* pychannels = PyList_New(numElements);
      for (; j < numElements; ++i, ++j)
      {
        PyList_SetItem(pychannels, i, PyFloat_FromDouble(event.data.valuator.channel[j]));
      }
      vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(this->Internals->PythonObject.GetPointer(),
        fname.GetPointer(), vtkself.GetPointer(), pynumchan.GetPointer(), pychannels, nullptr));
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
#endif
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::HandleTracker(const vtkVREvent& event)
{
#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  this->InvokeHandler("HandleTracker", event);
#else
  this->Superclass::HandleTracker(event);
#endif
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::HandleValuator(const vtkVREvent& event)
{
#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  this->InvokeHandler("HandleValuator", event);
#else
  this->Superclass::HandleValuator(event);
#endif
}

// ----------------------------------------------------------------------------
void vtkSMVRPythonInteractorStyleProxy::HandleButton(const vtkVREvent& event)
{
#if vtkSMVRPythonInteractorStyleProxy_WITH_PYTHON
  this->InvokeHandler("HandleButton", event);
#else
  this->Superclass::HandleButton(event);
#endif
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMVRPythonInteractorStyleProxy::SaveConfiguration()
{
  vtkPVXMLElement* elt = Superclass::SaveConfiguration();

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
    result = Superclass::Configure(child, locator);
  }

  return result;
}
