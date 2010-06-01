/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAxesWidget - A widget to manipulate an axe
//
// .SECTION Description
// This widget creates and manages its own vtkPVAxesActor.


#ifndef __vtkPVAxesWidget_h
#define __vtkPVAxesWidget_h

#include "vtkInteractorObserver.h"

class vtkActor2D;
class vtkKWApplication;
class vtkPolyData;
class vtkPVAxesActor;
class vtkPVAxesWidgetObserver;
class vtkRenderer;

class VTK_EXPORT vtkPVAxesWidget : public vtkInteractorObserver
{
public:
  static vtkPVAxesWidget* New();
  vtkTypeMacro(vtkPVAxesWidget, vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the axes actor to be displayed in this 3D widget.
  void SetAxesActor(vtkPVAxesActor *actor);
  vtkGetObjectMacro(AxesActor, vtkPVAxesActor);

  // Description:
  // Enable the 3D widget.
  virtual void SetEnabled(int);

  //BTX
  // Description:
  // Set the renderer this 3D widget will be contained in.
  void SetParentRenderer(vtkRenderer *ren);
  vtkRenderer* GetParentRenderer();
  //ETX

  // Description:
  // Callback to keep the camera for the axes actor up to date with the
  // camera in the parent renderer
  void ExecuteEvent(vtkObject *o, unsigned long event, void *calldata);

  // Description:
  // Set/get whether to allow this 3D widget to be interactively moved/scaled.
  void SetInteractive(int state);
  vtkGetMacro(Interactive, int);
  vtkBooleanMacro(Interactive, int);

  // Description:
  // Set/get the color of the outline of this widget.  The outline is visible
  // when (in interactive mode) the cursor is over this 3D widget.
  void SetOutlineColor(double r, double g, double b);
  double *GetOutlineColor();

  // Description:
  // Set/get the color of the axis labels of this widget.
  void SetAxisLabelColor(double r, double g, double b);
  double *GetAxisLabelColor();

  // Description:
  // Set/get the viewport to position/size this 3D widget.
  void SetViewport(double minX, double minY, double maxX, double maxY);
  double* GetViewport();

//BTX
protected:
  vtkPVAxesWidget();
  ~vtkPVAxesWidget();

  vtkRenderer *Renderer;
  vtkRenderer *ParentRenderer;

  vtkPVAxesActor *AxesActor;
  vtkPolyData *Outline;
  vtkActor2D *OutlineActor;

  static void ProcessEvents(vtkObject *object, unsigned long event,
                            void *clientdata, void *calldata);

  vtkPVAxesWidgetObserver *Observer;
  int StartTag;

  int MouseCursorState;
  int Moving;
  int StartPosition[2];

  int Interactive;

  void UpdateCursorIcon();
  void SetMouseCursor(int cursorState);

  int State;

  enum AxesWidgetState
  {
    Outside = 0,
    Inside,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
  };

  void OnButtonPress();
  void OnMouseMove();
  void OnButtonRelease();

  void MoveWidget();
  void ResizeTopLeft();
  void ResizeTopRight();
  void ResizeBottomLeft();
  void ResizeBottomRight();

  void SquareRenderer();

  unsigned long StartEventObserverId;
private:
  vtkPVAxesWidget(const vtkPVAxesWidget&);  // Not implemented
  void operator=(const vtkPVAxesWidget&);  // Not implemented
//ETX
};

#endif
