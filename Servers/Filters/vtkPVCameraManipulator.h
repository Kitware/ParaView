/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCameraManipulatorProxy - Manipulator for Camera animation.
// .SECTION Description
// This is the manipulator for animating camera.
// Unlike the base class, interpolation is not done by the Keyframe objects.
// Instead, this class does the interpolation using the values in
// the keyframe objects. All the keyframes added to a
// vtkSMCameraManipulatorProxy must be vtkSMCameraKeyFrameProxy.
// Like all animation proxies, this is a client side only proxy with no
// VTK objects created on the server side.

#ifndef __vtkPVCameraManipulator_h
#define __vtkPVCameraManipulator_h

#include "vtkPVKeyFrameCueManipulator.h"

class vtkCameraInterpolator;

class VTK_EXPORT vtkPVCameraManipulator : public vtkPVKeyFrameCueManipulator
{
public:
  static vtkPVCameraManipulator* New();
  vtkTypeMacro(vtkPVCameraManipulator, vtkPVKeyFrameCueManipulator);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum Modes
    {
    CAMERA,
    PATH
    };
  //ETX

  // Description:
  // This manipulator has two modes:
  // \li CAMERA - the traditional mode using vtkCameraInterpolator where camera
  // values are directly interpolated.
  // \li PATH - the easy-to-use path  based interpolation where the camera
  // position/camera focal point paths can be explicitly specified.
  // We may eventually deprecate CAMERA mode since it may run out of usability
  // as PATH mode matures. So the code precariously meanders between the two
  // right now, but deprecating the old should help clean that up.
  vtkSetClampMacro(Mode, int, CAMERA, PATH);
  vtkGetMacro(Mode, int);

protected:
  vtkPVCameraManipulator();
  ~vtkPVCameraManipulator();

  int Mode;

  virtual void Initialize(vtkPVAnimationCue*);
  virtual void Finalize(vtkPVAnimationCue*);
  // Description:
  // This updates the values based on currenttime.
  // currenttime is normalized to the time range of the Cue.
  virtual void UpdateValue(double currenttime,
                           vtkPVAnimationCue* cueproxy);

  vtkCameraInterpolator* CameraInterpolator;
private:
  vtkPVCameraManipulator(const vtkPVCameraManipulator&); // Not implemented.
  void operator=(const vtkPVCameraManipulator&); // Not implemented.

};

#endif
