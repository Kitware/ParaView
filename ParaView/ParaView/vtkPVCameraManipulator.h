/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.h
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
// .NAME vtkPVCameraManipulator - Abstraction of style away from button.
// .SECTION Description
// vtkPVCameraManipulator is a superclass for actions inside an interactor 
// style and associated with a single button.  An example might be
// rubber-band bounding-box zoom.  This abstraction allows a camera 
// manipulator to be assigned to any button.  This super class
// might become a subclass of vtkInteractorObserver in the future. 


#ifndef __vtkPVCameraManipulator_h
#define __vtkPVCameraManipulator_h

#include "vtkObject.h"

class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkKWApplication;

class VTK_EXPORT vtkPVCameraManipulator : public vtkObject
{
public:
  static vtkPVCameraManipulator *New();
  vtkTypeRevisionMacro(vtkPVCameraManipulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *iren);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *iren);
  virtual void OnButtonUp(int x, int y, vtkRenderer *ren,
                          vtkRenderWindowInteractor *iren);
  
  // Description:
  // These settings determine which button and modifiers the
  // manipulator responds to. Button can be either 1 (left), 2
  // (middle), and 3 right.
  vtkSetMacro(Button, int);
  vtkGetMacro(Button, int);
  vtkSetMacro(Shift, int);
  vtkGetMacro(Shift, int);
  vtkBooleanMacro(Shift, int);
  vtkSetMacro(Control, int);
  vtkGetMacro(Control, int);
  vtkBooleanMacro(Control, int);

  // Description:
  // For setting the center of rotation.
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);

  // Description:
  // In order to make calls on the application, we need a pointer to
  // it.
  void SetApplication(vtkKWApplication*);
  vtkGetObjectMacro(Application, vtkKWApplication);

  // Description:
  // Set and get the manipulator name.
  vtkSetStringMacro(ManipulatorName);
  vtkGetStringMacro(ManipulatorName);

protected:
  vtkPVCameraManipulator();
  ~vtkPVCameraManipulator();

  void ResetLights();

  char* ManipulatorName;

  int Button;
  int Shift;
  int Control;

  int LastX;
  int LastY;

  float Center[3];
  float DisplayCenter[2];
  void ComputeDisplayCenter(vtkRenderer *ren);

  vtkKWApplication *Application;

private:
  vtkPVCameraManipulator(const vtkPVCameraManipulator&); // Not implemented
  void operator=(const vtkPVCameraManipulator&); // Not implemented
};

#endif
