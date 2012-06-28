/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballMoveActor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrackballMoveActor - Pans camera with x y mouse movements.
// .SECTION Description
// vtkPVTrackballMoveActor allows the user to interactively
// manipulate the camera, the viewpoint of the scene.
// Moving the mouse down zooms in. Up zooms out.
// This manipulator has not been extended to parallel projection yet.
// It works in perspective by rotating the camera.

#ifndef __vtkPVTrackballMoveActor_h
#define __vtkPVTrackballMoveActor_h

#include "vtkCameraManipulator.h"

class VTK_EXPORT vtkPVTrackballMoveActor : public vtkCameraManipulator
{
public:
  static vtkPVTrackballMoveActor *New();
  vtkTypeMacro(vtkPVTrackballMoveActor, vtkCameraManipulator);
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
  
protected:
  vtkPVTrackballMoveActor();
  ~vtkPVTrackballMoveActor();

  vtkPVTrackballMoveActor(const vtkPVTrackballMoveActor&); // Not implemented
  void operator=(const vtkPVTrackballMoveActor&); // Not implemented
};

#endif
