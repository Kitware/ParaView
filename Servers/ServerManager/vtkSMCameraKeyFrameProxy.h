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
  vtkTypeRevisionMacro(vtkSMCameraKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Camera interpolation is not performed by the Keyframe proxy.
  // Instead the Manipulator manages it.
  // Thus method does nothing.
  virtual void UpdateValue(double ,
    vtkSMAnimationCueProxy* , vtkSMKeyFrameProxy* )  { }
 
  // Description:
  // Since this keyframe animates a proxy, the KeyValue at a given time
  // is also a proxy (Camera Proxy). The info properties are updated
  // (UpdateInformation) and then the values are stored. 
  void SetKeyValue(vtkSMProxy* cameraProxy);

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

  // Description:
  // Saves the proxy in batch.
  virtual void SaveInBatchScript(ofstream* file);
protected:
  vtkSMCameraKeyFrameProxy();
  ~vtkSMCameraKeyFrameProxy();
  vtkCamera* Camera;
  
private:
  vtkSMCameraKeyFrameProxy(const vtkSMCameraKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMCameraKeyFrameProxy&); // Not implemented.
};


#endif
