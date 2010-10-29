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
#include "vtkObjectFactory.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMCameraManipulatorProxy);
//------------------------------------------------------------------------------
vtkSMCameraManipulatorProxy::vtkSMCameraManipulatorProxy()
{
  this->Mode = PATH;
  this->CameraInterpolator = vtkCameraInterpolator::New();
}

//------------------------------------------------------------------------------
vtkSMCameraManipulatorProxy::~vtkSMCameraManipulatorProxy()
{
  this->CameraInterpolator->Delete();
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::Initialize(vtkSMAnimationCueProxy* cue)
{
  this->Superclass::Initialize(cue);
  int nos = this->GetNumberOfKeyFrames();
  this->CameraInterpolator->Initialize();
  this->CameraInterpolator->SetInterpolationTypeToSpline();
  if (nos < 2)
    {
    vtkErrorMacro("Too few keyframes to animate.");
    return;
    }
 
  if (this->Mode == PATH)
    {
    // No need to initialize this->CameraInterpolator in PATH mode.
    return;
    }

  // Set up this->CameraInterpolator.
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
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::Finalize(vtkSMAnimationCueProxy* cue)
{
  this->Superclass::Finalize(cue);
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::UpdateValue(double currenttime,
  vtkSMAnimationCueProxy* cue)
{
  if (this->Mode == CAMERA)
    {
    vtkSMProxy* renderViewProxy = cue->GetAnimatedProxy();
    vtkCamera* camera = vtkCamera::New();
    this->CameraInterpolator->InterpolateCamera(currenttime, camera);
    vtkSMPropertyHelper(renderViewProxy, "CameraPosition").Set(camera->GetPosition(), 3);
    vtkSMPropertyHelper(renderViewProxy, "CameraFocalPoint").Set(camera->GetFocalPoint(), 3);
    vtkSMPropertyHelper(renderViewProxy, "CameraViewUp").Set(camera->GetViewUp(), 3);
    vtkSMPropertyHelper(renderViewProxy, "CameraViewAngle").Set(0, camera->GetViewAngle());
    vtkSMPropertyHelper(renderViewProxy,
      "CameraClippingRange").Set(camera->GetClippingRange(), 2);
    vtkSMPropertyHelper(renderViewProxy, "CameraParallelScale").Set(0,
      camera->GetParallelScale());
    camera->Delete();
    renderViewProxy->UpdateVTKObjects();

#ifdef FIXME
    if (vtkSMRenderViewProxy::SafeDownCast(renderViewProxy))
      {
      vtkSMRenderViewProxy::SafeDownCast(renderViewProxy)->ResetCameraClippingRange();
      }
#endif
    }
  else
    {
    this->Superclass::UpdateValue(currenttime, cue);
    }
}

//------------------------------------------------------------------------------
void vtkSMCameraManipulatorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode:" << this->Mode << endl;
}
