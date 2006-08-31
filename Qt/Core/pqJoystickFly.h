/*=========================================================================

   Program: ParaView
   Module:    pqJoystickFly.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

========================================================================*/
// .NAME pqJoystickFly - Fly camera towards or away from the object
// .SECTION Description
// pqJoystickFly is a camera manipulator that makes the camera fly towards 
// or away from the object.

#ifndef __pqJoystickFly_h
#define __pqJoystickFly_h

#include "vtkCameraManipulator.h"
#include "pqCoreExport.h"

class vtkRenderer;

class PQCORE_EXPORT pqJoystickFly : public vtkCameraManipulator
{
public:
  static pqJoystickFly* New();
  vtkTypeRevisionMacro(pqJoystickFly, vtkCameraManipulator);
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
  // Set and get the speed of flying.
  vtkSetClampMacro(FlySpeed, double, 1, 30);  
  vtkGetMacro(FlySpeed, double);  

  // Description:
  // Set to 1 for fly in, and 0 for fly out.
  // Default is 1.
  vtkSetMacro(In, int);
  vtkGetMacro(In, int);
protected:
  pqJoystickFly();
  ~pqJoystickFly();

  int In;
  int FlyFlag;

  double FlySpeed;
  double Scale;
  double LastRenderTime;
  double CameraXAxis[3];
  double CameraYAxis[3];
  double CameraZAxis[3];

  void Fly(vtkRenderer* ren, vtkRenderWindowInteractor *rwi, 
           float scale, float speed);
  void ComputeCameraAxes(vtkRenderer*);

  pqJoystickFly(const pqJoystickFly&); // Not implemented
  void operator=(const pqJoystickFly&); // Not implemented
};

#endif
