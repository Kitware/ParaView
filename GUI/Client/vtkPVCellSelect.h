/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCellSelect.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCellSelect - Client side UI for a selection rectangle for 
// cells and points.
// .SECTION Description
// Creates a client GUI page that makes it possible to draw a rectangle on the
// render window and select all points and cells of a dataset that lie within.
// This does cell/point level selection, while vtkPVPickObjects does prop
// level selection.

#ifndef __vtkPVCellSelect_h
#define __vtkPVCellSelect_h

#include "vtkPVSource.h"

class vtkKWPushButton;
class vtkCallbackCommand;
class vtkRenderer;
class vtkInteractorObserver;
class vtkInteractorStyleRubberBandPick;

class VTK_EXPORT vtkPVCellSelect : public vtkPVSource
{
public:
  static vtkPVCellSelect* New();
  vtkTypeRevisionMacro(vtkPVCellSelect, vtkPVSource);

  // Description:
  // Set up the UI.
  virtual void CreateProperties();

  // Description:
  // Respond to the button.
  void SelectCallback();

  // Description:
  // Mouse event callbacks in the render window to define selection area with.
  void OnLeftButtonDown(int x, int y);
  void OnLeftButtonUp(int x, int y, vtkRenderer *renderer);

  // Description:
  // For internal use and playback only but need to be public for scripts.
  void CreateVert(int i, double v0, double v1, double v2, double v3);
  void SetVerts(int wireframe);

protected:
  vtkPVCellSelect();
  ~vtkPVCellSelect();

  void DoSelect();  
  virtual void AcceptCallbackInternal();

  //to watch mouse with
  vtkCallbackCommand* EventCallbackCommand;
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);
  
  //have to help out trace, state and batch saving because UI is 
  //for this source is not completely XML controlled, in particular
  //frustum vertices are not shown on the UI and set by a widget
  void AdditionalTraceSave();
  virtual void AdditionalStateSave(ofstream *file);
  virtual void AdditionalBatchSave(ofstream *file);


  //to use while picking
  vtkInteractorStyleRubberBandPick *RubberBand;

  //to restore after picking
  vtkInteractorObserver *SavedStyle;

  //to turn on selection
  vtkKWPushButton *SelectButton;

  //internal operating state
  int SelectReady;
  int InPickState;

  //frustum parameters (screen and world coordinates)
  int Xs;
  int Ys;
  int Xe;
  int Ye;
  double Verts[32];

  //needed because frustum params not shown in the UI and set by a widget
  virtual void Reset();
  double SavedVerts[32];

private:
  vtkPVCellSelect(const vtkPVCellSelect&); // Not implemented
  void operator=(const vtkPVCellSelect&); // Not implemented
};

#endif
