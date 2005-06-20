/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraManipulatorProxy.h

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
// the keyframe objects.

#ifndef __vtkSMCameraManipulatorProxy_h
#define __vtkSMCameraManipulatorProxy_h

#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"

class vtkCameraInterpolator;

class VTK_EXPORT vtkSMCameraManipulatorProxy : 
  public vtkSMKeyFrameAnimationCueManipulatorProxy
{
public:
  static vtkSMCameraManipulatorProxy* New();
  vtkTypeRevisionMacro(vtkSMCameraManipulatorProxy, 
    vtkSMKeyFrameAnimationCueManipulatorProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMCameraManipulatorProxy();
  ~vtkSMCameraManipulatorProxy();
 
  virtual void Initialize(vtkSMAnimationCueProxy*);
  virtual void Finalize(vtkSMAnimationCueProxy*);
  // Description:
  // This updates the values based on currenttime. 
  // currenttime is normalized to the time range of the Cue.
  virtual void UpdateValue(double currenttime, 
    vtkSMAnimationCueProxy* cueproxy);

  vtkCameraInterpolator* CameraInterpolator;
private:
  vtkSMCameraManipulatorProxy(const vtkSMCameraManipulatorProxy&); // Not implemented.
  void operator=(const vtkSMCameraManipulatorProxy&); // Not implemented.
  
};

#endif

