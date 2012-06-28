/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballRoll.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrackballRoll - Rolls camera arround a point.
// .SECTION Description
// vtkPVTrackballRoll allows the user to interactively
// manipulate the camera, the viewpoint of the scene.
// Roll tracks the mouse around the center of rotation.

#ifndef __vtkPVTrackballRoll_h
#define __vtkPVTrackballRoll_h

#include "vtkCameraManipulator.h"

class VTK_EXPORT vtkPVTrackballRoll : public vtkCameraManipulator
{
public:
  static vtkPVTrackballRoll *New();
  vtkTypeMacro(vtkPVTrackballRoll, vtkCameraManipulator);
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

protected:
  vtkPVTrackballRoll();
  ~vtkPVTrackballRoll();

  vtkPVTrackballRoll(const vtkPVTrackballRoll&); // Not implemented
  void operator=(const vtkPVTrackballRoll&); // Not implemented
};

#endif
