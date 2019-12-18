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
#include "vtkPVDataInformation.h"
#include "vtkPVRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkPVCameraCueManipulator);
//------------------------------------------------------------------------------
vtkPVCameraCueManipulator::vtkPVCameraCueManipulator()
{
  this->Mode = PATH;
  this->CameraInterpolator = vtkCameraInterpolator::New();
  this->DataSourceProxy = 0;
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
  this->CameraInterpolator->SetInterpolationType(this->InterpolationMode);

  if (nos < 2)
  {
    vtkErrorMacro("Too few keyframes to animate.");
    return;
  }

  if (this->Mode == PATH || this->Mode == FOLLOW_DATA)
  {
    // No need to initialize this->CameraInterpolator in PATH or
    // FOLLOW_DATA mode
    return;
  }

  // Set up this->CameraInterpolator.
  for (int i = 0; i < nos; i++)
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
void vtkPVCameraCueManipulator::UpdateValue(double currenttime, vtkPVAnimationCue* cue)
{
  vtkPVCameraAnimationCue* cameraCue = vtkPVCameraAnimationCue::SafeDownCast(cue);
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
  else if (this->Mode == FOLLOW_DATA)
  {
    vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(this->DataSourceProxy);
    if (proxy)
    {
      // Usually the pipeline will already be updated by the vtkSMAnimationScene before
      // the cue "tick" is called.  But if the representation the cue is following is
      // not visible then it will not be updated.  Force it to update here (this should
      // be a no-op for data that is visible in a view).
      vtkSMProxy* timeKeeper = cameraCue->GetTimeKeeper();
      if (timeKeeper)
      {
        double time = vtkSMPropertyHelper(timeKeeper, "Time").GetAsDouble();
        proxy->UpdatePipeline(time);

        // get the data bounds
        vtkPVDataInformation* info = proxy->GetDataInformation();
        double bounds[6];
        info->GetBounds(bounds);

        // calculate the center of the data
        vtkVector3d center((bounds[0] + bounds[1]) * 0.5, (bounds[2] + bounds[3]) * 0.5,
          (bounds[4] + bounds[5]) * 0.5);

        // move the camera's position and focal point based on
        // the data's position at the current time
        vtkVector3d position;
        cameraCue->GetCamera()->GetPosition(&position[0]);

        vtkVector3d focalPoint;
        cameraCue->GetCamera()->GetFocalPoint(&focalPoint[0]);

        cameraCue->GetCamera()->SetFocalPoint(&center[0]);

        position = center + (position - focalPoint);
        cameraCue->GetCamera()->SetPosition(&position[0]);
      }
      else
      {
        vtkWarningMacro("No Time Keepr for Follow Data Animation");
      }
    }
    else
    {
      vtkWarningMacro("No Data Source for Follow-Data Animation");
    }
  }
  else
  {
    this->Superclass::UpdateValue(currenttime, cue);
  }
}

//------------------------------------------------------------------------------
void vtkPVCameraCueManipulator::SetDataSourceProxy(vtkSMProxy* dataSourceProxy)
{
  if (dataSourceProxy != this->DataSourceProxy)
  {
    this->DataSourceProxy = dataSourceProxy;
  }
}

//------------------------------------------------------------------------------
void vtkPVCameraCueManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode:" << this->Mode << endl;
}
