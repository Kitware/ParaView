/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCameraKeyFrameProxy
// .SECTION Description
// Special key frame for animating Camera. Unlike typical keyframes,
// this keyframe interpolates a proxy and not a property of the proxy.
// A vtkSMCameraManipulatorProxy can only take vtkSMCameraKeyFrameProxy.
// Like all animation proxies, this is a client side only proxy with no
// VTK objects created on the server side.

#ifndef __vtkSMCameraKeyFrameProxy_h
#define __vtkSMCameraKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class vtkCamera;
class VTK_EXPORT vtkSMCameraKeyFrameProxy : public vtkSMKeyFrameProxy
{
public:
  static vtkSMCameraKeyFrameProxy* New();
  vtkTypeMacro(vtkSMCameraKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If the vtkSMCameraManipulatorProxy is in CAMERA mode, then this method is
  // not even called since the interpolation is done by vtkCameraInterpolator
  // maintained by vtkSMCameraManipulatorProxy itself. However,  in PATH mode,
  // this method is called to allow the key frame to use vtkCameraInterpolator2
  // to do path-based interpolations for the camera.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy,
    vtkSMKeyFrameProxy* next);
 
  // Description:
  // Updates the keyframe's current value using the camera.
  // This is a convenience method, it updates the properties on this proxy.
  void CopyValue(vtkCamera*);

  // Overridden, since these methods are not supported by this class.
  virtual void SetKeyValue(unsigned int , double ) { }
  virtual double GetKeyValue(unsigned int) {return 0;}

  // Description:
  // Get the camera i.e. the key value for this key frame.
  vtkGetObjectMacro(Camera, vtkCamera);

  // Description:
  // Methods to set the current camera value.
  void SetPosition(double x, double y, double z);
  void SetFocalPoint(double x, double y, double z);
  void SetViewUp(double x, double y, double z);
  void SetViewAngle(double angle);
  void SetParallelScale(double scale);

protected:
  vtkSMCameraKeyFrameProxy();
  ~vtkSMCameraKeyFrameProxy();

  vtkCamera* Camera;

private:
  vtkSMCameraKeyFrameProxy(const vtkSMCameraKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMCameraKeyFrameProxy&); // Not implemented.
};


#endif
