// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPython.h"

#include "pqPythonShellCompleter.h"

#include <QAbstractItemView>
#include <QStringListModel>

#include "vtkPythonInteractiveInterpreter.h"

//-----------------------------------------------------------------------------
QStringList pqPythonShellCompleter::getPythonCompletions(const QString& pythonObjectName)
{
  vtkPythonScopeGilEnsurer gilEnsurer;
  if (this->Interpreter == nullptr ||
    this->Interpreter->GetInteractiveConsoleLocalsPyObject() == nullptr)
  {
    return QStringList();
  }

  // Complete prompt using all local shell variable
  PyObject* object =
    reinterpret_cast<PyObject*>(this->Interpreter->GetInteractiveConsoleLocalsPyObject());
  Py_INCREF(object);

  // Unless we try to complete an object attribute,
  // in which case we can only complete with object's attributes
  if (!pythonObjectName.isEmpty())
  {
    object = this->derivePyObject(pythonObjectName, object);
  }

  QStringList results;
  this->appendPyObjectAttributes(object, results);
  return results;
}
