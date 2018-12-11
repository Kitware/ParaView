/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTrace.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#endif

#include "vtkSMTrace.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"

#include <cassert>
#include <map>
#include <sstream>

#if !(VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                   \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore)
class vtkSmartPyObject
{
public:
  operator bool() const { return false; }
};
#endif

class vtkSMTrace::vtkInternals
{
public:
  vtkSmartPyObject TraceModule;
  vtkSmartPyObject CreateItemFunction;
  vtkSmartPyObject UntraceableException;
};

vtkSmartPointer<vtkSMTrace> vtkSMTrace::ActiveTracer;
vtkStandardNewMacro(vtkSMTrace);
//----------------------------------------------------------------------------
vtkSMTrace::vtkSMTrace()
  : TraceXMLDefaults(false)
  , LogTraceToStdout(true)
  , PropertiesToTraceOnCreate(vtkSMTrace::RECORD_MODIFIED_PROPERTIES)
  , FullyTraceSupplementalProxies(false)
  , Internals(new vtkSMTrace::vtkInternals())
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  // ensure Python interpreter is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  this->Internals->TraceModule.TakeReference(PyImport_ImportModule("paraview.smtrace"));
  if (!this->Internals->TraceModule)
  {
    vtkErrorMacro("Failed to import paraview.smtrace module.");
  }
  else
  {
    this->Internals->CreateItemFunction.TakeReference(
      PyObject_GetAttrString(this->Internals->TraceModule, "_create_trace_item_internal"));
    if (!this->Internals->CreateItemFunction)
    {
      vtkErrorMacro(
        "Failed to locate the _create_trace_item_internal function in paraview.smtrace module.");
      this->Internals->TraceModule.TakeReference(NULL);
    }
    this->Internals->UntraceableException.TakeReference(
      PyObject_GetAttrString(this->Internals->TraceModule, "Untraceable"));
    if (!this->Internals->UntraceableException)
    {
      vtkErrorMacro("Failed to locate the Untraceable exception class in paraview.smtrace module.");
      this->Internals->TraceModule.TakeReference(NULL);
      this->Internals->CreateItemFunction.TakeReference(NULL);
    }
  }
#endif
}

//----------------------------------------------------------------------------
vtkSMTrace::~vtkSMTrace()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
vtkStdString vtkSMTrace::GetState(int propertiesToTraceOnCreate, bool skipHiddenRepresentations)
{
  if (vtkSMTrace::ActiveTracer.GetPointer() != NULL)
  {
    vtkGenericWarningMacro("Tracing is active. Cannot save state.");
    return vtkStdString();
  }

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;
  try
  {
    vtkSmartPyObject module(PyImport_ImportModule("paraview.smstate"));
    if (!module || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to import paraview.smstate module.");
      throw 1;
    }

    vtkSmartPyObject result(PyObject_CallMethod(module, const_cast<char*>("get_state"),
      const_cast<char*>("(ii)"), propertiesToTraceOnCreate, (skipHiddenRepresentations ? 1 : 0)));
    if (!result || PyErr_Occurred())
    {
      vtkGenericWarningMacro("Failed to generate state.");
      throw 1;
    }
    return vtkStdString(PyString_AsString(result));
  }
  catch (int)
  {
    if (PyErr_Occurred())
    {
      PyErr_Print();
      PyErr_Clear();
    }
  }
#endif
  (void)propertiesToTraceOnCreate;
  (void)skipHiddenRepresentations;
  return vtkStdString();
}

//----------------------------------------------------------------------------
vtkSMTrace* vtkSMTrace::StartTrace(const char* preamble)
{
  if (vtkSMTrace::ActiveTracer.GetPointer() == NULL)
  {
    vtkSMTrace::ActiveTracer = vtkSmartPointer<vtkSMTrace>::New();
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    if (!vtkSMTrace::ActiveTracer->Internals->TraceModule)
    {
      vtkGenericWarningMacro("Trace not started since required Python modules are missing.");
      vtkSMTrace::ActiveTracer = NULL;
    }
    else
    {
      vtkPythonScopeGilEnsurer gilEnsurer;
      std::ostringstream str;
      str << "# trace generated using " << vtkSMProxyManager::GetParaViewSourceVersion() << "\n"
          << "#"
          << "\n"
          << "# To ensure correct image size when batch processing, please search \n"
          << "# for and uncomment the line `# renderView*.ViewSize = [*,*]`\n";
      vtkSmartPyObject _start_trace_internal(PyObject_CallMethod(
        vtkSMTrace::ActiveTracer->GetTraceModule(), const_cast<char*>("_start_trace_internal"),
        const_cast<char*>("(s)"), const_cast<char*>(preamble ? preamble : str.str().c_str())));
      vtkSMTrace::ActiveTracer->CheckForError();
    }
#else
    (void)preamble;
#endif
  }
  else
  {
    vtkGenericWarningMacro("Trace already started. Continuing with the same one.");
  }
  return vtkSMTrace::ActiveTracer;
}

