// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVTrackballPanAxisConstrained
 * @brief   Pans camera with x y mouse movements.
 *
 * vtkPVTrackballPanAxisConstrained allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * Moving the mouse down zooms in. Up zooms out.
 * This manipulator has not been extended to parallel projection yet.
 * It works in perspective by rotating the camera.
 * This manipulator allows constraining the axis of movement
 */

#ifndef vtkTrackballPan_h
#define vtkTrackballPan_h

#include "vtkPVCameraManipulator.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro
#include <memory>                                     // for std::unique_ptr

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballPanAxisConstrained
  : public vtkPVCameraManipulator
{
public:
  static vtkPVTrackballPanAxisConstrained* New();
  vtkTypeMacro(vtkPVTrackballPanAxisConstrained, vtkPVCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void OnKeyUp(vtkRenderWindowInteractor*) override;
  void OnKeyDown(vtkRenderWindowInteractor*) override;

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
  vtkPVTrackballPanAxisConstrained();
  ~vtkPVTrackballPanAxisConstrained() override;

  vtkPVTrackballPanAxisConstrained(const vtkPVTrackballPanAxisConstrained&) = delete;
  void operator=(const vtkPVTrackballPanAxisConstrained&) = delete;

  struct Internal;
  std::unique_ptr<Internal> Internals;
};

#endif
