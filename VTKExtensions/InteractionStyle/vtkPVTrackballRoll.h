// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVTrackballRoll
 * @brief   Rolls camera around a point.
 *
 * vtkPVTrackballRoll allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * Roll tracks the mouse around the center of rotation.
 */

#ifndef vtkPVTrackballRoll_h
#define vtkPVTrackballRoll_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballRoll : public vtkCameraManipulator
{
public:
  static vtkPVTrackballRoll* New();
  vtkTypeMacro(vtkPVTrackballRoll, vtkCameraManipulator);
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

protected:
  vtkPVTrackballRoll();
  ~vtkPVTrackballRoll() override;

  vtkPVTrackballRoll(const vtkPVTrackballRoll&) = delete;
  void operator=(const vtkPVTrackballRoll&) = delete;
};

#endif
