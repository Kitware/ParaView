// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVKeyFrameAnimationCue.h"

#include "vtkObjectFactory.h"
#include "vtkPVKeyFrameCueManipulator.h"

//----------------------------------------------------------------------------
vtkPVKeyFrameAnimationCue::vtkPVKeyFrameAnimationCue()
{
  vtkPVKeyFrameCueManipulator* manip = vtkPVKeyFrameCueManipulator::New();
  this->SetManipulator(manip);
  manip->Delete();
}

//----------------------------------------------------------------------------
vtkPVKeyFrameAnimationCue::~vtkPVKeyFrameAnimationCue() = default;

//----------------------------------------------------------------------------
vtkPVKeyFrameCueManipulator* vtkPVKeyFrameAnimationCue::GetKeyFrameManipulator()
{
  return vtkPVKeyFrameCueManipulator::SafeDownCast(this->GetManipulator());
}

//----------------------------------------------------------------------------
int vtkPVKeyFrameAnimationCue::AddKeyFrame(vtkPVKeyFrame* keyframe)
{
  return this->GetKeyFrameManipulator()->AddKeyFrame(keyframe);
}

//----------------------------------------------------------------------------
int vtkPVKeyFrameAnimationCue::GetLastAddedKeyFrameIndex()
{
  return this->GetKeyFrameManipulator()->GetLastAddedKeyFrameIndex();
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCue::RemoveKeyFrame(vtkPVKeyFrame* kf)
{
  this->GetKeyFrameManipulator()->RemoveKeyFrame(kf);
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCue::RemoveAllKeyFrames()
{
  this->GetKeyFrameManipulator()->RemoveAllKeyFrames();
}

//----------------------------------------------------------------------------
void vtkPVKeyFrameAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
