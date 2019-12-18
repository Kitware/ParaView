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
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballRotate : public vtkCameraManipulator
{
public:
  static vtkPVTrackballRotate* New();
  vtkTypeMacro(vtkPVTrackballRotate, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  //@}

  //@{
  /**
   * These methods are called on all registered manipulators, not just the
   * active one. Hence, these should just be used to record state and not
   * perform any interactions.
   * Overridden to capture if the x,y,z key is pressed.
   */
  void OnKeyUp(vtkRenderWindowInteractor* iren) override;
  void OnKeyDown(vtkRenderWindowInteractor* iren) override;
  //@}

  /**
   * Returns the currently pressed key code.
   */
  vtkGetMacro(KeyCode, char);

protected:
  vtkPVTrackballRotate();
  ~vtkPVTrackballRotate() override;

  char KeyCode;
  vtkPVTrackballRotate(const vtkPVTrackballRotate&) = delete;
  void operator=(const vtkPVTrackballRotate&) = delete;
};

#endif
