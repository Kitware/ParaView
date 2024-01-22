// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonCalculatorCompleter_h
#define pqPythonCalculatorCompleter_h

#include <vtkPython.h>

#include "pqPythonCompleter.h"
#include "vtkSMSourceProxy.h"

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
    this->setCompleteEmptyPrompts(true); // Ovrerride pqPythonCompleter default
  }

protected:
  QStringList getPythonCompletions(const QString& pythonObjectName) override;

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
