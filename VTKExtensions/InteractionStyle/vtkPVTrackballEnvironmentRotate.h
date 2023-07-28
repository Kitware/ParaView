// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVTrackballEnvironmentRotate
 * @brief   Rotates the environment with xy mouse movement.
 *
 * vtkPVTrackballEnvironmentRotate allows the user to rotate the renderer environment.
 */

#ifndef vtkPVTrackballEnvironmentRotate_h
#define vtkPVTrackballEnvironmentRotate_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballEnvironmentRotate
  : public vtkCameraManipulator
{
public:
  static vtkPVTrackballEnvironmentRotate* New();
  vtkTypeMacro(vtkPVTrackballEnvironmentRotate, vtkCameraManipulator);

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

protected:
  vtkPVTrackballEnvironmentRotate() = default;
  ~vtkPVTrackballEnvironmentRotate() override = default;

  vtkPVTrackballEnvironmentRotate(const vtkPVTrackballEnvironmentRotate&) = delete;
  void operator=(const vtkPVTrackballEnvironmentRotate&) = delete;

  void EnvironmentRotate(vtkRenderer* ren, vtkRenderWindowInteractor* rwi);
};

#endif
