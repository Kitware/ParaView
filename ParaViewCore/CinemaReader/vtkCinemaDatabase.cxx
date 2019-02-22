/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCinemaDatabase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h"

#include "vtkCamera.h"
#include "vtkCinemaDatabase.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>

namespace
{
std::vector<std::string> PyListToVectorOfStrings(vtkSmartPyObject pyobj)
{
  std::vector<std::string> retval;
  const Py_ssize_t len = PyList_Size(pyobj);
  for (Py_ssize_t cc = 0; cc < len; ++cc)
  {
    PyObject* obj = PyList_GetItem(pyobj, cc);
    if (obj && PyString_Check(obj))
    {
      const char* str = PyString_AsString(obj);
      retval.push_back(str);
    }
    else if (obj && PyUnicode_Check(obj))
    {
      vtkSmartPyObject objLatin1(PyUnicode_AsLatin1String(obj));
      const char* str = PyString_AsString(objLatin1);
      retval.push_back(str);
    }
    else
    {
      vtkGenericWarningMacro("Unexpected Python value!");
    }
  }
  return retval;
}

std::vector<double> PyListToVectorOfDoubles(vtkSmartPyObject pyobj)
{
  std::vector<double> retval;
  const Py_ssize_t len = PyList_Size(pyobj);
  for (Py_ssize_t cc = 0; cc < len; ++cc)
  {
    PyObject* obj = PyList_GetItem(pyobj, cc);
    if (obj && PyInt_Check(obj))
    {
      double val = static_cast<double>(PyInt_AsLong(obj));
      retval.push_back(val);
    }
    else if (obj && PyFloat_Check(obj))
    {
      double val = static_cast<double>(PyFloat_AsDouble(obj));
      retval.push_back(val);
    }
    else
    {
      vtkGenericWarningMacro("Unexpected Python value!");
    }
  }
  return retval;
}

template <class T>
std::vector<vtkSmartPointer<T> > PyListToVectorOfVTKObjects(vtkSmartPyObject pyobj)
{
  std::vector<vtkSmartPointer<T> > layers;
  if (PyList_Check(pyobj))
  {
    const Py_ssize_t len = PyList_Size(pyobj);
    for (Py_ssize_t cc = 0; cc < len; ++cc)
    {
      PyObject* obj = PyList_GetItem(pyobj, cc);
      T* img = T::SafeDownCast(vtkPythonUtil::GetPointerFromObject(obj, "vtkObjectBase"));
      if (img)
      {
        layers.push_back(img);
      }
    }
  }
  return layers;
}
}

class vtkCinemaDatabase::vtkInternals
{
  bool Initialized;
  std::string OldFileName;

  vtkSmartPyObject CinemaReaderModule;
  vtkSmartPyObject FileStore;

public:
  vtkInternals()
    : Initialized(false)
  {
  }

  bool IsLoaded() const { return this->FileStore; }

  // Will import necessary Python modules and return true if all's ready.
  bool InitializePython()
  {
    if (!this->Initialized)
    {
      this->Initialized = true;
      vtkPythonInterpreter::Initialize();
      vtkPythonScopeGilEnsurer gilEnsurer;
      this->CinemaReaderModule.TakeReference(
        PyImport_ImportModule("paraview.tpl.cinema_python.adaptors.paraview.cinemareader"));
      if (!this->CinemaReaderModule)
      {
        vtkGenericWarningMacro(
          "Failed to import 'paraview.tpl.cinema_python.adaptors.paraview.cinemareader' module.");
        if (PyErr_Occurred())
        {
          PyErr_Print();
          PyErr_Clear();
        }
        return false;
      }
    }
    return this->CinemaReaderModule;
  }

  // Load a database.
  bool LoadDatabase(const char* filename)
  {
    if (!this->InitializePython())
    {
      return false;
    }

    vtkPythonScopeGilEnsurer gilEnsurer;
    assert(filename != NULL || filename[0] != 0);
    if (this->CinemaReaderModule && (!this->FileStore || (this->OldFileName != filename)))
    {
      vtkSmartPyObject argList(Py_BuildValue("(s)", filename));
      this->FileStore.TakeReference(PyObject_CallMethod(
        this->CinemaReaderModule, const_cast<char*>("load"), const_cast<char*>("s"), filename));
      if (!this->FileStore)
      {
        if (PyErr_ExceptionMatches(PyExc_RuntimeError))
        {
          vtkGenericWarningMacro("This Cinema store is not supported. "
                                 "Only 'composite-image-stack' aka Spec-C file stores "
                                 "with 'azimuth-elevation-roll' aka inward facing pose cameras "
                                 "are supported.")
        }
        else
        {
          vtkGenericWarningMacro("Failed to instantiate a 'FileStore'.");
          PyErr_Print();
        }
        PyErr_Clear();
        return false;
      }
      this->OldFileName = filename;
    }
    return this->FileStore;
  }

