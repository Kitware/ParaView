// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPythonSelector
 * @brief Select cells/points using numpy expressions
 */

#ifndef vtkPythonSelector_h
#define vtkPythonSelector_h

#include "vtkPVVTKExtensionsExtractionPythonModule.h" //needed for exports
#include "vtkSelector.h"

class vtkSelectionNode;

class VTKPVVTKEXTENSIONSEXTRACTIONPYTHON_EXPORT vtkPythonSelector : public vtkSelector
{
public:
  static vtkPythonSelector* New();
  vtkTypeMacro(vtkPythonSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to delegate the selection to the Python expression.
   */
  void Execute(vtkDataObject* input, vtkDataObject* output) override;

protected:
  vtkPythonSelector();
  ~vtkPythonSelector() override;

  bool ComputeSelectedElements(vtkDataObject*, vtkSignedCharArray*) override;

private:
  vtkPythonSelector(const vtkPythonSelector&) = delete;
  void operator=(const vtkPythonSelector&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
