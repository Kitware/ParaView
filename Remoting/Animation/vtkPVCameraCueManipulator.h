/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraCueManipulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCameraManipulatorProxy
 * @brief   Manipulator for Camera animation.
 *
 * This is the manipulator for animating camera.
 * Unlike the base class, interpolation is not done by the Keyframe objects.
 * Instead, this class does the interpolation using the values in
 * the keyframe objects. All the keyframes added to a
 * vtkSMCameraManipulatorProxy must be vtkSMCameraKeyFrameProxy.
 * Like all animation proxies, this is a client side only proxy with no
 * VTK objects created on the server side.
 */

#ifndef vtkPVCameraCueManipulator_h
#define vtkPVCameraCueManipulator_h

#include "vtkCameraInterpolator.h" // needed for interpolation enum
#include "vtkPVKeyFrameCueManipulator.h"
#include "vtkRemotingAnimationModule.h" //needed for exports

class vtkSMProxy;

class VTKREMOTINGANIMATION_EXPORT vtkPVCameraCueManipulator : public vtkPVKeyFrameCueManipulator
{
public:
  static vtkPVCameraCueManipulator* New();
  vtkTypeMacro(vtkPVCameraCueManipulator, vtkPVKeyFrameCueManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Modes
  {
    CAMERA,
    PATH,
    FOLLOW_DATA
  };

  //@{
  /**
   * This manipulator has three modes:
   * \li CAMERA - the traditional mode using vtkCameraInterpolator where camera
   * values are directly interpolated.
   * \li PATH - the easy-to-use path  based interpolation where the camera
   * position/camera focal point paths can be explicitly specified.
   * We may eventually deprecate CAMERA mode since it may run out of usability
   * as PATH mode matures. So the code precariously meanders between the two
   * right now, but deprecating the old should help clean that up.
   * \li FOLLOW_DATA - the camera will follow the data set with the
   * SetDataSourceProxy() method.
   */
  vtkSetClampMacro(Mode, int, CAMERA, FOLLOW_DATA);
  vtkGetMacro(Mode, int);
  //@}

  //@{
  /**
   * Set the interpolation mode for the CAMERA mode.
   */
  vtkSetClampMacro(InterpolationMode, int, vtkCameraInterpolator::INTERPOLATION_TYPE_LINEAR,
    vtkCameraInterpolator::INTERPOLATION_TYPE_SPLINE);
  vtkGetMacro(InterpolationMode, int);
  //@}

  /**
   * Set the data source proxy. This is used when in the FOLLOW_DATA mode. The
   * camera will track the data referred to by the data source proxy.
   */
  void SetDataSourceProxy(vtkSMProxy* dataSourceProxy);

protected:
  vtkPVCameraCueManipulator();
  ~vtkPVCameraCueManipulator() override;

  int Mode;
  int InterpolationMode;

  void Initialize(vtkPVAnimationCue*) override;
  void Finalize(vtkPVAnimationCue*) override;
  /**
   * This updates the values based on currenttime.
   * currenttime is normalized to the time range of the Cue.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cueproxy) override;

  vtkCameraInterpolator* CameraInterpolator;
  vtkSMProxy* DataSourceProxy;

private:
  vtkPVCameraCueManipulator(const vtkPVCameraCueManipulator&) = delete;
  void operator=(const vtkPVCameraCueManipulator&) = delete;
};

#endif
