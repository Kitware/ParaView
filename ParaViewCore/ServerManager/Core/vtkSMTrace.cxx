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
#include "vtkPVConfig.h" // needed for PARAVIEW_ENABLE_PYTHON
#ifdef PARAVIEW_ENABLE_PYTHON
# include "vtkPython.h"
# include "vtkPythonInterpreter.h"
# include "vtkPythonUtil.h"
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
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"

#include <sstream>
#include <map>
#include <cassert>

#ifdef PARAVIEW_ENABLE_PYTHON
// Smart pointer for PyObjects. Calls Py_XDECREF when scope ends.
class SmartPyObject
{
  PyObject *Object;

public:
  SmartPyObject(PyObject *obj = NULL)
    : Object(obj)
    {
    }
  ~SmartPyObject()
    {
    Py_XDECREF(this->Object);
    }
  PyObject *operator->() const
    {
    return this->Object;
    }
  PyObject *GetPointer() const
    {
    return this->Object;
    }
  operator bool () const
    {
    return this->Object != NULL;
    }
  operator PyObject* () const
    {
    return this->Object;
    }

  void TakeReference(PyObject* obj)
    {
    if (this->Object)
      {
      Py_DECREF(this->Object);
      }
    this->Object = obj;
    }
  PyObject* ReleaseReference()
    {
    PyObject* ret = this->Object;
    this->Object = NULL;
    return ret;
    }
private:
  SmartPyObject(const SmartPyObject&);
  void operator=(const SmartPyObject&) const;

};
#else
class SmartPyObject
{
public:
  operator bool () const
    {
    return false;
    }
};
#endif

class vtkSMTrace::vtkInternals
{
public:
  SmartPyObject TraceModule;
  SmartPyObject CreateItemFunction;
  SmartPyObject UntraceableException;
};

