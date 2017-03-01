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
/**
 * @class   vtkPVTrackballPan
 * @brief   Pans camera with x y mouse movements.
 *
 * vtkPVTrackballPan allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * Moving the mouse down zooms in. Up zooms out.
 * This manipulator has not been extended to parallel projection yet.
 * It works in perspective by rotating the camera.
*/

#ifndef vtkPVTrackballPan_h
#define vtkPVTrackballPan_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballPan : public vtkCameraManipulator
{
public:
  static vtkPVTrackballPan* New();
  vtkTypeMacro(vtkPVTrackballPan, vtkCameraManipulator);
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
  vtkPVTrackballPan();
  ~vtkPVTrackballPan();

  vtkPVTrackballPan(const vtkPVTrackballPan&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTrackballPan&) VTK_DELETE_FUNCTION;
};

#endif
