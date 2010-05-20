/*=========================================================================

   Program: ParaView
   Module:    vtkPVAxesWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVAxesWidget - A widget to manipulate an axe
//
// .SECTION Description
// This widget creates and manages its own vtkPVAxesActor.


#ifndef __vtkPVAxesWidget_h
#define __vtkPVAxesWidget_h

#include "vtkInteractorObserver.h"
#include "pqCoreExport.h"

class vtkActor2D;
class vtkKWApplication;
class vtkPolyData;
class vtkPVAxesActor;
class vtkPVAxesWidgetObserver;
class vtkRenderer;

class PQCORE_EXPORT vtkPVAxesWidget : public vtkInteractorObserver
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
  
//BTX
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
//ETX
  
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
};

#endif
