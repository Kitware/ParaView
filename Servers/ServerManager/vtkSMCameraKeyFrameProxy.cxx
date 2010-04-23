/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraKeyFrameProxy.h"

#include "vtkCamera.h"
#include "vtkCameraInterpolator2.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMCameraManipulatorProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMCameraKeyFrameProxy);
//----------------------------------------------------------------------------
vtkSMCameraKeyFrameProxy::vtkSMCameraKeyFrameProxy()
{
  this->Camera = vtkCamera::New();
}

//----------------------------------------------------------------------------
vtkSMCameraKeyFrameProxy::~vtkSMCameraKeyFrameProxy()
{
  this->Camera->Delete();
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::CopyValue(vtkCamera* camera)
{
  if (camera)
    {
    vtkSMPropertyHelper(this, "Position").Set(camera->GetPosition(), 3);
    vtkSMPropertyHelper(this, "FocalPoint").Set(camera->GetFocalPoint(), 3);
    vtkSMPropertyHelper(this, "ViewUp").Set(camera->GetViewUp(), 3);
    vtkSMPropertyHelper(this, "ViewAngle").Set(0, camera->GetViewAngle());
    vtkSMPropertyHelper(this, "ParallelScale").Set(0,
      camera->GetParallelScale());
    this->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetPosition(double x, double y, double z)
{
  this->Camera->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetFocalPoint(double x, double y, double z)
{
  this->Camera->SetFocalPoint(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetViewUp(double x, double y, double z)
{
  this->Camera->SetViewUp(x, y, z);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetViewAngle(double angle)
{
  this->Camera->SetViewAngle(angle);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::SetParallelScale(double scale)
{
  this->Camera->SetParallelScale(scale);
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::UpdateValue(double currenttime,
  vtkSMAnimationCueProxy* cueProxy,
  vtkSMKeyFrameProxy* next)
{
  if (next == this)
    {
    assert(currenttime == 0.0);
    // Happens for the last keyframe. In PATH based animations, the last
    // keyframe is bogus, we really want to the previous keyframe to handle
    // this. So we do that.
    vtkSMCameraManipulatorProxy* manip =
      vtkSMCameraManipulatorProxy::SafeDownCast(
        cueProxy->GetManipulator());
    if (manip)
      {
      vtkSMKeyFrameProxy* kf = manip->GetPreviousKeyFrame(this);
      if (kf && kf != this)
        {
        kf->UpdateValue(1.0, cueProxy, this);
        return;
        }
      }
    }
  vtkSMProxy* cameraProxy = cueProxy->GetAnimatedProxy();
  if (!cameraProxy)
    {
    vtkErrorMacro("Don't know what to animate. "
      "Please set the AnimatedProxy on the animation cue.");
    return;
    }
  
  vtkCamera* camera = vtkCamera::New();
  camera->SetPosition(this->Camera->GetPosition());
  camera->SetFocalPoint(this->Camera->GetFocalPoint());
  camera->SetViewUp(this->Camera->GetViewUp());
  camera->SetViewAngle(this->Camera->GetViewAngle());
  camera->SetParallelScale(this->Camera->GetParallelScale());

  vtkCameraInterpolator2* interpolator = vtkCameraInterpolator2::SafeDownCast(
    this->GetClientSideObject());
  if (!interpolator)
    {
    vtkErrorMacro("Failed to locate vtkCameraInterpolator2.");
    return;
    }
  interpolator->InterpolateCamera(currenttime, camera);

  vtkSMPropertyHelper(cameraProxy, "CameraPosition").Set(camera->GetPosition(), 3);
  vtkSMPropertyHelper(cameraProxy, "CameraFocalPoint").Set(camera->GetFocalPoint(), 3);
  vtkSMPropertyHelper(cameraProxy, "CameraViewUp").Set(camera->GetViewUp(), 3);
  vtkSMPropertyHelper(cameraProxy, "CameraViewAngle").Set(0, camera->GetViewAngle());
  vtkSMPropertyHelper(cameraProxy, "CameraParallelScale").Set(0,
    camera->GetParallelScale());
  camera->Delete();
  cameraProxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMCameraKeyFrameProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Camera: " << this->Camera << endl;
}
