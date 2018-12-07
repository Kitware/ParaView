/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSMPreselectionPipeline.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkSMPreselectionPipeline
 *
 *
 * Preselection enables the user to inspect cells/points without actually
 * selecting them. The user moves the mouse cursor over a cell and
 * see the cell highlighted.
 * This is a global object that holds the pipeline for the interactive
 * selection and the point tooltip mode.
 *
 * @sa
 * vtkSMInteractiveSelectionPipeline vtkSMTooltipSelectionPipeline
*/

#ifndef vtkSMPreselectionPipeline_h
#define vtkSMPreselectionPipeline_h

#include "vtkObject.h"
#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkWeakPointer.h"                  // Weak Pointer

// Forward declarations
class vtkCallbackCommand;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkSMSourceProxy;
class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMPreselectionPipeline : public vtkObject
{
public:
  vtkTypeMacro(vtkSMPreselectionPipeline, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Create the interactive selection representation
   */
  virtual vtkSMProxy* GetSelectionRepresentation() const;
  virtual vtkSMProxy* GetOrCreateSelectionRepresentation();
  virtual vtkSMProxy* CreateSelectionRepresentation(vtkSMSourceProxy* extract);
  //@}

  /**
   * Shows the interactive selection for 'selection' and 'sourceRepresentation'.
   * If either sourceRepresentation or selection are null it hides the
   * interactive selection.
   */
  virtual void Show(vtkSMSourceProxy* sourceRepresentation, vtkSMSourceProxy* selection,
    vtkSMRenderViewProxy* view);
  /**
   * Hides the interactive selection
   */
  virtual void Hide(vtkSMRenderViewProxy* view);
  /**
   * Copies the labels for interactive selection from
   * the selection labels in the representation parameter.
   */
  void CopyLabels(vtkSMProxy* representation);

protected:
  vtkSMPreselectionPipeline();
  ~vtkSMPreselectionPipeline() override;
  static void OnColorModified(
    vtkObject* source, unsigned long eid, void* clientdata, void* calldata);
  static void ClearCache(vtkObject* source, unsigned long eid, void* clientdata, void* calldata);
  virtual void ClearCache();
  vtkSMSourceProxy* ConnectPVExtractSelection(
    vtkSMSourceProxy* source, unsigned int sourceOutputPort, vtkSMSourceProxy* selection);

private:
  vtkSMPreselectionPipeline(const vtkSMPreselectionPipeline&) = delete;
  void operator=(const vtkSMPreselectionPipeline&) = delete;

protected:
  vtkSMSourceProxy* ExtractInteractiveSelection;
  vtkSMProxy* SelectionRepresentation;

  vtkWeakPointer<vtkSMRenderViewProxy> PreviousView;
  vtkWeakPointer<vtkSMSourceProxy> PreviousRepresentation;
  vtkCallbackCommand* ColorObserver;
  vtkCallbackCommand* ConnectionObserver;
};

#endif