//----------------------------------------------------------------------------
vtkStdString vtkSMTrace::StopTrace()
{
  if (!vtkSMTrace::ActiveTracer)
  {
    vtkGenericWarningMacro("Use vtkSMTrace::StartTrace() to start a trace before calling "
                           "vtkSMTrace::StopTrace().");
    return vtkStdString();
  }

  vtkSmartPointer<vtkSMTrace> active = vtkSMTrace::ActiveTracer;
  vtkSMTrace::ActiveTracer = NULL;

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject _stop_trace_internal(
    PyObject_CallMethod(active->GetTraceModule(), const_cast<char*>("_stop_trace_internal"), NULL));
  if (active->CheckForError() == false)
  {
    // no error.
    if (Py_None != _stop_trace_internal.GetPointer() && _stop_trace_internal.GetPointer() != NULL)
    {
      return vtkStdString(PyString_AsString(_stop_trace_internal));
    }
    else
    {
      vtkGenericWarningMacro("Empty trace returned!!!");
      return vtkStdString();
    }
  }
#endif
  return vtkStdString();
}

//----------------------------------------------------------------------------
vtkStdString vtkSMTrace::GetCurrentTrace()
{
  if (!vtkSMTrace::ActiveTracer)
  {
    vtkGenericWarningMacro("Use vtkSMTrace::StartTrace() to start a trace before calling "
                           "vtkSMTrace::GetCurrentTrace().");
    return vtkStdString();
  }

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkSMTrace* active = vtkSMTrace::ActiveTracer;
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject get_current_trace_output(PyObject_CallMethod(
    active->GetTraceModule(), const_cast<char*>("get_current_trace_output"), NULL));
  if (active->CheckForError() == false && get_current_trace_output)
  {
    // no error.
    return vtkStdString(PyString_AsString(get_current_trace_output));
  }
#endif
  return vtkStdString();
}

//----------------------------------------------------------------------------
void vtkSMTrace::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const vtkSmartPyObject& vtkSMTrace::GetTraceModule() const
{
  return this->Internals->TraceModule;
}

//----------------------------------------------------------------------------
const vtkSmartPyObject& vtkSMTrace::GetCreateItemFunction() const
{
  return this->Internals->CreateItemFunction;
}

//----------------------------------------------------------------------------
bool vtkSMTrace::CheckForError()
{
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkPythonScopeGilEnsurer gilEnsurer;
  PyObject* exception = PyErr_Occurred();
  if (exception)
  {
    if (PyErr_ExceptionMatches(this->Internals->UntraceableException))
    {
      // catch Untraceable exceptions. We can log them when debugging is
      // enabled.
      PyErr_Clear();
      return false;
    }
    PyErr_Print();
    PyErr_Clear();
    return true;
  }
#endif
  return false;
}

//****************************************************************************
//    vtkSMTrace::TraceItemArgs
//****************************************************************************

//----------------------------------------------------------------------------
class vtkSMTrace::TraceItemArgs::vtkInternals
{
public:
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkSmartPyObject KWArgs;
  vtkSmartPyObject PositionalArgs;
  vtkInternals() {}
  ~vtkInternals() {}

  // This method will create a new PyDict is none is already
  // created for KWArgs.
  PyObject* GetKWArgs()
  {
    if (!this->KWArgs)
    {
      this->KWArgs.TakeReference(PyDict_New());
      assert(this->KWArgs);
    }
    return this->KWArgs;
  }

