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
#include "vtkPVCameraManipulator.h"
#include "vtkPVAnimationCue.h"
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
void vtkPVCameraKeyFrame::CopyValue(vtkCamera* camera)
{
  if (camera)
    {
    this->Camera->SetPosition(camera->GetPosition());
    this->Camera->SetFocalPoint(camera->GetFocalPoint());
    this->Camera->SetViewUp(camera->GetViewUp());
    this->Camera->SetViewAngle(camera->GetViewAngle());
    this->Camera->SetParallelScale(camera->GetParallelScale());
    }
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
void vtkPVCameraKeyFrame::UpdateValue( double currenttime,
                                       vtkPVAnimationCue* cue,
                                       vtkPVKeyFrame* next)
{
  if (next == this)
    {
    assert(currenttime == 0.0);
    // Happens for the last keyframe. In PATH based animations, the last
    // keyframe is bogus, we really want to the previous keyframe to handle
    // this. So we do that.
    vtkPVCameraManipulator* manip;
    manip = vtkPVCameraManipulator::SafeDownCast(cue->GetManipulator());

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
  // FIXME: Utkarsh, can't we just do a DeepCopy ??? ++++++++++++++++++++++++++++
  camera->SetPosition(this->Camera->GetPosition());
  camera->SetFocalPoint(this->Camera->GetFocalPoint());
  camera->SetViewUp(this->Camera->GetViewUp());
  camera->SetViewAngle(this->Camera->GetViewAngle());
  camera->SetParallelScale(this->Camera->GetParallelScale());
  this->Interpolator->InterpolateCamera(currenttime, camera);

  // Apply changes
  cue->BeginUpdateAnimationValues();

  // FIXME how to set camera property correclty.... ++++++++++++++++++++++++++
  // cue->SetAnimationValue( ???? , this->GetKeyValue());
  //  vtkSMPropertyHelper(cameraProxy, "CameraPosition").Set(camera->GetPosition(), 3);
  //  vtkSMPropertyHelper(cameraProxy, "CameraFocalPoint").Set(camera->GetFocalPoint(), 3);
  //  vtkSMPropertyHelper(cameraProxy, "CameraViewUp").Set(camera->GetViewUp(), 3);
  //  vtkSMPropertyHelper(cameraProxy, "CameraViewAngle").Set(0, camera->GetViewAngle());
  //  vtkSMPropertyHelper(cameraProxy, "CameraParallelScale").Set(0,camera->GetParallelScale());

  cue->EndUpdateAnimationValues();
  camera->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCameraKeyFrame::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Camera: " << this->Camera << endl;
}
