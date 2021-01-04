/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkZSpaceCamera
* @brief   Extends vtkOpenGLCamera to use custom view and projection matrix given by zSpace SDK.
 *
 * This is needed to change the view / projection matrix during a render(), depending on the
 * vtkCamera::LeftEye value (support for stereo).
*/

#ifndef vtkZSpaceCamera_h
#define vtkZSpaceCamera_h

#include "vtkNew.h"
#include "vtkOpenGLCamera.h"
#include "vtkZSpaceViewModule.h" // for export macro

class vtkZSpaceSDKManager;

class VTKZSPACEVIEW_EXPORT vtkZSpaceCamera : public vtkOpenGLCamera
{
public:
  static vtkZSpaceCamera* New();
  vtkTypeMacro(vtkZSpaceCamera, vtkOpenGLCamera);

  /**
   * Return the model view matrix of model view transform given by zSpace SDK.
   */
  virtual vtkMatrix4x4* GetModelViewTransformMatrix() override;

  /**
   * Return the projection transform matrix given by zSpace SDK.
   */
  virtual vtkMatrix4x4* GetProjectionTransformMatrix(
    double aspect, double nearz, double farz) override;

  /**
   * Set the zSpaceSDKManager used to get the custom zSpace
   * view and projection matrix.
   */
  vtkSetMacro(ZSpaceSDKManager, vtkZSpaceSDKManager*);

protected:
  vtkZSpaceCamera() = default;
  ~vtkZSpaceCamera() override = default;

  vtkZSpaceSDKManager* ZSpaceSDKManager;

private:
  vtkZSpaceCamera(const vtkZSpaceCamera&) = delete;
  void operator=(const vtkZSpaceCamera&) = delete;
};

#endif
