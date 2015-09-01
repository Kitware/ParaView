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
// .NAME vtkSMInteractiveSelectionPipeline -- Pipeline for interactive selection
//
// .SECTION Description
// Interactive selection enables the user to inspect cells/points before he
// decides to select them. The user moves the mouse cursor over a cell, can
// inspect attributes of the cell and can select the cell by clicking on it.
// This is a global object that holds the pipeline for showing the interactive
// selection.


#ifndef __vtkSMInteractiveSelectionPipeline_h
#define __vtkSMInteractiveSelectionPipeline_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkObject.h"

// Forward declarations
class vtkCallbackCommand;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkSMSourceProxy;
class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMInteractiveSelectionPipeline : public vtkObject
{
public:
  static vtkSMInteractiveSelectionPipeline* New();
  vtkTypeMacro(vtkSMInteractiveSelectionPipeline,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent );
  static vtkSMInteractiveSelectionPipeline* GetInstance();

  // Description:
  // Get/Create the interactive selection representation
  vtkSMProxy* GetSelectionRepresentation() const;
  vtkSMProxy* GetOrCreateSelectionRepresentation();
  vtkSMProxy* CreateSelectionRepresentation(vtkSMSourceProxy* extract);
  

  // Description:
  // Shows the interactive selection for 'selection' and 'sourceRepresentation'.
  // If either sourceRepresentation or selection are null it hides the 
  // interactive selection.
  void Show(vtkSMSourceProxy* sourceRepresentation, 
            vtkSMSourceProxy* selection, vtkSMRenderViewProxy* view);
  // Description:
  // Hides the interactive selection
  void Hide(vtkSMRenderViewProxy* view);
  // Description:
  // Copies the labels for interactive selection from 
  // the selection labels in the representation parameter.
  void CopyLabels(vtkSMProxy* representation);


protected:
  vtkSMInteractiveSelectionPipeline();
  ~vtkSMInteractiveSelectionPipeline();
  static void OnColorModified(
    vtkObject* source, unsigned long eid, void* clientdata, void *calldata);
  static void OnConnectionClosed(
    vtkObject* source, unsigned long eid, void* clientdata, void *calldata);
  void OnConnectionClosed();
  vtkSMSourceProxy* ConnectPVExtractSelection(
    vtkSMSourceProxy* source, unsigned int sourceOutputPort, 
    vtkSMSourceProxy* selection);

private:
  // Not implemented
  vtkSMInteractiveSelectionPipeline(
    const vtkSMInteractiveSelectionPipeline&); // Not implemented
  void operator=(const vtkSMInteractiveSelectionPipeline&); // Not implemented

protected:  
  vtkSMSourceProxy* ExtractInteractiveSelection;
  vtkSMProxy* SelectionRepresentation;

  vtkSMRenderViewProxy* PreviousView;
  vtkSMSourceProxy* PreviousRepresentation;
  vtkCallbackCommand* ColorObserver;
  vtkCallbackCommand* ConnectionObserver;
};

#endif
