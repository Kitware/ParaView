// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPython.h"

#include "pqPythonShellCompleter.h"

#include <QAbstractItemView>
#include <QStringListModel>

#include "vtkPythonInteractiveInterpreter.h"

//-----------------------------------------------------------------------------
QStringList pqPythonShellCompleter::getPythonCompletions(const QString& pythonObjectName, bool call)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (this->Interpreter == nullptr ||
    this->Interpreter->GetInteractiveConsoleLocalsPyObject() == nullptr)
  {
    return QStringList();
  }

  // Complete prompt using all local shell variable
  PyObject* locals =
    reinterpret_cast<PyObject*>(this->Interpreter->GetInteractiveConsoleLocalsPyObject());
  Py_INCREF(locals);

  PyObject* builtins = this->getBuiltins(locals);
  Py_XINCREF(builtins);

  PyObject* object = locals;

  QStringList results;

  // Unless we try to complete an object attribute,
  // in which case we can only complete with object's attributes
  if (!pythonObjectName.isEmpty())
  {
    object = this->derivePyObject(pythonObjectName, locals);

    if (object)
    {
      Py_XDECREF(builtins);
    }
    else
    {
      object = this->derivePyObject(pythonObjectName, builtins);
    }

    if (call)
    {
      if (object && PyFunction_Check(object))
      {
        this->appendFunctionKeywordArguments(object, results);
      }
      Py_XDECREF(object);
    }
    else
    {
      this->appendPyObjectAttributes(object, results);
    }
  }
  else
  {
    this->appendPyObjectAttributes(locals, results);
    this->appendPyObjectAttributes(builtins, results);
  }

  return results;
}
