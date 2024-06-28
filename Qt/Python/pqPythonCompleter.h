// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonCompleter_h
#define pqPythonCompleter_h

#include "vtkPython.h"

#include "pqPythonModule.h"    //  needed for PQPYTHON_EXPORT.
#include "pqWidgetCompleter.h" // Defines pqWidgetCompleter interface

/**
 * Abstract class for Python completers. Child classes must implement how Python attributes
 * are retrieved from a given variable name prompt.
 */
class PQPYTHON_EXPORT pqPythonCompleter : public pqWidgetCompleter
{
public:
  pqPythonCompleter(QWidget* parent)
    : pqWidgetCompleter(parent){};

protected:
  QStringList getCompletions(const QString& prompt) override;
  QString getCompletionPrefix(const QString& prompt) override;

  /**
   * From a prompt string that can contain multiple tokens, retrieve the last (possibly incomplete)
   * variable name.
   */
  QString getVariableToComplete(const QString& prompt);

  /**
   * Given a PyObject, append all of its attributes in the results list.
   */
  void appendPyObjectAttributes(PyObject* object, QStringList& results);

  /**
   * Given a PyObject that is a function, append all of it's keyword arguments to the
   * results list.
   */
  void appendFunctionKeywordArguments(PyObject* function, QStringList& results);

  /**
   * Given `pythonObjectName` string in the form "X.Y.Z.T" and script locals,
   * return the most derived PyObject that matches the string.
   * For example, given "X.Y.Z" string and a locals object containing an object X with attribute Z,
   * returns the object X.Y
   */
  PyObject* derivePyObject(const QString& pythonObjectName, PyObject* locals);

  /**
   * Given a text prompt, return a list of possible completions.
   * This method must be implemented in concrete classes.
   */
  virtual QStringList getPythonCompletions(const QString& pythonObjectName) = 0;
};
#endif