  std::vector<std::string> GetPipelineObjects() const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(
      PyObject_CallMethod(this->FileStore, const_cast<char*>("get_objects"), NULL));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfStrings(retVal);
    }
    return std::vector<std::string>();
  }

  std::vector<std::string> GetPipelineObjectParents(const std::string& name) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(
      this->FileStore, const_cast<char*>("get_parents"), const_cast<char*>("s"), name.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfStrings(retVal);
    }
    return std::vector<std::string>();
  }

  bool GetPipelineObjectVisibility(const std::string& name) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(
      this->FileStore, const_cast<char*>("get_visibility"), const_cast<char*>("s"), name.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyBool_Check(retVal))
    {
      return (retVal == Py_True);
    }
    return false;
  }

  std::vector<std::string> GetControlParameters(const std::string& name) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(this->FileStore,
      const_cast<char*>("get_control_parameters"), const_cast<char*>("s"), name.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfStrings(retVal);
    }
    return std::vector<std::string>();
  }

  std::string GetFieldName(const std::string& objectname) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(this->FileStore,
      const_cast<char*>("get_field_name"), const_cast<char*>("s"), objectname.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    if (PyString_Check(retVal))
    {
      std::string str(PyString_AsString(retVal));
      return str;
    }
    else if (PyUnicode_Check(retVal))
    {
      vtkSmartPyObject objLatin1(PyUnicode_AsLatin1String(retVal));
      std::string str(PyString_AsString(objLatin1));
      return str;
    }
    return std::string();
  }

  std::vector<std::string> GetFieldValues(
    const std::string& name, const std::string& valuetype) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(
      PyObject_CallMethod(this->FileStore, const_cast<char*>("get_field_values"),
        const_cast<char*>("ss"), name.c_str(), valuetype.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfStrings(retVal);
    }
    return std::vector<std::string>();
  }
  bool GetFieldValueRange(
    const std::string& object, const std::string& field, double range[2]) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(
      PyObject_CallMethod(this->FileStore, const_cast<char*>("get_field_valuerange"),
        const_cast<char*>("ss"), object.c_str(), field.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      const std::vector<double> v = PyListToVectorOfDoubles(retVal);
      if (v.size() == 2)
      {
        range[0] = v[0];
        range[1] = v[1];
        return true;
      }
    }
    return false;
  }

  std::vector<std::string> GetControlParameterValues(const std::string& name) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(this->FileStore,
      const_cast<char*>("get_control_values_as_strings"), const_cast<char*>("s"), name.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfStrings(retVal);
    }
    return std::vector<std::string>();
  }

  std::vector<double> GetControlParameterValuesAsDouble(const std::string& name) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(this->FileStore,
      const_cast<char*>("get_control_values"), const_cast<char*>("s"), name.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfDoubles(retVal);
    }
    return std::vector<double>();
  }

  std::vector<std::string> GetTimeSteps() const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(
      PyObject_CallMethod(this->FileStore, const_cast<char*>("get_timesteps"), NULL));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    else if (PyList_Check(retVal))
    {
      return PyListToVectorOfStrings(retVal);
    }
    return std::vector<std::string>();
  }

  std::vector<vtkSmartPointer<vtkImageData> > TranslateQuery(const std::string& query) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(this->FileStore,
      const_cast<char*>("translate_query"), const_cast<char*>("s"), query.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
      return std::vector<vtkSmartPointer<vtkImageData> >();
    }
    else
    {
      return PyListToVectorOfVTKObjects<vtkImageData>(retVal);
    }
  }

  std::vector<vtkSmartPointer<vtkCamera> > Cameras(const std::string& ts) const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(PyObject_CallMethod(
      this->FileStore, const_cast<char*>("get_cameras"), const_cast<char*>("s"), ts.c_str()));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
      return std::vector<vtkSmartPointer<vtkCamera> >();
    }
    else
    {
      return PyListToVectorOfVTKObjects<vtkCamera>(retVal);
    }
  }

  std::string GetSpec() const
  {
    vtkPythonScopeGilEnsurer gilEnsurer;
    vtkSmartPyObject retVal(
      PyObject_CallMethod(this->FileStore, const_cast<char*>("get_spec"), NULL));
    if (!retVal)
    {
      PyErr_Print();
      PyErr_Clear();
    }
    if (PyString_Check(retVal))
    {
      return std::string(PyString_AsString(retVal));
    }
    else if (PyUnicode_Check(retVal))
    {
      vtkSmartPyObject objLatin1(PyUnicode_AsLatin1String(retVal));
      return std::string(PyString_AsString(objLatin1));
    }
    return std::string();
  }
};

