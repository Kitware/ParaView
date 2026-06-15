// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonCalculatorCompleter_h
#define pqPythonCalculatorCompleter_h

#include "pqPythonCompleter.h"
#include "vtkSMSourceProxy.h" // For vtkSMSourceProxy

/**
 * Completer to be used for the Python calculator expression field.
 * Looks up attributes of Python objects, static calculator-specific variables and data set field
 * names.
 */
class PQPYTHON_EXPORT pqPythonCalculatorCompleter : public pqPythonCompleter
{
public:
  pqPythonCalculatorCompleter(QWidget* parent, vtkSMSourceProxy* input)
    : pqPythonCompleter(parent)
    , Input(input)
  {
    this->setCompleteEmptyPrompts(true); // Override pqPythonCompleter default
  }

  struct FunctionInfo
  {
    QString name;
    QString docstring;
    QString module;
  };

  /**
   * Return function info (name, docstring, module) for all callable, non-private symbols in the
   * calculator's Python namespace (`paraview.detail.calculator`), sorted by module then name.
   * Functions whose module is in @p ignoredModules are excluded.
   */
  static QList<FunctionInfo> getFunctionInfos(const QStringList& ignoredModules = {});

protected:
  QStringList getPythonCompletions(const QString& pythonObjectName, bool call) override;

  /**
   * Append the names of arrays of the dataset attributes information object at the end of the
   * `results` list.
   */
  void AddArrayNamesToResults(
    const vtkPVDataSetAttributesInformation* attributesInformation, QStringList& results);

private:
  vtkSMSourceProxy* Input; // Needed to retrieve input field names
};
#endif
