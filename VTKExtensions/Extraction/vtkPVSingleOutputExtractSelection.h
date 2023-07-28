// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVSingleOutputExtractSelection
 *
 * vtkPVSingleOutputExtractSelection extends to vtkPVExtractSelection to simply
 * hide the second output-port. This is the filter used in ParaView GUI.
 */

#ifndef vtkPVSingleOutputExtractSelection_h
#define vtkPVSingleOutputExtractSelection_h

#include "vtkPVExtractSelection.h"

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkPVSingleOutputExtractSelection
  : public vtkPVExtractSelection
{
public:
  static vtkPVSingleOutputExtractSelection* New();
  vtkTypeMacro(vtkPVSingleOutputExtractSelection, vtkPVExtractSelection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVSingleOutputExtractSelection();
  ~vtkPVSingleOutputExtractSelection() override;

private:
  vtkPVSingleOutputExtractSelection(const vtkPVSingleOutputExtractSelection&) = delete;
  void operator=(const vtkPVSingleOutputExtractSelection&) = delete;
};

#endif
