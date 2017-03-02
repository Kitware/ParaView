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
/**
 * @class   vtkPVTrackballMoveActor
 * @brief   Pans camera with x y mouse movements.
 *
 * vtkPVTrackballMoveActor allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * Moving the mouse down zooms in. Up zooms out.
 * This manipulator has not been extended to parallel projection yet.
 * It works in perspective by rotating the camera.
*/

#ifndef vtkPVTrackballMoveActor_h
#define vtkPVTrackballMoveActor_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballMoveActor : public vtkCameraManipulator
{
public:
  static vtkPVTrackballMoveActor* New();
  vtkTypeMacro(vtkPVTrackballMoveActor, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void OnMouseMove(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) VTK_OVERRIDE;
  virtual void OnButtonDown(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) VTK_OVERRIDE;
  virtual void OnButtonUp(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) VTK_OVERRIDE;
  //@}

protected:
  vtkPVTrackballMoveActor();
  ~vtkPVTrackballMoveActor();

  vtkPVTrackballMoveActor(const vtkPVTrackballMoveActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTrackballMoveActor&) VTK_DELETE_FUNCTION;
};

#endif
