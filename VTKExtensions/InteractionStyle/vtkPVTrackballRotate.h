// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

  ///@{
  /**
   * These methods are called on all registered manipulators, not just the
   * active one. Hence, these should just be used to record state and not
   * perform any interactions.
   * Overridden to capture if the x,y,z key is pressed.
   */
  void OnKeyUp(vtkRenderWindowInteractor* iren) override;
  void OnKeyDown(vtkRenderWindowInteractor* iren) override;
  ///@}

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
