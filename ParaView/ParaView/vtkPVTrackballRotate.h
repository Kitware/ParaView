/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballRotate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrackballRotate - Rotates camera with xy mouse movement.
// .SECTION Description
// vtkPVTrackballRotate allows the user to interactively
// manipulate the camera, the viewpoint of the scene.

#ifndef __vtkPVTrackballRotate_h
#define __vtkPVTrackballRotate_h

#include "vtkPVCameraManipulator.h"

class VTK_EXPORT vtkPVTrackballRotate : public vtkPVCameraManipulator
{
public:
  static vtkPVTrackballRotate *New();
  vtkTypeRevisionMacro(vtkPVTrackballRotate, vtkPVCameraManipulator);
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
  vtkPVTrackballRotate();
  ~vtkPVTrackballRotate();

  float Center[3];
  float DisplayCenter[2];

  vtkPVTrackballRotate(const vtkPVTrackballRotate&); // Not implemented
  void operator=(const vtkPVTrackballRotate&); // Not implemented
};

#endif
