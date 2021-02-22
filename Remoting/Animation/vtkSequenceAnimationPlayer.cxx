/*=========================================================================

  Program:   ParaView
  Module:    vtkSequenceAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSequenceAnimationPlayer.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSequenceAnimationPlayer);
//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::vtkSequenceAnimationPlayer()
{
  this->NumberOfFrames = 10;
  this->FrameNo = 0;
}

//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::~vtkSequenceAnimationPlayer() = default;

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::StartLoop(
  double starttime, double endtime, double vtkNotUsed(curtime), double* playbackWindow)
{
  // the frame index is inited to 0 ONLY when an animation is not resumed from
  // an intermediate frame
  this->FrameNo = 0;

  this->StartTime = starttime;
  this->EndTime = endtime;

  // currenttime, which might be the 'scene time' (usually unequal to
  // starttime) upon the previous pause / stop operation (if any), is used to
  // determine the actual frame index from which to resume the animation
  if (playbackWindow[0] > starttime)
  {
    // let's resume from the frame NEXT to the one on which the animation WAS
    // paused / stopped
    this->FrameNo = static_cast<int>((playbackWindow[0] - this->StartTime) *
                        (this->NumberOfFrames - 1) / (this->EndTime - this->StartTime) +
                      0.5) +
      1;

    // Let's compute the upper bounds in Frame unit
    this->MaxFrameWindow = static_cast<int>((playbackWindow[1] - this->StartTime) *
                               (this->NumberOfFrames - 1) / (this->EndTime - this->StartTime) +
                             0.5) +
      1;
  }
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GetNextTime(double curtime)
{
  if (this->FrameNo == 0 && curtime > this->StartTime)
  {
    // Invalid Frame No, compute it correctly
    this->FrameNo = static_cast<int>((curtime - this->StartTime) * (this->NumberOfFrames - 1) /
                        (this->EndTime - this->StartTime) +
                      0.5) +
      1;
  }
  this->FrameNo++;
  if (this->StartTime >= this->EndTime && this->FrameNo >= this->MaxFrameWindow)
  {
    return VTK_DOUBLE_MAX;
  }

  double time = this->StartTime +
    ((this->EndTime - this->StartTime) * this->FrameNo) / (this->NumberOfFrames - 1);
  return time;
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToNext(double start, double end, double curtime)
{
  double delta = static_cast<double>(end - start) / (this->NumberOfFrames - 1);
  return (curtime + delta);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToPrevious(double start, double end, double curtime)
{
  double delta = static_cast<double>(end - start) / (this->NumberOfFrames - 1);
  return (curtime - delta);
}

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
