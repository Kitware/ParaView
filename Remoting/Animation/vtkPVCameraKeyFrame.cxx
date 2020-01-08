/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraKeyFrame.h"

#include "vtkCamera.h"
#include "vtkCameraInterpolator2.h"
#include "vtkObjectFactory.h"
#include "vtkPVCameraAnimationCue.h"
#include "vtkPVCameraCueManipulator.h"
#include "vtkPVKeyFrame.h"

#include <assert.h>

vtkStandardNewMacro(vtkPVCameraKeyFrame);
//----------------------------------------------------------------------------
vtkPVCameraKeyFrame::vtkPVCameraKeyFrame()
{
  this->Camera = vtkCamera::New();
  this->Interpolator = vtkCameraInterpolator2::New();
}

//----------------------------------------------------------------------------
vtkPVCameraKeyFrame::~vtkPVCameraKeyFrame()
{
  this->Camera->Delete();
  this->Interpolator->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetPosition(double x, double y, double z)
{
  this->Camera->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetFocalPoint(double x, double y, double z)
{
  this->Camera->SetFocalPoint(x, y, z);
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetViewUp(double x, double y, double z)
{
  this->Camera->SetViewUp(x, y, z);
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetViewAngle(double angle)
{
  this->Camera->SetViewAngle(angle);
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetParallelScale(double scale)
{
  this->Camera->SetParallelScale(scale);
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::UpdateValue(
  double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next)
{
  vtkPVCameraAnimationCue* cameraCue = vtkPVCameraAnimationCue::SafeDownCast(cue);
  if (!cameraCue)
  {
    vtkErrorMacro("This keyframe can only be added to "
                  "vtkPVCameraCueManipulator.");
    return;
  }
  if (!cameraCue->GetCamera())
  {
    return;
  }
  if (next == this)
  {
    assert(currenttime == 0.0);
    // Happens for the last keyframe. In PATH based animations, the last
    // keyframe is bogus, we really want to the previous keyframe to handle
    // this. So we do that.
    vtkPVCameraCueManipulator* manip;
    manip = vtkPVCameraCueManipulator::SafeDownCast(cue->GetManipulator());

    if (manip)
    {
      vtkPVKeyFrame* kf = manip->GetPreviousKeyFrame(this);
      if (kf && kf != this)
      {
        kf->UpdateValue(1.0, cue, this);
        return;
      }
    }
  }

  // Local vars
  vtkCamera* camera = vtkCamera::New();
  camera->ShallowCopy(this->Camera);
  this->Interpolator->InterpolateCamera(currenttime, camera);

  // Apply changes
  cue->BeginUpdateAnimationValues();
  vtkCamera* animatedCamera = cameraCue->GetCamera();
  animatedCamera->SetPosition(camera->GetPosition());
  animatedCamera->SetFocalPoint(camera->GetFocalPoint());
  animatedCamera->SetViewUp(camera->GetViewUp());
  animatedCamera->SetViewAngle(camera->GetViewAngle());
  animatedCamera->SetParallelScale(camera->GetParallelScale());
  cue->EndUpdateAnimationValues();
  camera->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::AddPositionPathPoint(double x, double y, double z)
{
  this->Interpolator->AddPositionPathPoint(x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::ClearPositionPath()
{
  this->Interpolator->ClearPositionPath();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::AddFocalPathPoint(double x, double y, double z)
{
  this->Interpolator->AddFocalPathPoint(x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::ClearFocalPath()
{
  this->Interpolator->ClearFocalPath();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetFocalPointMode(int val)
{
  this->Interpolator->SetFocalPointMode(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetPositionMode(int val)
{
  this->Interpolator->SetPositionMode(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetClosedFocalPath(bool val)
{
  this->Interpolator->SetClosedFocalPath(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetClosedPositionPath(bool val)
{
  this->Interpolator->SetClosedPositionPath(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Camera: " << this->Camera << endl;
}
