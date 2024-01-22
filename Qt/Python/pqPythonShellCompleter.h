// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonShellCompleter_h
#define pqPythonShellCompleter_h

#include "pqPythonCompleter.h"
#include "pqPythonModule.h" //  needed for PQPYTHON_EXPORT.
#include "vtkWeakPointer.h" // for weak pointer

class vtkPythonInteractiveInterpreter;

/**
 * Completer class for Python shell, using interactive shell context to provide line completions.
 */
class PQPYTHON_EXPORT pqPythonShellCompleter : public pqPythonCompleter
{
public:
  pqPythonShellCompleter(QWidget* parent, vtkPythonInteractiveInterpreter* interp)
    : pqPythonCompleter(parent)
    , Interpreter(interp){};

protected:
  QStringList getPythonCompletions(const QString& pythonObjectName) override;

private:
  vtkWeakPointer<vtkPythonInteractiveInterpreter> Interpreter;
};

#endif
