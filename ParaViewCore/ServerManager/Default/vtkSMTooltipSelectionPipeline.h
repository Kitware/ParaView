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
// .NAME vtkSMTooltipSelectionPipeline -- Pipeline for point tooltip mode
//
// .SECTION Description
// Point tooltip mode enables the user to inspect points (coordinates,
// data array values) by hovering the mouse cursor over a point.
// This is a global object that holds the pipeline for showing the point
// tooltip mode.
//
// .SECTION See Also
// vtkSMPreselectionPipeline vtkSMInteractiveSelectionPipeline


#ifndef vtkSMTooltipSelectionPipeline_h
#define vtkSMTooltipSelectionPipeline_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMPreselectionPipeline.h"

#include <string> // STL Header

class vtkDataObject;
class vtkDataSet;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMTooltipSelectionPipeline :
  public vtkSMPreselectionPipeline
{
public:
  static vtkSMTooltipSelectionPipeline* New();
  vtkTypeMacro(vtkSMTooltipSelectionPipeline,vtkSMPreselectionPipeline);
  void PrintSelf(ostream& os, vtkIndent indent );
  static vtkSMTooltipSelectionPipeline* GetInstance();

  // Description:
  // Re-implemented from vtkSMPreselectionPipeline
  virtual void Hide(vtkSMRenderViewProxy* view);
  virtual void Show(vtkSMSourceProxy* sourceRepresentation,
    vtkSMSourceProxy* selection, vtkSMRenderViewProxy* view);

  // Description:
  // Return true if a tooltip can be displayed according to the context,
  // otherwise return false. The argument showTooltip is true if the tooltip
  // must be shown, false if the tooltip must be hidden.
  bool CanDisplayTooltip(bool& showTooltip);

  // Description:
  // Get information about the tooltip to be displayed.
  // Return false if the method failed computing information.
  bool GetTooltipInfo(double tooltipPos[2], std::string& tooltipText);

protected:
  vtkSMTooltipSelectionPipeline();
  ~vtkSMTooltipSelectionPipeline();

  // Description:
  // Re-implemented from vtkSMPreselectionPipeline
  void ClearCache();

  // Description:
  // Connect the ClientServerMoveData filter to the pipeline to get
  // the selection on the client side.
  vtkDataObject* ConnectPVMoveSelectionToClient(
    vtkSMSourceProxy* source, unsigned int sourceOutputPort);

  // Description:
  // Get the id of the selected point.
  bool GetCurrentSelectionId(vtkSMRenderViewProxy* view, vtkIdType& selId);

  // Description:
  // Extract dataset from the dataObject, which can be either directly a dataset 
  // or a composite dataset containing only one dataset.
  vtkDataSet* FindDataSet(vtkDataObject* dataObject, 
    bool& compositeFound, std::string& compositeName);

  vtkSMSourceProxy* MoveSelectionToClient;
  vtkIdType PreviousSelectionId;
  bool SelectionFound;
  bool TooltipEnabled;

private:
  // Not implemented
  vtkSMTooltipSelectionPipeline(
    const vtkSMTooltipSelectionPipeline&); // Not implemented
  void operator=(const vtkSMTooltipSelectionPipeline&); // Not implemented
};

#endif