  PyObject* GetPositionalArgs()
  {
    if (!this->PositionalArgs)
    {
      this->PositionalArgs.TakeReference(PyList_New(0));
      assert(this->PositionalArgs);
    }
    return this->PositionalArgs;
  }
#endif
};

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs::TraceItemArgs()
  : Internals(new vtkSMTrace::TraceItemArgs::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs::~TraceItemArgs()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* key, vtkObject* val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject keyObj(PyString_FromString(key));
    vtkSmartPyObject valObj(vtkPythonUtil::GetObjectFromPointer(val));
    assert(valObj && keyObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)key;
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* key, const char* val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject keyObj(PyString_FromString(key));
    vtkSmartPyObject valObj;
    if (val == NULL)
    {
      PyObject* none = Py_None;
      Py_INCREF(none);
      valObj.TakeReference(none);
    }
    else
    {
      valObj.TakeReference(PyString_FromString(val));
    }
    assert(keyObj && valObj);
    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)key;
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* key, int val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject keyObj(PyString_FromString(key));
    vtkSmartPyObject valObj(PyInt_FromLong(val));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)key;
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* key, double val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject keyObj(PyString_FromString(key));
    vtkSmartPyObject valObj(PyFloat_FromDouble(val));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)key;
  (void)val;
  return *this;
}
//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* key, bool val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject keyObj(PyString_FromString(key));
    vtkSmartPyObject valObj(PyBool_FromLong(val ? 1 : 0));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)key;
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(vtkObject* val)
{
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject valObj(vtkPythonUtil::GetObjectFromPointer(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* val)
{
  assert(val);
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject valObj(PyString_FromString(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(int val)
{
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject valObj(PyInt_FromLong(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(double val)
{
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject valObj(PyFloat_FromDouble(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)val;
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(bool val)
{
  if (vtkSMTrace::GetActiveTracer())
  {
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject valObj(PyBool_FromLong(val ? 1 : 0));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
  }
  (void)val;
  return *this;
}

//****************************************************************************
//    vtkSMTrace::TraceItem
//****************************************************************************

class vtkSMTrace::TraceItem::TraceItemInternals
{
public:
  vtkSmartPyObject PyItem;
};
//----------------------------------------------------------------------------
vtkSMTrace::TraceItem::TraceItem(const char* type)
  : Type(type)
  , Internals(new vtkSMTrace::TraceItem::TraceItemInternals())
{
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItem::~TraceItem()
{
// if activated, delete the item
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  vtkSMTrace* tracer = vtkSMTrace::GetActiveTracer();
  if (tracer && this->Internals->PyItem)
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject reply(
      PyObject_CallMethod(this->Internals->PyItem, const_cast<char*>("finalize"), NULL));
    tracer->CheckForError();
    tracer->InvokeEvent(vtkCommand::UpdateEvent);
  }
#endif

  delete this->Internals;
  this->Internals = NULL;

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  if (tracer)
  {
    tracer->CheckForError();
  }
#endif
}

//----------------------------------------------------------------------------
void vtkSMTrace::TraceItem::operator=(const TraceItemArgs& arguments)
{
// Create the python object and pass the arguments to it.
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter && VTK_MODULE_ENABLE_VTK_Python &&                     \
  VTK_MODULE_ENABLE_VTK_WrappingPythonCore
  if (vtkSMTrace* tracer = vtkSMTrace::GetActiveTracer())
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    assert(tracer->GetTraceModule());
    assert(tracer->GetCreateItemFunction());

    vtkSmartPyObject args(PyTuple_New(3));
    PyTuple_SET_ITEM(args.GetPointer(), 0, PyString_FromString(this->Type));
    if (arguments.Internals->PositionalArgs)
    {
      PyTuple_SET_ITEM(
        args.GetPointer(), 1, arguments.Internals->PositionalArgs.ReleaseReference());
    }
    else
    {
      PyObject* none = Py_None;
      Py_INCREF(none);
      PyTuple_SET_ITEM(args.GetPointer(), 1, none);
    }
    if (arguments.Internals->KWArgs)
    {
      PyTuple_SET_ITEM(args.GetPointer(), 2, arguments.Internals->KWArgs.ReleaseReference());
    }
    else
    {
      PyObject* none = Py_None;
      Py_INCREF(none);
      PyTuple_SET_ITEM(args.GetPointer(), 2, none);
    }
    this->Internals->PyItem.TakeReference(
      PyObject_Call(tracer->GetCreateItemFunction(), args, NULL));
    tracer->CheckForError();
  }
#endif
  (void)arguments;
}
