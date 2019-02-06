/*=========================================================================

  Program:   ParaView
  Module:    vtkSIPythonSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // must be 1st include.

#include "vtkSIPythonSourceProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkDataObjectAlgorithm.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPythonAlgorithmPlugin.h" // needed to ensure init in static builds.
#include "vtkPythonAlgorithm.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSMMessage.h"
#include "vtkSmartPyObject.h"

#include <vtksys/SystemTools.hxx>

#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace
{

inline void SafePyErrorPrintAndClear()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (PyErr_Occurred() != nullptr)
  {
    PyErr_Print();
    PyErr_Clear();
  }
}

inline void SafePyErrorClear()
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (PyErr_Occurred() != nullptr)
  {
    PyErr_Clear();
  }
}

class create_error : public std::exception
{
};
class convert_error : public std::exception
{
};

// push contents of return value from a vtkSmartPyObject to a
// vtkClientServerStream.
vtkClientServerStream& operator<<(vtkClientServerStream& os, vtkSmartPyObject& obj)
{
  if (PyBool_Check(obj))
  {
    os << ((obj.GetPointer() == Py_True) ? true : false);
  }
  else if (PyInt_Check(obj))
  {
    os << PyInt_AsLong(obj);
  }
  else if (PyLong_Check(obj))
  {
    os << PyLong_AsLong(obj);
  }
  else if (PyFloat_Check(obj))
  {
    os << PyFloat_AsDouble(obj);
  }
  else if (PyString_Check(obj))
  {
    os << PyString_AsString(obj);
  }
  else if (PyVTKObject_Check(obj))
  {
    os << PyVTKObject_GetObject(obj);
  }
  else
  {
    throw convert_error();
  }
  return os;
}

template <typename T, typename F>
vtkSmartPyObject convert_value(const vtkClientServerStream& msg, int idx, int arg, F f)
{
  T cval;
  if (!msg.GetArgument(idx, arg, &cval))
  {
    throw convert_error();
  }
  return vtkSmartPyObject(f(cval));
}

// Converts the argument index given by `arg`, from a `msg` at index `idx` using.
// Handles both single valued arguments and arrays. For arrays, it creates a
// python-list. `f` is a functor that is called to convert a single-value of
// type T, to the PyObject. This function will **steal** the reference from the
// returned PyObject.
//
// Throws convert_error on failure.
template <typename T, typename F>
vtkSmartPyObject convert_value_or_array(const vtkClientServerStream& msg, int idx, int arg, F f)
{
  vtkTypeUInt32 length = 0;
  if (msg.GetArgumentLength(idx, arg, &length))
  {
    std::unique_ptr<T[]> cval(new T[length]);
    if (!msg.GetArgument(idx, arg, cval.get(), length))
    {
      throw convert_error();
    }

    vtkSmartPyObject pylist(PyList_New(length));
    for (vtkTypeUInt32 cc = 0; cc < length; ++cc)
    {
      PyList_SET_ITEM(pylist.GetPointer(), cc, f(cval[cc]));
    }
    return pylist;
  }
  else
  {
    return convert_value<T, F>(msg, idx, arg, f);
  }
}

// converts arguments from msg in the range (2, max) to a tuple of PyObjects.
// throws convert_error on error.
vtkSmartPyObject ConvertCSArgsToPyTuple(const vtkClientServerStream& msg)
{
  std::vector<vtkSmartPyObject> args;

  // first 2 args are the object and method and hence should be skipped.
  for (int cc = 2, max = msg.GetNumberOfArguments(0); cc < max; ++cc)
  {
    switch (msg.GetArgumentType(0, cc))
    {
      case vtkClientServerStream::int8_array:
      case vtkClientServerStream::int8_value:
      case vtkClientServerStream::int16_array:
      case vtkClientServerStream::int16_value:
      case vtkClientServerStream::int32_array:
      case vtkClientServerStream::int32_value:
      case vtkClientServerStream::uint8_array:
      case vtkClientServerStream::uint8_value:
      case vtkClientServerStream::uint16_array:
      case vtkClientServerStream::uint16_value:
      case vtkClientServerStream::uint32_array:
      case vtkClientServerStream::uint32_value:
        args.push_back(convert_value_or_array<vtkTypeInt32>(
          msg, 0, cc, [](const vtkTypeInt32& cval) { return PyInt_FromLong(cval); }));
        break;

      case vtkClientServerStream::int64_value:
      case vtkClientServerStream::int64_array:
        args.push_back(convert_value_or_array<vtkTypeInt64>(
          msg, 0, cc, [](const vtkTypeInt64& cval) { return PyLong_FromLongLong(cval); }));
        break;

      case vtkClientServerStream::uint64_value:
      case vtkClientServerStream::uint64_array:
        args.push_back(convert_value_or_array<vtkTypeUInt64>(
          msg, 0, cc, [](const vtkTypeUInt64& cval) { return PyLong_FromUnsignedLongLong(cval); }));
        break;

      case vtkClientServerStream::float32_value:
      case vtkClientServerStream::float32_array:
        args.push_back(convert_value_or_array<float>(msg, 0, cc,
          [](const float& cval) { return PyFloat_FromDouble(static_cast<double>(cval)); }));
        break;

      case vtkClientServerStream::float64_value:
      case vtkClientServerStream::float64_array:
        args.push_back(convert_value_or_array<double>(
          msg, 0, cc, [](const double& cval) { return PyFloat_FromDouble(cval); }));
        break;

      case vtkClientServerStream::bool_value:
        args.push_back(convert_value<bool>(msg, 0, cc, [](const bool& cval) {
          vtkSmartPyObject obj;
          obj = cval ? Py_True : Py_False;
          return obj;
        }));
        break;

      case vtkClientServerStream::string_value:
        args.push_back(convert_value<std::string>(msg, 0, cc, [](const std::string& cval) {
          vtkSmartPyObject obj;
          if (cval.empty())
          {
            obj = Py_None;
          }
          else
          {
            obj.TakeReference(PyString_FromStringAndSize(cval.c_str(), cval.size()));
          }

          return obj;
        }));
        break;

      case vtkClientServerStream::id_value:
        args.push_back(convert_value<vtkClientServerID>(msg, 0, cc,
          [](const vtkClientServerID& cval) { return PyLong_FromUnsignedLong(cval.ID); }));
        break;

      case vtkClientServerStream::vtk_object_pointer:
        args.push_back(convert_value<vtkObjectBase*>(msg, 0, cc, [](vtkObjectBase* cval) {
          // note: this method returns a new reference.
          return vtkPythonUtil::GetObjectFromPointer(cval);
        }));
        break;

      default:
        throw convert_error();
    }
  }

  vtkSmartPyObject tuple(PyTuple_New(static_cast<Py_ssize_t>(args.size())));
  for (size_t cc = 0; cc < args.size(); ++cc)
  {
    PyTuple_SET_ITEM(tuple.GetPointer(), cc, args[cc].ReleaseReference());
  }

  return tuple;
}

int vtkCustomPythonAlgorithmCommand(vtkClientServerInterpreter*, vtkObjectBase* ob,
  const char* method, const vtkClientServerStream& msg, vtkClientServerStream& resultStream,
  void* /*ctx*/)
{
  vtkPythonAlgorithm* algo = vtkPythonAlgorithm::SafeDownCast(ob);
  if (!algo)
  {
    std::ostringstream vtkmsg;
    vtkmsg << "Cannot cast " << ob->GetClassName() << " object to vtkPythonAlgorithm.  "
           << "This probably means the class specifies the incorrect superclass in vtkTypeMacro.";
    resultStream.Reset();
    resultStream << vtkClientServerStream::Error << vtkmsg.str() << 0 << vtkClientServerStream::End;
    return 0;
  }

  // this is a little funky, we'll simply forward the command to Python interp.
  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject result;
  try
  {
    vtkSmartPyObject vtkself(vtkPythonUtil::GetObjectFromPointer(algo));
    vtkSmartPyObject pyargs(ConvertCSArgsToPyTuple(msg));
    vtkSmartPyObject pymethod(PyObject_GetAttrString(vtkself, method));
    if (!pymethod)
    {
      // this ensures that we can call non-existent methods silently, if that's
      // what was intended.
      SafePyErrorClear();
      std::ostringstream vtkmsg;
      vtkmsg << "No method named `" << method << "` found.";
      resultStream.Reset();
      resultStream << vtkClientServerStream::Error << vtkmsg.str() << vtkClientServerStream::End;
      return 0;
    }
    result.TakeReference(PyObject_Call(pymethod.GetPointer(), pyargs.GetPointer(), nullptr));
    if (result.GetPointer() == nullptr)
    {
      SafePyErrorPrintAndClear();
      return 1; // don't want the error to be fatal.
    }
  }
  catch (convert_error&)
  {
    std::ostringstream vtkmsg;
    vtkmsg << "Failed to convert arguments to pass on vtkPythonAlgorithm for '" << method << "'.";
    resultStream.Reset();
    resultStream << vtkClientServerStream::Error << vtkmsg.str() << vtkClientServerStream::End;
    return 0;
  }

  // convert result to vtkClientServerStream.
  resultStream.Reset();
  if (result.GetPointer() == Py_None)
  {
    // all's well.
    return 1;
  }
  else
  {
    try
    {
      resultStream << vtkClientServerStream::Reply << result << vtkClientServerStream::End;
      return 1;
    }
    catch (convert_error&)
    {
      std::ostringstream vtkmsg;
      vtkmsg << "Failed to convert return value from '" << method << "'.";
      resultStream.Reset();
      resultStream << vtkClientServerStream::Error << vtkmsg.str() << vtkClientServerStream::End;
      return 0;
    }
  }
}
}

