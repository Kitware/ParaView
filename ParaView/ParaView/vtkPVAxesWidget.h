/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAxesWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVAxesWidget
// .SECTION Description

#ifndef __vtkPVAxesWidget_h
#define __vtkPVAxesWidget_h

#include "vtkInteractorObserver.h"

class vtkPVAxesActor;
class vtkPVAxesWidgetObserver;
class vtkRenderer;

class VTK_EXPORT vtkPVAxesWidget : public vtkInteractorObserver
{
public:
  static vtkPVAxesWidget* New();
  vtkTypeRevisionMacro(vtkPVAxesWidget, vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetAxesActor(vtkPVAxesActor *actor);
  vtkGetObjectMacro(AxesActor, vtkPVAxesActor);

  virtual void SetEnabled(int);

  void SetParentRenderer(vtkRenderer *ren);

  void ExecuteEvent(vtkObject *o, unsigned long event, void *calldata);
  
protected:
  vtkPVAxesWidget();
  ~vtkPVAxesWidget();
  
  vtkRenderer *Renderer;
  vtkRenderer *ParentRenderer;
  
  vtkPVAxesActor *AxesActor;
  
  static void ProcessEvents(vtkObject *object, unsigned long event,
                            void *clientdata, void *calldata);

  vtkPVAxesWidgetObserver *Observer;
  
  int MouseCursorState;
  int Moving;
  int StartPosition[2];
  
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
  
private:
  vtkPVAxesWidget(const vtkPVAxesWidget&);  // Not implemented
  void operator=(const vtkPVAxesWidget&);  // Not implemented
};

#endif