vtkStandardNewMacro(vtkCinemaDatabase);
//----------------------------------------------------------------------------
vtkCinemaDatabase::vtkCinemaDatabase()
{
  this->Internals = new vtkCinemaDatabase::vtkInternals();
}

//----------------------------------------------------------------------------
vtkCinemaDatabase::~vtkCinemaDatabase()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
bool vtkCinemaDatabase::Load(const char* fname)
{
  if (fname && fname[0] != 0)
  {
    return this->Internals->LoadDatabase(fname);
  }

  return false;
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCinemaDatabase::GetPipelineObjects() const
{
  return this->Internals->IsLoaded() ? this->Internals->GetPipelineObjects()
                                     : std::vector<std::string>();
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCinemaDatabase::GetPipelineObjectParents(const std::string& name) const
{
  return this->Internals->IsLoaded() ? this->Internals->GetPipelineObjectParents(name)
                                     : std::vector<std::string>();
}

//----------------------------------------------------------------------------
bool vtkCinemaDatabase::GetPipelineObjectVisibility(const std::string& objectname) const
{
  return this->Internals->IsLoaded() ? this->Internals->GetPipelineObjectVisibility(objectname)
                                     : false;
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCinemaDatabase::GetControlParameters(const std::string& name) const
{
  return this->Internals->IsLoaded() ? this->Internals->GetControlParameters(name)
                                     : std::vector<std::string>();
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCinemaDatabase::GetControlParameterValues(const std::string& name) const
{
  if (!this->Internals->IsLoaded())
  {
    return std::vector<std::string>();
  }

  std::vector<std::string> parameters = this->GetControlParameters(name);
  if (std::find(parameters.begin(), parameters.end(), name) != parameters.end())
  {
    return this->Internals->GetControlParameterValues(name);
  }

  return std::vector<std::string>();
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCinemaDatabase::GetTimeSteps() const
{
  return this->Internals->IsLoaded() ? this->Internals->GetTimeSteps() : std::vector<std::string>();
}

//----------------------------------------------------------------------------
std::string vtkCinemaDatabase::GetFieldName(const std::string& objectname) const
{
  return this->Internals->IsLoaded() ? this->Internals->GetFieldName(objectname) : std::string();
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkCinemaDatabase::GetFieldValues(
  const std::string& objectname, const std::string& valuetype) const
{
  return this->Internals->IsLoaded() ? this->Internals->GetFieldValues(objectname, valuetype)
                                     : std::vector<std::string>();
}

//----------------------------------------------------------------------------
bool vtkCinemaDatabase::GetFieldValueRange(
  const std::string& object, const std::string& field, double range[2]) const
{
  return this->Internals->IsLoaded() ? this->Internals->GetFieldValueRange(object, field, range)
                                     : false;
}

//----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkImageData> > vtkCinemaDatabase::TranslateQuery(
  const std::string& query) const
{
  return this->Internals->IsLoaded() ? this->Internals->TranslateQuery(query)
                                     : std::vector<vtkSmartPointer<vtkImageData> >();
}

//----------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkCamera> > vtkCinemaDatabase::Cameras(
  const std::string& timestep) const
{
  return this->Internals->IsLoaded() ? this->Internals->Cameras(timestep)
                                     : std::vector<vtkSmartPointer<vtkCamera> >();
}

//----------------------------------------------------------------------------
int vtkCinemaDatabase::GetSpec() const
{
  std::string spec = this->Internals->IsLoaded() ? this->Internals->GetSpec() : std::string("");
  int res = Spec::UNKNOWN;
  if (spec == "specA")
  {
    res = Spec::CINEMA_SPEC_A;
  }
  else if (spec == "specC")
  {
    res = Spec::CINEMA_SPEC_C;
  }
  return res;
}

//----------------------------------------------------------------------------
std::string vtkCinemaDatabase::GetNearestParameterValue(
  const std::string& param, double value) const
{
  if (!this->Internals->IsLoaded())
  {
    return std::string();
  }

  std::vector<double> values = this->Internals->GetControlParameterValuesAsDouble(param);
  std::vector<double>::iterator valIterator;
  valIterator = std::lower_bound(values.begin(), values.end(), value);

  double result = value;
  if (*valIterator != value)
  {
    if (valIterator == values.begin())
    {
      // value is lower than all values, take the first
      result = *valIterator;
    }
    else if (valIterator == values.end())
    {
      // value is higher than all, take the last
      --valIterator;
      result = *valIterator;
    }
    else
    {
      // take the nearest
      double upper = *valIterator;
      --valIterator;
      double lower = *valIterator;
      result = std::abs(result - lower) > std::abs(upper - result) ? upper : lower;
    }
  }

  std::ostringstream ostr;
  ostr << result;
  return ostr.str();
}

//----------------------------------------------------------------------------
void vtkCinemaDatabase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
