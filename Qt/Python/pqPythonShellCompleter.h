// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonShellCompleter_h
#define pqPythonShellCompleter_h

#include "pqConsoleWidget.h"

#include "pqPythonModule.h" //  needed for PQPYTHON_EXPORT.

#include "vtkWeakPointer.h" // for weak pointer

class vtkPythonInteractiveInterpreter;

class PQPYTHON_EXPORT pqPythonShellCompleter : public pqConsoleWidgetCompleter
{
public:
  pqPythonShellCompleter(QWidget* p, vtkPythonInteractiveInterpreter* interp);

  /**
   * Update the completion model given a string. The given string is the current entered text.
   */
  void updateCompletionModel(const QString& rootText) override;

protected:
  /**
   * Given a python variable name, look up its attributes and return them in a
   * string list.
   */
  QStringList getPythonAttributes(const QString& pythonObjectName);

private:
  vtkWeakPointer<vtkPythonInteractiveInterpreter> Interpreter;
};

#endif
