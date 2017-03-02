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
/**
 * @class   vtkPVTrackballRotate
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkPVTrackballRotate allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
*/

#ifndef vtkPVTrackballRotate_h
#define vtkPVTrackballRotate_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVTrackballRotate : public vtkCameraManipulator
{
public:
  static vtkPVTrackballRotate* New();
  vtkTypeMacro(vtkPVTrackballRotate, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  virtual void OnMouseMove(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) VTK_OVERRIDE;
  virtual void OnButtonDown(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) VTK_OVERRIDE;
  virtual void OnButtonUp(
    int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * These methods are called on all registered manipulators, not just the
   * active one. Hence, these should just be used to record state and not
   * perform any interactions.
   * Overridden to capture if the x,y,z key is pressed.
   */
  virtual void OnKeyUp(vtkRenderWindowInteractor* iren) VTK_OVERRIDE;
  virtual void OnKeyDown(vtkRenderWindowInteractor* iren) VTK_OVERRIDE;
  //@}

  /**
   * Returns the currently pressed key code.
   */
  vtkGetMacro(KeyCode, char);

protected:
  vtkPVTrackballRotate();
  ~vtkPVTrackballRotate();

  char KeyCode;
  vtkPVTrackballRotate(const vtkPVTrackballRotate&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTrackballRotate&) VTK_DELETE_FUNCTION;
};

#endif
