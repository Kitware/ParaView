// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMInteractiveSelectionPipeline
 *
 *
 * Interactive selection enables the user to inspect cells/points before he
 * decides to select them. The user moves the mouse cursor over a cell, can
 * inspect attributes of the cell and can select the cell by clicking on it.
 * This is a global object that holds the pipeline for showing the interactive
 * selection.
 *
 * @sa
 * vtkSMPreselectionPipeline vtkSMTooltipSelectionPipeline
 */

#ifndef vtkSMInteractiveSelectionPipeline_h
#define vtkSMInteractiveSelectionPipeline_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMPreselectionPipeline.h"

class VTKREMOTINGMISC_EXPORT vtkSMInteractiveSelectionPipeline : public vtkSMPreselectionPipeline
{
public:
  static vtkSMInteractiveSelectionPipeline* New();
  vtkTypeMacro(vtkSMInteractiveSelectionPipeline, vtkSMPreselectionPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSMInteractiveSelectionPipeline* GetInstance();

protected:
  vtkSMInteractiveSelectionPipeline();
  ~vtkSMInteractiveSelectionPipeline() override;

private:
  vtkSMInteractiveSelectionPipeline(const vtkSMInteractiveSelectionPipeline&) = delete;
  void operator=(const vtkSMInteractiveSelectionPipeline&) = delete;
};

#endif