class vtkSIPythonSourceProxy::vtkInternals
{
public:
  vtkSmartPyObject Module;
  vtkSmartPyObject VTKObject;
};

vtkStandardNewMacro(vtkSIPythonSourceProxy);
//----------------------------------------------------------------------------
vtkSIPythonSourceProxy::vtkSIPythonSourceProxy()
  : Internals(nullptr)
  , ReimportModules(false)
  , Dummy()
{
  // We want to make sure that we "delete" the Python object on exit.
  // This makes for easier cleanup in Python code since `Py_Finalize()`
  // may not call the `__del__` method on the object held.
  this->Dummy->AddObserver(vtkCommand::ExitEvent, this, &vtkSIPythonSourceProxy::OnPyFinalize);
}

//----------------------------------------------------------------------------
vtkSIPythonSourceProxy::~vtkSIPythonSourceProxy()
{
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::OnPyFinalize()
{
  this->Internals.reset();
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::Initialize(vtkPVSessionCore* session)
{
  this->Superclass::Initialize(session);
  if (vtkClientServerInterpreter* interp = this->GetInterpreter())
  {
    if (!interp->HasCommandFunction("vtkPythonAlgorithm"))
    {
      interp->AddCommandFunction("vtkPythonAlgorithm", vtkCustomPythonAlgorithmCommand);
    }
  }
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSIPythonSourceProxy::NewVTKObject(const char* className)
{
  vtkPythonInterpreter::Initialize();
  std::string module = vtksys::SystemTools::GetFilenameWithoutLastExtension(className);
  std::string classname = vtksys::SystemTools::GetFilenameLastExtension(className);
  // remove the leading ".".
  classname.erase(classname.begin());

  this->Internals.reset(new vtkSIPythonSourceProxy::vtkInternals());
  try
  {
    vtkInternals internals;
    vtkPythonScopeGilEnsurer gilEnsurer;
    internals.Module.TakeReference(PyImport_ImportModule(module.c_str()));
    if (!internals.Module)
    {
      vtkErrorMacro("Failed to import module '" << module.c_str() << "'.");
      throw create_error();
    }

    if (this->ReimportModules)
    {
      vtkSmartPyObject newmodule(PyImport_ReloadModule(internals.Module));
      if (!newmodule)
      {
        // this could be from a plugin, in which case, the reload is a little
        // tricky.
        vtkSmartPyObject pvdetail(PyImport_ImportModule("paraview.detail.pythonalgorithm"));
        if (pvdetail)
        {
          vtkSmartPyObject reload_plugin_module(
            PyObject_GetAttrString(pvdetail, "reload_plugin_module"));
          if (reload_plugin_module)
          {
            newmodule.TakeReference(PyObject_CallFunctionObjArgs(
              reload_plugin_module, internals.Module.GetPointer(), nullptr));
          }
          else
          {
            // silently clear the AttributeError raised by
            // `PyObject_GetAttrString`
            SafePyErrorClear();
          }
        }
      }

      if (newmodule)
      {
        internals.Module = newmodule;
      }
      else
      {
        vtkWarningMacro("Failed to re-import module '" << module.c_str() << "'.");
      }
    }

    vtkSmartPyObject classPyObject;
    classPyObject.TakeReference(PyObject_GetAttrString(internals.Module, classname.c_str()));
    if (!classPyObject)
    {
      vtkErrorMacro("Failed to locate class '" << classname.c_str() << "' in module '"
                                               << module.c_str() << "'.");
      throw create_error();
    }

    internals.VTKObject.TakeReference(PyObject_CallObject(classPyObject, nullptr));
    if (!internals.VTKObject)
    {
      vtkErrorMacro("Failed to create object '" << className << "'.");
      throw create_error();
    }

    if (vtkObjectBase* obj =
          vtkPythonUtil::GetPointerFromObject(internals.VTKObject, "vtkAlgorithm"))
    {
      (*this->Internals) = internals;
      obj->Register(this);
      return obj;
    }
  }
  catch (create_error&)
  {
    SafePyErrorPrintAndClear();
    return vtkDataObjectAlgorithm::New();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::DeleteVTKObjects()
{
  this->Superclass::DeleteVTKObjects();
  this->Internals.reset(nullptr);
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::RecreateVTKObjects()
{
  this->ReimportModules = true;
  this->Superclass::RecreateVTKObjects();
  this->ReimportModules = false;
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::Push(vtkSMMessage* msg)
{
  // if Internals is non-null & Internals->VTKObject is null, it indicates a
  // failure to create the request Python object.
  if (this->Internals == nullptr || this->Internals->VTKObject)
  {
    this->Superclass::Push(msg);
  }
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::Pull(vtkSMMessage* msg)
{
  // if Internals is non-null & Internals->VTKObject is null, it indicates a
  // failure to create the request Python object.
  if (this->Internals == nullptr || this->Internals->VTKObject)
  {
    this->Superclass::Pull(msg);
  }
}

//----------------------------------------------------------------------------
void vtkSIPythonSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
