/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleFly.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInteractorStyleFly - interactive manipulation of the camera
// .SECTION Description
// vtkPVInteractorStyleFly allows the user to interactively manipulate the
// camera (the viewpoint) to fly around the scene.

#ifndef __vtkPVInteractorStyleFly
#define __vtkPVInteractorStyleFly

#include "vtkInteractorStyle.h"

class VTK_EXPORT vtkPVInteractorStyleFly : public vtkInteractorStyle
{
public:
  static vtkPVInteractorStyleFly *New();
  vtkTypeRevisionMacro(vtkPVInteractorStyleFly, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons.
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  
  // Description:
  // These methods are for the interactions for this interactor style.
  virtual void Fly(float scale, float speed);
  
  // Description:
  // Set the fly speed
  vtkSetMacro(Speed, float);
  vtkGetMacro(Speed, float);
  
protected:
  vtkPVInteractorStyleFly();
  ~vtkPVInteractorStyleFly();
  
  void ComputeCameraAxes();
  void ResetLights();
  
  // Used to signal the fly loop to stop.
  int FlyFlag;
  float Speed;
  double LastRenderTime;
  double CameraXAxis[3];
  double CameraYAxis[3];
  double CameraZAxis[3];

  vtkPVInteractorStyleFly(const vtkPVInteractorStyleFly&); // Not implemented
  void operator=(const vtkPVInteractorStyleFly&); // Not implemented
};

#endif
