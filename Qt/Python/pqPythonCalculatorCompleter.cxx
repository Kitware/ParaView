// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonCalculatorCompleter.h"

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPython.h"
#include "vtkPythonInterpreter.h"

#include <QRegularExpression>

QList<pqPythonCalculatorCompleter::FunctionInfo> pqPythonCalculatorCompleter::getFunctionInfos(
  const QStringList& ignoredModules)
{
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  PyObject* calcModule = PyImport_ImportModule("paraview.detail.calculator");
  if (!calcModule)
  {
    return {};
  }

  PyObject* dict = PyModule_GetDict(calcModule); // borrowed reference
  QList<FunctionInfo> results;
  PyObject* key;
  PyObject* value;
  Py_ssize_t pos = 0;
  while (PyDict_Next(dict, &pos, &key, &value))
  {
    // skip not callable types values and object types
    if (!PyCallable_Check(value) || PyType_Check(value))
    {
      continue;
    }
    const char* name = PyUnicode_AsUTF8(key);
    // skip private functions
    if (!name || name[0] == '_')
    {
      continue;
    }
    FunctionInfo info;
    info.name = name;

    PyObject* doc = PyObject_GetAttrString(value, "__doc__");
    if (doc && doc != Py_None)
    {
      const char* docStr = PyUnicode_AsUTF8(doc);
      if (docStr)
      {
        info.docstring = docStr;
      }
    }
    Py_XDECREF(doc);
    PyErr_Clear();

    PyObject* mod = PyObject_GetAttrString(value, "__module__");
    if (mod && mod != Py_None)
    {
      const char* modStr = PyUnicode_AsUTF8(mod);
      if (modStr)
      {
        info.module = modStr;
      }
    }
    Py_XDECREF(mod);
    PyErr_Clear();

    if (!ignoredModules.contains(info.module))
    {
      results << info;
    }
  }
  Py_DECREF(calcModule);

  std::sort(results.begin(), results.end(),
    [](const FunctionInfo& a, const FunctionInfo& b) { return a.name < b.name; });
  return results;
}

//-----------------------------------------------------------------------------
QStringList pqPythonCalculatorCompleter::getPythonCompletions(
  const QString& pythonObjectName, bool call)
{
  Q_UNUSED(call);

  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer;

  // `object` contains all locals available in the `calculator.py` script
  PyObject* calc_module = PyImport_ImportModule("paraview.detail.calculator");
  if (calc_module == nullptr)
  {
    return QStringList{};
  }
  PyObject* object = PyModule_GetDict(calc_module);
  Py_INCREF(object);

  // Constant variable names, defined in calculator.py
  const QStringList hardCodedVariables{ "time_value", "t_value", "time_index", "t_index",
    "Points" };

  QStringList results;
  vtkPVDataInformation* inputInformation = this->Input->GetDataInformation(0);

  // Generate completions for "inputs[X].Y" expressions
  if (pythonObjectName.contains(QRegularExpression("^inputs\\[[0-9]+\\]$")))
  {
    results += hardCodedVariables;

    auto addIfNonEmpty = [&](vtkPVDataSetAttributesInformation* dsa, const char* name)
    {
      if (dsa && dsa->GetNumberOfArrays() > 0)
      {
        results << name;
      }
    };
    addIfNonEmpty(inputInformation->GetPointDataInformation(), "PointData");
    addIfNonEmpty(inputInformation->GetCellDataInformation(), "CellData");
    addIfNonEmpty(inputInformation->GetFieldDataInformation(), "FieldData");
    addIfNonEmpty(inputInformation->GetVertexDataInformation(), "VertexData");
    addIfNonEmpty(inputInformation->GetEdgeDataInformation(), "EdgeData");
    addIfNonEmpty(inputInformation->GetRowDataInformation(), "RowData");

    return results;
  }
  else
  {
    // Complete an object attribute : replace `object` containing all the locals
    // by the attributes of the object we're trying to complete.
    if (!pythonObjectName.isEmpty())
    {
      object = this->derivePyObject(pythonObjectName, object);
    }
    else
    {
      results += hardCodedVariables;
      results << "inputs";

      // Array names shortcuts are only available for the first calculator input
      this->AddArrayNamesToResults(inputInformation->GetPointDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetCellDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetFieldDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetVertexDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetEdgeDataInformation(), results);
      this->AddArrayNamesToResults(inputInformation->GetRowDataInformation(), results);
    }
  }

  this->appendPyObjectAttributes(object, results);
  return results;
}

//-----------------------------------------------------------------------------
void pqPythonCalculatorCompleter::AddArrayNamesToResults(
  const vtkPVDataSetAttributesInformation* attributesInformation, QStringList& results)
{
  if (!attributesInformation)
  {
    return;
  }

  for (int i = 0; i < attributesInformation->GetNumberOfArrays(); i++)
  {
    vtkPVArrayInformation* arrayInfo = attributesInformation->GetArrayInformation(i);

    for (auto& c : std::string(arrayInfo->GetName()))
    {
      if (c == ' ')
      {
        return;
      }
    }

    results << arrayInfo->GetName();
  }
}
