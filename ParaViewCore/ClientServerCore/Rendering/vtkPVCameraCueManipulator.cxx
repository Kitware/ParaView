/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraCueManipulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraCueManipulator.h"

#include "vtkCamera.h"
#include "vtkCameraInterpolator.h"
#include "vtkObjectFactory.h"
#include "vtkPVCameraAnimationCue.h"
#include "vtkPVCameraKeyFrame.h"
#include "vtkPVRenderView.h"

vtkStandardNewMacro(vtkPVCameraCueManipulator);
//------------------------------------------------------------------------------
vtkPVCameraCueManipulator::vtkPVCameraCueManipulator()
{
  this->Mode = PATH;
  this->CameraInterpolator = vtkCameraInterpolator::New();
}

//------------------------------------------------------------------------------
vtkPVCameraCueManipulator::~vtkPVCameraCueManipulator()
{
  this->CameraInterpolator->Delete();
}

//------------------------------------------------------------------------------
void vtkPVCameraCueManipulator::Initialize(vtkPVAnimationCue* cue)
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
    vtkPVCameraKeyFrame* kf;
    kf = vtkPVCameraKeyFrame::SafeDownCast(this->GetKeyFrameAtIndex(i));

    if (!kf)
      {
      vtkErrorMacro("All keyframes in a vtkPVCameraKeyFrame must be "
                    "vtkPVCameraKeyFrame");
      continue;
      }
    this->CameraInterpolator->AddCamera(kf->GetKeyTime(), kf->GetCamera());
    }
}

//------------------------------------------------------------------------------
void vtkPVCameraCueManipulator::Finalize(vtkPVAnimationCue* cue)
{
  this->Superclass::Finalize(cue);
}

//------------------------------------------------------------------------------
void vtkPVCameraCueManipulator::UpdateValue(double currenttime,
                                         vtkPVAnimationCue* cue)
{
  vtkPVCameraAnimationCue* cameraCue =
    vtkPVCameraAnimationCue::SafeDownCast(cue);
  if (!cameraCue)
    {
    vtkErrorMacro("This manipulator only works with vtkPVCameraAnimationCue.");
    return;
    }

  vtkCamera* animatedCamera = cameraCue->GetCamera();
  if (!animatedCamera)
    {
    vtkErrorMacro("No camera to animate.");
    return;
    }

  if (this->Mode == CAMERA)
    {
    vtkCamera* camera = vtkCamera::New();
    this->CameraInterpolator->InterpolateCamera(currenttime, camera);

    animatedCamera->SetPosition(camera->GetPosition());
    animatedCamera->SetFocalPoint(camera->GetFocalPoint());
    animatedCamera->SetViewUp(camera->GetViewUp());
    animatedCamera->SetViewAngle(camera->GetViewAngle());
    animatedCamera->SetParallelScale(camera->GetParallelScale());
    camera->Delete();

    cameraCue->GetView()->ResetCameraClippingRange();
    }
  else
    {
    this->Superclass::UpdateValue(currenttime, cue);
    }
}

//------------------------------------------------------------------------------
void vtkPVCameraCueManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode:" << this->Mode << endl;
}
