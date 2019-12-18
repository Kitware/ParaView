/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
vtkPVKeyFrameAnimationCue::~vtkPVKeyFrameAnimationCue()
{
}

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
