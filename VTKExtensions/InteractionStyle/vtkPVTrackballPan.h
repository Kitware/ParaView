// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballPan : public vtkCameraManipulator
{
public:
  static vtkPVTrackballPan* New();
  vtkTypeMacro(vtkPVTrackballPan, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  ///@}

protected:
  vtkPVTrackballPan();
  ~vtkPVTrackballPan() override;

  vtkPVTrackballPan(const vtkPVTrackballPan&) = delete;
  void operator=(const vtkPVTrackballPan&) = delete;
};

#endif
