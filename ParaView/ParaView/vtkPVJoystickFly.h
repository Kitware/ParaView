/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFly.h
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
// .NAME vtkPVJoystickFly - Rotates camera with xy mouse movement.
// .SECTION Description
// vtkPVJoystickFly allows the user to interactively
// manipulate the camera, the viewpoint of the scene.

#ifndef __vtkPVJoystickFly_h
#define __vtkPVJoystickFly_h

#include "vtkPVCameraManipulator.h"

class vtkRenderer;

class VTK_EXPORT vtkPVJoystickFly : public vtkPVCameraManipulator
{
public:
  vtkTypeRevisionMacro(vtkPVJoystickFly, vtkPVCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove(int x, int y, vtkRenderer *ren,
                           vtkRenderWindowInteractor *rwi);
  virtual void OnButtonDown(int x, int y, vtkRenderer *ren,
                            vtkRenderWindowInteractor *rwi);
  virtual void OnButtonUp(int x, int y, vtkRenderer *ren,
                          vtkRenderWindowInteractor *rwi);

  // Description:
  // For setting the center of rotation.
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);
  
protected:
  vtkPVJoystickFly();
  ~vtkPVJoystickFly();

  float Center[3];
  float DisplayCenter[2];

  int In;
  int FlyFlag;

  double Speed;
  double Scale;
  double LastRenderTime;
  double CameraXAxis[3];
  double CameraYAxis[3];
  double CameraZAxis[3];

  void Fly(vtkRenderer* ren, vtkRenderWindowInteractor *rwi, 
           float scale, float speed);
  void ComputeCameraAxes(vtkRenderer*);

  vtkPVJoystickFly(const vtkPVJoystickFly&); // Not implemented
  void operator=(const vtkPVJoystickFly&); // Not implemented
};

#endif
