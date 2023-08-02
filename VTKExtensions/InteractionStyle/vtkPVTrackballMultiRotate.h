// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkPVTrackballMultiRotate
 *
 *
 *
 * This camera manipulator combines the vtkPVTrackballRotate and
 * vtkPVTrackballRoll manipulators in one.  Think of there being an invisible
 * sphere in the middle of the screen.  If you grab that sphere and move the
 * mouse, you will rotate that sphere.  However, if you grab outside that sphere
 * and move the mouse, you will roll the view.
 *
 */

#ifndef vtkPVTrackballMultiRotate_h
#define vtkPVTrackballMultiRotate_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class vtkCameraManipulator;
class vtkPVTrackballRoll;
class vtkPVTrackballRotate;

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballMultiRotate
  : public vtkCameraManipulator
{
public:
  vtkTypeMacro(vtkPVTrackballMultiRotate, vtkCameraManipulator);
  static vtkPVTrackballMultiRotate* New();
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
  vtkPVTrackballMultiRotate();
  ~vtkPVTrackballMultiRotate() override;

  vtkPVTrackballRotate* RotateManipulator;
  vtkPVTrackballRoll* RollManipulator;

  vtkCameraManipulator* CurrentManipulator;

private:
  vtkPVTrackballMultiRotate(const vtkPVTrackballMultiRotate&) = delete;
  void operator=(const vtkPVTrackballMultiRotate&) = delete;
};

#endif // vtkPVTrackballMultiRotate_h
