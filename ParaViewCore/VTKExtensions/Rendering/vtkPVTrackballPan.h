/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballPan.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrackballPan - Pans camera with x y mouse movements.
// .SECTION Description
// vtkPVTrackballPan allows the user to interactively
// manipulate the camera, the viewpoint of the scene.
// Moving the mouse down zooms in. Up zooms out.
// This manipulator has not been extended to parallel projection yet.
// It works in perspective by rotating the camera.

#ifndef __vtkPVTrackballPan_h
#define __vtkPVTrackballPan_h

#include "vtkCameraManipulator.h"

class VTK_EXPORT vtkPVTrackballPan : public vtkCameraManipulator
{
public:
  static vtkPVTrackballPan *New();
  vtkTypeMacro(vtkPVTrackballPan, vtkCameraManipulator);
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
  vtkPVTrackballPan();
  ~vtkPVTrackballPan();

  vtkPVTrackballPan(const vtkPVTrackballPan&); // Not implemented
  void operator=(const vtkPVTrackballPan&); // Not implemented
};

#endif
