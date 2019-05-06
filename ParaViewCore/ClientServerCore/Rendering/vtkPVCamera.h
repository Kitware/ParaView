/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCamera.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCamera
 *
 * vtkPVCamera extends vtkOpenGLCamera with controls that are specific to
 * ray tracing. When ray tracing is not enabled, at compile or runtime,
 * they do nothing.
*/

#ifndef vtkPVCamera_h
#define vtkPVCamera_h

#include "vtkOpenGLCamera.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVCamera : public vtkOpenGLCamera
{
public:
  static vtkPVCamera* New();
  vtkTypeMacro(vtkPVCamera, vtkOpenGLCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Enables depth of field for the perspective camera.
   */
  void SetDepthOfField(int dof);

  /**
   * Checks if depth of field is enabled.
   */
  int GetDepthOfField();

protected:
  vtkPVCamera();
  ~vtkPVCamera();

private:
  vtkPVCamera(const vtkPVCamera&) = delete;
  void operator=(const vtkPVCamera&) = delete;
};

#endif
