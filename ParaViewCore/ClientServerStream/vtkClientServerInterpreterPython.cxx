/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerInterpreterPython.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"

#include "vtkClientServerInterpreter.h"

#include "vtkClientServerStream.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

namespace
{

struct PythonCSShimData
{
  PythonCSShimData(PyObject* obj)
  {
    this->Object = obj;

    Py_INCREF(this->Object);
  }
  ~PythonCSShimData() { Py_DECREF(this->Object); }

  PyObject* Object;
};
}

//----------------------------------------------------------------------------
static void FreePythonCSShimData(void* ctx)
{
  PythonCSShimData* data = (PythonCSShimData*)ctx;

  delete data;
}

#define Py_RETURN_CHECKED(pyobj)                                                                   \
  do                                                                                               \
  {                                                                                                \
    if (pyobj)                                                                                     \
    {                                                                                              \
      return pyobj;                                                                                \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      Py_RETURN_NONE;                                                                              \
    }                                                                                              \
  } while (false)

//----------------------------------------------------------------------------
template <typename T>
static PyObject* PythonConvert(T const& value)
{
  long longValue = value;
  Py_RETURN_CHECKED(PyLong_FromLong(longValue));
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(int const& value)
{
  long longValue = value;
  Py_RETURN_CHECKED(PyInt_FromLong(longValue));
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(unsigned int const& value)
{
  unsigned long unsignedLongValue = value;
  Py_RETURN_CHECKED(PyLong_FromUnsignedLong(unsignedLongValue));
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(long long const& value)
{
  Py_RETURN_CHECKED(PyLong_FromLongLong(value));
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(unsigned long long const& value)
{
  Py_RETURN_CHECKED(PyLong_FromUnsignedLongLong(value));
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(double const& value)
{
  Py_RETURN_CHECKED(PyFloat_FromDouble(value));
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(float const& value)
{
  return PythonConvert<double>(value);
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(bool const& value)
{
  if (value)
  {
    return PyBool_FromLong(1);
  }
  else
  {
    return PyBool_FromLong(0);
  }
}

//----------------------------------------------------------------------------
template <>
PyObject* PythonConvert(vtkStdString const& value)
{
  Py_RETURN_CHECKED(PyString_FromStringAndSize(value.c_str(), value.size()));
}

//----------------------------------------------------------------------------
template <typename T>
static PyObject* MakeArrayOf(const vtkClientServerStream& msg, int arg, vtkTypeUInt32 length)
{
  T* argArray = new T[length];
  if (!msg.GetArgument(0, arg, argArray, length))
  {
    delete[] argArray;
    argArray = NULL;
    Py_RETURN_NONE;
  }

  PyObject* tuple = PyTuple_New(length);
  for (vtkTypeUInt32 i = 0; i < length; ++i)
  {
    PyTuple_SET_ITEM(tuple, i, PythonConvert<T>(argArray[i]));
  }

  delete[] argArray;
  return tuple;
}

//----------------------------------------------------------------------------
template <typename T>
static PyObject* MakeScalar(const vtkClientServerStream& msg, int arg)
{
  T argValue;
  if (!msg.GetArgument(0, arg, &argValue))
  {
    Py_RETURN_NONE;
  }

  return PythonConvert<T>(argValue);
}

//----------------------------------------------------------------------------
static vtkObjectBase* NewInstanceCallback(void* ctx)
{
  PythonCSShimData* data = (PythonCSShimData*)ctx;

  vtkSmartPyObject args(PyTuple_New(0));
  vtkSmartPyObject instance(PyObject_Call(data->Object, args, NULL));

  vtkObjectBase* obj = PyVTKObject_GetObject(instance);
  obj->Register(NULL);

  return obj;
}

//----------------------------------------------------------------------------
static void ReturnResult(PyObject* result, vtkClientServerStream& resultStream)
{
  resultStream.Reset();

  if (PyVTKObject_Check(result))
  {
    vtkObjectBase* obj = PyVTKObject_GetObject(result);
    resultStream << vtkClientServerStream::Reply << obj << vtkClientServerStream::End;
  }
  else if (PyString_Check(result))
  {
    const char* str = PyString_AsString(result);
    resultStream << vtkClientServerStream::Reply << str << vtkClientServerStream::End;
  }
  else if (PyBool_Check(result))
  {
    bool num = result == Py_True;
    resultStream << vtkClientServerStream::Reply << num << vtkClientServerStream::End;
  }
  else if (PyLong_Check(result))
  {
#if PY_VERSION_HEX >= 0x02070000
    int overflow;
    PY_LONG_LONG num = PyLong_AsLongLongAndOverflow(result, &overflow);
    if (!overflow)
    {
      resultStream << vtkClientServerStream::Reply << num << vtkClientServerStream::End;
    }
    else
    {
      unsigned PY_LONG_LONG unum = PyLong_AsUnsignedLongLong(result);
      resultStream << vtkClientServerStream::Reply << unum << vtkClientServerStream::End;
    }
#else
    PY_LONG_LONG num = PyLong_AsLongLong(result);
    if (PyErr_Occurred())
    {
      PyErr_Clear();
      unsigned PY_LONG_LONG unum = PyLong_AsUnsignedLongLong(result);
      resultStream << vtkClientServerStream::Reply << unum << vtkClientServerStream::End;
    }
    else
    {
      resultStream << vtkClientServerStream::Reply << num << vtkClientServerStream::End;
    }
#endif
  }
  else if (PyFloat_Check(result))
  {
    double num = PyFloat_AsDouble(result);
    resultStream << vtkClientServerStream::Reply << num << vtkClientServerStream::End;
  }
  else if (PyInt_Check(result))
  {
    long num = PyInt_AsLong(result);
    resultStream << vtkClientServerStream::Reply << num << vtkClientServerStream::End;
  }
  else if (PyTuple_Check(result))
  {
    Py_ssize_t size = PyTuple_Size(result);
    // Check the type of the first item to see what we should do.
    PyObject* sentinel_item = PyTuple_GET_ITEM(result, 0);

#define convert_array(convert, type)                                                               \
  do                                                                                               \
  {                                                                                                \
    type* arr = new type[size];                                                                    \
    for (Py_ssize_t i = 0; i < size; ++i)                                                          \
    {                                                                                              \
      PyObject* item = PyTuple_GET_ITEM(result, i);                                                \
      type num = convert(item);                                                                    \
      arr[i] = num;                                                                                \
    }                                                                                              \
    resultStream << vtkClientServerStream::Reply << vtkClientServerStream::InsertArray(arr, size)  \
                 << vtkClientServerStream::End;                                                    \
    delete[] arr;                                                                                  \
  } while (false)

    if (PyLong_Check(sentinel_item))
    {
      convert_array(PyLong_AsUnsignedLongLong, unsigned PY_LONG_LONG);
    }
    else if (PyInt_Check(sentinel_item))
    {
      convert_array(PyInt_AsLong, long);
    }
    else if (PyFloat_Check(sentinel_item))
    {
      convert_array(PyFloat_AsDouble, double);
    }

#undef convert_array
  }
  else
  {
    resultStream << vtkClientServerStream::Error << "Unable to convert result from Python"
                 << vtkClientServerStream::End;
  }
}

//----------------------------------------------------------------------------
static vtkStdString GetPythonErrorString()
{
  PyObject* type;
  PyObject* value;
  PyObject* traceback;

  // Increments refcounts for returns.
  PyErr_Fetch(&type, &value, &traceback);
  // place the results in vtkSmartPyObjects so the reference counts
  // are automatically decremented
  vtkSmartPyObject sType(type);
  vtkSmartPyObject sValue(value);
  vtkSmartPyObject sTraceback(traceback);

  if (!sType)
  {
    return "No error from Python?!";
  }

  vtkSmartPyObject pyexc_string(PyObject_Str(sValue));
  vtkStdString exc_string;
  if (pyexc_string)
  {
    exc_string = PyString_AsString(pyexc_string);
  }
  else
  {
    exc_string = "<Unable to convert Python error to string>";
  }

  PyErr_Clear();

  return exc_string;
}

//----------------------------------------------------------------------------
static int CommandFunctionCallback(vtkClientServerInterpreter* /*interp*/, vtkObjectBase* ptr,
  const char* method, const vtkClientServerStream& msg, vtkClientServerStream& result, void* ctx)
{
  PythonCSShimData* data = (PythonCSShimData*)ctx;

  if (!PyObject_HasAttrString(data->Object, method))
  {
    if (result.GetNumberOfMessages() > 0 && result.GetCommand(0) == vtkClientServerStream::Error &&
      result.GetNumberOfArguments(0) > 1)
    {
      /* A superclass wrapper prepared a special message. */
      return 0;
    }
    vtkOStrStreamWrapper vtkmsg;
    vtkmsg << "Object type: " << ptr->GetClassName() << ", could not find requested method: \""
           << method << "\".\n";
    result.Reset();
    result << vtkClientServerStream::Error << vtkmsg.str() << vtkClientServerStream::End;
    vtkmsg.rdbuf()->freeze(0);
    return 0;
  }

  vtkSmartPyObject vtkObject(vtkPythonUtil::GetObjectFromPointer(ptr));
  PyObject* methodObject = PyObject_GetAttrString(vtkObject, method);

  const int argOffset = 2;
  int numArgs = msg.GetNumberOfArguments(0) - argOffset;
  vtkSmartPyObject args(PyTuple_New(numArgs));

  for (Py_ssize_t i = 0; i < numArgs; ++i)
  {
    int msgArg = i + argOffset;
    int pyArg = i;
    vtkClientServerStream::Types type = msg.GetArgumentType(0, msgArg);
    vtkTypeUInt32 length;
    msg.GetArgumentLength(0, msgArg, &length);

#define array_mappings(call)                                                                       \
  call(int8, vtkTypeInt8) call(int16, vtkTypeInt16) call(int32, vtkTypeInt32)                      \
    call(int64, vtkTypeInt64) call(uint8, vtkTypeUInt8) call(uint16, vtkTypeUInt16)                \
      call(uint32, vtkTypeUInt32) call(uint64, vtkTypeUInt64) call(float32, vtkTypeFloat32)        \
        call(float64, vtkTypeFloat64)
#define scalar_mappings(call) array_mappings(call) call(bool, bool) call(string, vtkStdString)

    switch (type)
    {
#define make_array(msgType, cxxType)                                                               \
  case vtkClientServerStream::msgType##_array:                                                     \
  {                                                                                                \
    PyObject* obj = MakeArrayOf<cxxType>(msg, msgArg, length);                                     \
    PyTuple_SET_ITEM(args.GetPointer(), pyArg, obj);                                               \
    break;                                                                                         \
  }

      array_mappings(make_array)

#undef make_array

#define make_scalar(msgType, cxxType)                                                              \
  case vtkClientServerStream::msgType##_value:                                                     \
  {                                                                                                \
    PyObject* obj = MakeScalar<cxxType>(msg, msgArg);                                              \
    PyTuple_SET_ITEM(args.GetPointer(), pyArg, obj);                                               \
    break;                                                                                         \
  }

        scalar_mappings(make_scalar)

#undef make_scalar

#undef scalar_mappings
#undef array_mappings

          case vtkClientServerStream::vtk_object_pointer:
      {
        vtkObjectBase* argObj;
        if (msg.GetArgument(0, msgArg, &argObj) && argObj)
        {
          PyObject* obj = vtkPythonUtil::GetObjectFromPointer(argObj);
          PyTuple_SET_ITEM(args.GetPointer(), pyArg, obj);
        }
        else
        {
          Py_INCREF(Py_None);
          PyTuple_SET_ITEM(args.GetPointer(), pyArg, Py_None);
        }
        break;
      }
      // Streams and ID values are intended to be handled internally; they
      // should never be passed as arguments.
      case vtkClientServerStream::stream_value:
      case vtkClientServerStream::id_value:
      // If we don't recognize it, pass None as the argument.
      default:
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(args.GetPointer(), pyArg, Py_None);
        break;
    }
  }

  vtkSmartPyObject callResult(PyObject_CallObject(methodObject, args));

  if (!callResult)
  {
    vtkOStrStreamWrapper vtkmsg;
    vtkStdString pymsg = GetPythonErrorString();
    vtkmsg << "Object type: " << ptr->GetClassName() << ", failure when calling method: \""
           << method << "\": " << pymsg << ".\n";
    result.Reset();
    result << vtkClientServerStream::Error << vtkmsg.str() << vtkClientServerStream::End;
    vtkmsg.rdbuf()->freeze(0);
    return 0;
  }

  ReturnResult(callResult, result);

  return 1;
}

//----------------------------------------------------------------------------
int vtkClientServerInterpreter::LoadImpl(const char* moduleName)
{
  if (!vtkPythonInterpreter::IsInitialized())
  {
    return 1;
  }

  static std::string const moduleSuffix = "Python";
  vtkSmartPyObject module(PyImport_ImportModule((moduleName + moduleSuffix).c_str()));

  if (!module)
  {
    return 1;
  }

  vtkSmartPyObject properties(PyObject_Dir(module));
  Py_ssize_t size = PyList_Size(properties);

  for (Py_ssize_t i = 0; i < size; ++i)
  {
    PyObject* item = PyList_GetItem(properties, i);
    const char* name = PyString_AsString(item);
    if (!strncmp("vtk", name, 3))
    {
      vtkSmartPyObject cls(PyObject_GetAttr(module, item));

      PythonCSShimData* newInstanceData = new PythonCSShimData(cls);
      this->AddNewInstanceFunction(
        name, NewInstanceCallback, newInstanceData, FreePythonCSShimData);

      PythonCSShimData* commandData = new PythonCSShimData(cls);
      this->AddCommandFunction(name, CommandFunctionCallback, commandData, FreePythonCSShimData);
    }
  }

  return 0;
}
