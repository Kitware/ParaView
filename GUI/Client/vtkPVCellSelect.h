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
// .NAME vtkPVCellSelect - A DUMMY GUI FILTER THAT LETS YOU DRAW A RECTANGLE
// .SECTION Description

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
  
  //to use while picking
  vtkInteractorStyleRubberBandPick *RubberBand;

  //to restore after picking
  vtkInteractorObserver *SavedStyle;

  //to turn on selection
  vtkKWPushButton *SelectButton;

  //internal operating state
  int SelectReady;
  int InPickState;
  int SelectionType;
  int LastSelectType;

  //frustum parameters (screen and world coordinates)
  int Xs;
  int Ys;
  int Xe;
  int Ye;
  double Verts[32];

private:
  vtkPVCellSelect(const vtkPVCellSelect&); // Not implemented
  void operator=(const vtkPVCellSelect&); // Not implemented
};

#endif
