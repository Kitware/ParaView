/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballSkyboxRotate.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTrackballSkyboxRotate
 * @brief   Rotates the environment skybox with xy mouse movement.
 *
 * vtkPVTrackballSkyboxRotate allows the user to rotate a
 * Skybox in a renderer.
*/

#ifndef vtkPVTrackballSkyboxRotate_h
#define vtkPVTrackballSkyboxRotate_h

#include "vtkCameraManipulator.h"
#include "vtkPVVTKExtensionsInteractionStyleModule.h" // needed for export macro

class vtkSkybox;

class VTKPVVTKEXTENSIONSINTERACTIONSTYLE_EXPORT vtkPVTrackballSkyboxRotate
  : public vtkCameraManipulator
{
public:
  static vtkPVTrackballSkyboxRotate* New();
  vtkTypeMacro(vtkPVTrackballSkyboxRotate, vtkCameraManipulator);

  //@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  //@}

  /**
   * Set the skybox that will be rotated.
   */
  vtkSetMacro(Skybox, vtkSkybox*);

protected:
  vtkPVTrackballSkyboxRotate() = default;
  ~vtkPVTrackballSkyboxRotate() override = default;

  vtkPVTrackballSkyboxRotate(const vtkPVTrackballSkyboxRotate&) = delete;
  void operator=(const vtkPVTrackballSkyboxRotate&) = delete;

  void EnvironmentRotate(vtkRenderer* ren, vtkRenderWindowInteractor* rwi);

  vtkSkybox* Skybox = nullptr;
};

#endif
