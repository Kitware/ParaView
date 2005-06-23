/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraManipulatorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraManipulatorProxy.h"

#include "vtkCamera.h"
#include "vtkCameraInterpolator.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkQuaternionInterpolator.h"

#include <math.h>

#define QUATERNION 1
static void MtoQ(double m[4][4], double q[4])
{
  q[0] =  sqrt(m[0][0] + m[1][1] + m[2][2] + m[3][3]) / 2.0;
  double s4 = 4*q[0];
  q[1] = m[2][1] - m[1][2] / s4;
  q[2] = m[0][2] - m[2][0] / s4;
  q[3] = m[1][0] - m[0][1] / s4;
  double mag = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
  cout << "Magnitude: " << mag << endl;
  for (int i=0; i < 4; i++)
    {
    q[i] /= mag;
    }
}

static void QtoM(double q[4], double m[4][4])
{
  double mag = sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
  
  cout << "Interpolated Magnitude: " << mag << endl;
  for (int i=0; i < 4; i++)
    {
    q[i] /= mag;
    }
  double &s = q[0];
  double &x = q[1];
  double &y = q[2];
  double &z = q[3];
  
  double xx = x*x;
  double yy = y*y;
  double zz = z*z;
  double xy2 = 2*x*y;
  double yz2 = 2*y*z;
  double xz2 = 2*z*x;
  double sz2 = 2*s*z;
  double sx2 = 2*s*x;
  double sy2 = 2*s*y;

  
  m[0][0] = 1.0 - 2.0 * (yy + zz); 
  m[0][1] = xy2 - sz2;
  m[0][2] = sy2 + xz2;
  m[0][3] = 0;
  
  m[1][0] = xy2 + sz2;
  m[1][1] = 1.0 - 2.0 * (xx + zz);
  m[1][2] = -sx2 + yz2;
  m[1][3] = 0;

  m[2][0] = -sy2 + xz2;
  m[2][1] = sx2 + yz2;
  m[2][2] = 1.0 - 2.0 * (xx + yy);
  m[2][3] = 0;

  m[3][0] = m[3][1] = m[3][2] = 0.0;
  m[3][3] = 1.0;

  for (int x = 0; x < 4; x++)
    {
    for (int y=0; y < 4; y++)
      cout << m[x][y] << " ";
    cout << endl;
    }
  
}

vtkStandardNewMacro(vtkSMCameraManipulatorProxy);
vtkCxxRevisionMacro(vtkSMCameraManipulatorProxy, "1.2");
//------------------------------------------------------------------------------
vtkSMCameraManipulatorProxy::vtkSMCameraManipulatorProxy()
{
  this->CameraInterpolator = vtkCameraInterpolator::New();
  this->QuaternionInterpolator = vtkQuaternionInterpolator::New();
}

//------------------------------------------------------------------------------
vtkSMCameraManipulatorProxy::~vtkSMCameraManipulatorProxy()
{
  this->CameraInterpolator->Delete();
  this->QuaternionInterpolator->Delete();
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::Initialize(vtkSMAnimationCueProxy* cue)
{
  this->Superclass::Initialize(cue);
  int nos = this->GetNumberOfKeyFrames();
#ifdef QUATERNION
  this->CameraInterpolator->Initialize();
  if (nos < 2)
    {
    vtkErrorMacro("Too few keyframes to animate.");
    return;
    }
  
  for(int i=0; i < nos; i++)
    {
    vtkSMCameraKeyFrameProxy* kf = vtkSMCameraKeyFrameProxy::SafeDownCast(
      this->GetKeyFrameAtIndex(i));
    if (!kf)
      {
      vtkErrorMacro("All keyframes in a vtkSMCameraKeyFrameProxy must be "
        "vtkSMCameraKeyFrameProxy");
      continue;
      }
    this->CameraInterpolator->AddCamera(kf->GetKeyTime(), kf->GetCamera());
    }
#else
  this->QuaternionInterpolator->Initialize();
  for (int i = 0; i < nos; i++)
    {
    vtkSMCameraKeyFrameProxy* kf = vtkSMCameraKeyFrameProxy::SafeDownCast(
      this->GetKeyFrameAtIndex(i));
    if (!kf)
      {
      vtkErrorMacro("All keyframes in a vtkSMCameraKeyFrameProxy must be "
        "vtkSMCameraKeyFrameProxy");
      continue;
      }
    ;
    double q[4];
    ::MtoQ(kf->GetCamera()->GetViewTransformMatrix()->Element, q);
    this->QuaternionInterpolator->AddQuaternion(kf->GetKeyTime(),q);
    }
#endif
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::Finalize(vtkSMAnimationCueProxy* cue)
{
  this->Superclass::Finalize(cue);
}

//------------------------------------------------------------------------------
#define CameraToProperty(proxy, camera, propertyid) \
{\
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(\
    proxy->GetProperty("Camera" #propertyid));\
  if (dvp) \
    {\
    dvp->SetElements(camera->Get##propertyid());\
    }\
  else \
    {\
    vtkErrorMacro("Failed to find property Camera" #propertyid );\
    }\
}

#define CameraToPropertySingleElement(proxy, camera, propertyid) \
{\
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(\
    proxy->GetProperty("Camera" #propertyid));\
  if (dvp) \
    {\
    dvp->SetElement(0,camera->Get##propertyid());\
    }\
  else \
    {\
    vtkErrorMacro("Failed to find property Camera" #propertyid );\
    }\
}
//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::UpdateValue(double currenttime,
  vtkSMAnimationCueProxy* cue)
{
  vtkSMProxy* cameraProxy = cue->GetAnimatedProxy();
  vtkCamera* interpolatedCamera = vtkCamera::New();
#ifdef QUATERNION
  this->CameraInterpolator->InterpolateCamera(currenttime, interpolatedCamera);
#else
  double q[4];
  this->QuaternionInterpolator->InterpolateQuaternion(currenttime, q);
  ::QtoM(q, interpolatedCamera->GetViewTransformMatrix()->Element);
#endif
  CameraToProperty(cameraProxy, interpolatedCamera, Position); 
  CameraToProperty(cameraProxy, interpolatedCamera, FocalPoint); 
  CameraToProperty(cameraProxy, interpolatedCamera, ViewUp); 
  CameraToProperty(cameraProxy, interpolatedCamera, ClippingRange); 
  CameraToPropertySingleElement(cameraProxy, interpolatedCamera, ViewAngle);
  CameraToPropertySingleElement(cameraProxy, interpolatedCamera, ParallelScale);
  
  cameraProxy->UpdateVTKObjects(); 
  interpolatedCamera->Delete();
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
