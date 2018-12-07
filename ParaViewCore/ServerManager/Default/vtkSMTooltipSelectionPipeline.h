/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSMTooltipSelectionPipeline.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkSMTooltipSelectionPipeline
 *
 *
 * Point tooltip mode enables the user to inspect points (coordinates,
 * data array values) by hovering the mouse cursor over a point.
 * This is a global object that holds the pipeline for showing the point
 * tooltip mode.
 *
 * @sa
 * vtkSMPreselectionPipeline vtkSMInteractiveSelectionPipeline
*/

#ifndef vtkSMTooltipSelectionPipeline_h
#define vtkSMTooltipSelectionPipeline_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMPreselectionPipeline.h"

#include <string> // STL Header

class vtkDataObject;
class vtkDataSet;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMTooltipSelectionPipeline
  : public vtkSMPreselectionPipeline
{
public:
  static vtkSMTooltipSelectionPipeline* New();
  vtkTypeMacro(vtkSMTooltipSelectionPipeline, vtkSMPreselectionPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSMTooltipSelectionPipeline* GetInstance();

  //@{
  /**
   * Re-implemented from vtkSMPreselectionPipeline
   */
  void Hide(vtkSMRenderViewProxy* view) override;
  void Show(vtkSMSourceProxy* sourceRepresentation, vtkSMSourceProxy* selection,
    vtkSMRenderViewProxy* view) override;
  //@}

  /**
   * Return true if a tooltip can be displayed according to the context,
   * otherwise return false. The argument showTooltip is true if the tooltip
   * must be shown, false if the tooltip must be hidden.
   */
  bool CanDisplayTooltip(bool& showTooltip);

  /**
   * Get information about the tooltip to be displayed.
   * Return false if the method failed computing information.
   */
  bool GetTooltipInfo(int association, double tooltipPos[2], std::string& tooltipText);

protected:
  vtkSMTooltipSelectionPipeline();
  ~vtkSMTooltipSelectionPipeline() override;

  /**
   * Re-implemented from vtkSMPreselectionPipeline
   */
  void ClearCache() override;

  /**
   * Connect the ClientServerMoveData filter to the pipeline to get
   * the selection on the client side.
   */
  vtkDataObject* ConnectPVMoveSelectionToClient(
    vtkSMSourceProxy* source, unsigned int sourceOutputPort);

  /**
   * Get the id of the selected point.
   */
  bool GetCurrentSelectionId(vtkSMRenderViewProxy* view, vtkIdType& selId);

  /**
   * Extract dataset from the dataObject, which can be either directly a dataset
   * or a composite dataset containing only one dataset.
   */
  vtkDataSet* FindDataSet(
    vtkDataObject* dataObject, bool& compositeFound, std::string& compositeName);

  vtkSMSourceProxy* MoveSelectionToClient;
  vtkIdType PreviousSelectionId;
  bool SelectionFound;
  bool TooltipEnabled;

private:
  vtkSMTooltipSelectionPipeline(const vtkSMTooltipSelectionPipeline&) = delete;
  void operator=(const vtkSMTooltipSelectionPipeline&) = delete;
};

#endif
