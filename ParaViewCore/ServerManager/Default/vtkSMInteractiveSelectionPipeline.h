/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSMInteractiveSelectionPipeline.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
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

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMPreselectionPipeline.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMInteractiveSelectionPipeline
  : public vtkSMPreselectionPipeline
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