vtkSmartPointer<vtkSMTrace> vtkSMTrace::ActiveTracer;
vtkStandardNewMacro(vtkSMTrace);
//----------------------------------------------------------------------------
vtkSMTrace::vtkSMTrace():
  TraceXMLDefaults(false),
  LogTraceToStdout(true),
  PropertiesToTraceOnCreate(vtkSMTrace::RECORD_MODIFIED_PROPERTIES),
  TracePropertiesOnExistingProxies(false),
  Internals(new vtkSMTrace::vtkInternals())
{
#ifdef PARAVIEW_ENABLE_PYTHON
  // ensure Python interpreter is initialized.
  vtkPythonInterpreter::Initialize();
  this->Internals->TraceModule.TakeReference(PyImport_ImportModule("paraview.smtracer"));
  if (!this->Internals->TraceModule)
    {
    vtkErrorMacro("Failed to import paraview.smtracer module.");
    }
  else
    {
    this->Internals->CreateItemFunction.TakeReference(
      PyObject_GetAttrString(this->Internals->TraceModule, "createTraceItem"));
    if (!this->Internals->CreateItemFunction)
      {
      vtkErrorMacro("Failed to locate the createTraceItem function in paraview.smtracer module.");
      this->Internals->TraceModule.TakeReference(NULL);
      }
    this->Internals->UntraceableException.TakeReference(
      PyObject_GetAttrString(this->Internals->TraceModule, "Untraceable"));
    if (!this->Internals->UntraceableException)
      {
      vtkErrorMacro("Failed to locate the Untraceable exception class in paraview.smtracer module.");
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
vtkSMTrace* vtkSMTrace::StartTrace()
{
  if (vtkSMTrace::ActiveTracer.GetPointer() == NULL)
    {
    vtkSMTrace::ActiveTracer = vtkSmartPointer<vtkSMTrace>::New();
#ifdef PARAVIEW_ENABLE_PYTHON
    if (!vtkSMTrace::ActiveTracer->Internals->TraceModule)
      {
      vtkGenericWarningMacro("Start not started since required Python modules are missing.");
      vtkSMTrace::ActiveTracer = NULL;
      }
    else
      {
      SmartPyObject startTrace(
        PyObject_CallMethod(vtkSMTrace::ActiveTracer->GetTraceModule(),
          const_cast<char*>("startTrace"), NULL));
      vtkSMTrace::ActiveTracer->CheckForError();
      }
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

#ifdef PARAVIEW_ENABLE_PYTHON
  SmartPyObject stopTrace(
    PyObject_CallMethod(active->GetTraceModule(), const_cast<char*>("stopTrace"), NULL));
  if (active->CheckForError() == false)
    {
    // no error.
    if (Py_None != stopTrace.GetPointer() && stopTrace.GetPointer() != NULL)
      {
      return vtkStdString(PyString_AsString(stopTrace));
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

  vtkSMTrace* active = vtkSMTrace::ActiveTracer;
#ifdef PARAVIEW_ENABLE_PYTHON
  SmartPyObject getTrace(
    PyObject_CallMethod(active->GetTraceModule(), const_cast<char*>("getTrace"), NULL));
  if (active->CheckForError() == false)
    {
    // no error.
    return vtkStdString(PyString_AsString(getTrace));
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
const SmartPyObject& vtkSMTrace::GetTraceModule() const
{
  return this->Internals->TraceModule;
}

//----------------------------------------------------------------------------
const SmartPyObject& vtkSMTrace::GetCreateItemFunction() const
{
  return this->Internals->CreateItemFunction;
}

//----------------------------------------------------------------------------
bool vtkSMTrace::CheckForError()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  PyObject *exception = PyErr_Occurred();
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
#ifdef PARAVIEW_ENABLE_PYTHON
  SmartPyObject KWArgs;
  SmartPyObject PositionalArgs;
  vtkInternals()
    {
    }
  ~vtkInternals()
    {
    }

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
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(
  const char* key, vtkObject* val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject keyObj(PyString_FromString(key));
    SmartPyObject valObj(vtkPythonUtil::GetObjectFromPointer(val));
    assert(valObj && keyObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(
  const char* key, const char* val)
{
  assert(key && val);
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject keyObj(PyString_FromString(key));
    SmartPyObject valObj(PyString_FromString(val));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(
  const char* key, int val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject keyObj(PyString_FromString(key));
    SmartPyObject valObj(PyInt_FromLong(val));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(
  const char* key, double val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject keyObj(PyString_FromString(key));
    SmartPyObject valObj(PyFloat_FromDouble(val));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}
//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(
  const char* key, bool val)
{
  assert(key);
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject keyObj(PyString_FromString(key));
    SmartPyObject valObj(PyBool_FromLong(val? 1 : 0));
    assert(keyObj && valObj);

    int ret = PyDict_SetItem(this->Internals->GetKWArgs(), keyObj, valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(vtkObject* val)
{
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject valObj(vtkPythonUtil::GetObjectFromPointer(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(const char* val)
{
  assert(val);
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject valObj(PyString_FromString(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(int val)
{
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject valObj(PyInt_FromLong(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(double val)
{
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject valObj(PyFloat_FromDouble(val));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItemArgs& vtkSMTrace::TraceItemArgs::arg(bool val)
{
  if (vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    SmartPyObject valObj(PyBool_FromLong(val? 1 : 0));
    assert(valObj);
    int ret = PyList_Append(this->Internals->GetPositionalArgs(), valObj);
    (void)ret;
    assert(ret == 0);
#endif
    }
  return *this;
}


//****************************************************************************
//    vtkSMTrace::TraceItem
//****************************************************************************

class vtkSMTrace::TraceItem::TraceItemInternals
{
public:
  SmartPyObject PyItem;
};
//----------------------------------------------------------------------------
vtkSMTrace::TraceItem::TraceItem(const char* type)
  : Type(type),
  Internals(new vtkSMTrace::TraceItem::TraceItemInternals())
{
}

//----------------------------------------------------------------------------
vtkSMTrace::TraceItem::~TraceItem()
{
  // if activated, delete the item
#ifdef PARAVIEW_ENABLE_PYTHON
  vtkSMTrace* tracer = vtkSMTrace::GetActiveTracer();
  if (tracer && this->Internals->PyItem)
    {
    SmartPyObject reply(
      PyObject_CallMethod(this->Internals->PyItem,
        const_cast<char*>("finalize"), NULL));
    tracer->CheckForError();
    tracer->InvokeEvent(vtkCommand::UpdateEvent);
    }
#endif

  delete this->Internals;
  this->Internals = NULL;

#ifdef PARAVIEW_ENABLE_PYTHON
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
  if (vtkSMTrace* tracer = vtkSMTrace::GetActiveTracer())
    {
#ifdef PARAVIEW_ENABLE_PYTHON
    assert(tracer->GetTraceModule());
    assert(tracer->GetCreateItemFunction());

    SmartPyObject args(PyTuple_New(3));
    PyTuple_SET_ITEM(args.GetPointer(), 0, PyString_FromString(this->Type));
    if (arguments.Internals->PositionalArgs)
      {
      PyTuple_SET_ITEM(args.GetPointer(), 1,
        arguments.Internals->PositionalArgs.ReleaseReference());
      }
    else
      {
      PyObject* none = Py_None;
      Py_INCREF(none);
      PyTuple_SET_ITEM(args.GetPointer(), 1, none);
      }
    if (arguments.Internals->KWArgs)
      {
      PyTuple_SET_ITEM(args.GetPointer(), 2,
        arguments.Internals->KWArgs.ReleaseReference());
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
#endif
    }
}
