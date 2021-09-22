/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballEnvironmentRotate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  //@}

protected:
  vtkPVTrackballEnvironmentRotate() = default;
  ~vtkPVTrackballEnvironmentRotate() override = default;

  vtkPVTrackballEnvironmentRotate(const vtkPVTrackballEnvironmentRotate&) = delete;
  void operator=(const vtkPVTrackballEnvironmentRotate&) = delete;

  void EnvironmentRotate(vtkRenderer* ren, vtkRenderWindowInteractor* rwi);
};

#endif
