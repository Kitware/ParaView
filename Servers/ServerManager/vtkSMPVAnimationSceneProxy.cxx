/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVAnimationSceneProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVAnimationSceneProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVAnimationScene.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMPVAnimationSceneProxy);
vtkCxxRevisionMacro(vtkSMPVAnimationSceneProxy, "1.8");
vtkCxxSetObjectMacro(vtkSMPVAnimationSceneProxy, TimeKeeper, vtkSMProxy);
//-----------------------------------------------------------------------------
vtkSMPVAnimationSceneProxy::vtkSMPVAnimationSceneProxy()
{
  this->NumberOfFrames = 10;
  this->Duration = 10;
  this->ClockTimeRange[0] = this->ClockTimeRange[1] = 0;
  this->UpdatingTime = false;
  this->TimeKeeper = 0;
}

//-----------------------------------------------------------------------------
vtkSMPVAnimationSceneProxy::~vtkSMPVAnimationSceneProxy()
{
  this->SetTimeKeeper(0);
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->AnimationCue = vtkPVAnimationScene::New();
  this->InitializeObservers(this->AnimationCue);
  this->ObjectsCreated = 1;

  this->Superclass::CreateVTKObjects();
  this->SetTimeMode(vtkAnimationScene::TIMEMODE_RELATIVE);
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::SetPlayMode(int mode)
{
  switch (mode)
    {
  case SEQUENCE:
    this->SetFrameRate(1);
    this->SetStartTime(0);
    this->SetEndTime(this->NumberOfFrames-1);
    break;

  case REALTIME:
    this->SetFrameRate(1);
    this->SetStartTime(0);
    this->SetEndTime(this->Duration);
    break;

  case SNAP_TO_TIMESTEPS:
    this->SetFrameRate(1);
    this->SetStartTime(this->ClockTimeRange[0]);
    this->SetEndTime(this->ClockTimeRange[1]);
    break;
    }

  this->Superclass::SetPlayMode(mode);
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::SetClockTimeRange(double min, double max)
{
  if (this->ClockTimeRange[0] != min ||this->ClockTimeRange[1] != max)
    {
    this->ClockTimeRange[0] = min;
    this->ClockTimeRange[1] = max;
    if (this->PlayMode == SNAP_TO_TIMESTEPS)
      {
      this->SetStartTime(min);
      this->SetEndTime(max);
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::SetDuration(int secs)
{
  if (this->Duration != secs)
    {
    this->Duration = secs;
    if (this->PlayMode == REALTIME)
      {
      this->SetEndTime(this->Duration);
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::SetNumberOfFrames(int frames)
{
  if (this->NumberOfFrames != frames)
    {
    this->NumberOfFrames = frames;
    if (this->PlayMode == SEQUENCE)
      {
      this->SetEndTime(this->NumberOfFrames-1);
      }
    this->Modified();
    }

}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::AddTimeStep(double time)
{
  vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->AddTimeStep(time);
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::RemoveTimeStep(double time)
{
  vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->RemoveTimeStep(time);
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::RemoveAllTimeSteps()
{
  vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->RemoveAllTimeSteps();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMPVAnimationSceneProxy::GetNumberOfTimeSteps()
{
  return vtkPVAnimationScene::SafeDownCast(
    this->AnimationCue)->GetNumberOfTimeSteps();
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::GoToFirst()
{
  this->SetAnimationTime(this->GetStartTime()); 
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::GoToLast()
{
  this->SetAnimationTime(this->GetEndTime());
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::GoToNext()
{
  double current_animation_time = vtkPVAnimationScene::SafeDownCast(
      this->AnimationCue)->GetAnimationTime();

  this->UpdatingTime = true;
  switch (this->PlayMode)
    {
  case SEQUENCE:
  case REALTIME:
    // In sequence, we go to next frame, in realtime we go to next second.
    if (this->GetEndTime() >= current_animation_time+1)
      {
      this->SetAnimationTime(current_animation_time+1);
      }
    break;

  case SNAP_TO_TIMESTEPS:
    if (this->GetEndTime() != current_animation_time)
      {
      this->SetAnimationTime(
        vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->
        GetNextTimeStep(current_animation_time));
      }
    break;
    }
  this->UpdatingTime = false;
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::GoToPrevious()
{
  double current_animation_time = vtkPVAnimationScene::SafeDownCast(
      this->AnimationCue)->GetAnimationTime();

  this->UpdatingTime = true;
  switch (this->PlayMode)
    {
  case SEQUENCE:
  case REALTIME:
    // In sequence, we go to next frame, in realtime we go to next second.
    if (this->GetStartTime() <= current_animation_time-1)
      {
      this->SetAnimationTime(current_animation_time-1);
      }
    break;

  case SNAP_TO_TIMESTEPS:
    if (this->GetStartTime() != current_animation_time)
      {
      this->SetAnimationTime(
        vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->
        GetPreviousTimeStep(current_animation_time));
      }
    break;
    }
  this->UpdatingTime = false;
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::SetClockTime(double time)
{
  if (this->IsInPlay() || this->UpdatingTime)
    {
    return;
    }

  this->UpdatingTime = true;
  double normalized_time = 0;
  if (this->ClockTimeRange[0] != this->ClockTimeRange[1])
    {
    normalized_time = (time - this->ClockTimeRange[0])/
      (this->ClockTimeRange[1] - this->ClockTimeRange[0]);
    }
  if (normalized_time < 0.0)
    {
    normalized_time = 0;
    }
  if (normalized_time > 1.0)
    {
    normalized_time = 1.0;
    }

  double animation_start = this->GetStartTime();
  double animation_end = this->GetEndTime();

  double animation_time = 
    animation_start + normalized_time * (animation_end - animation_start);
  this->SetAnimationTime(animation_time);
  this->UpdatingTime = false;
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::SetFramesPerTimestep(int fpt)
{
  vtkPVAnimationScene::SafeDownCast(
    this->AnimationCue)->SetFramesPerTimestep(fpt);
}

//-----------------------------------------------------------------------------
int vtkSMPVAnimationSceneProxy::GetFramesPerTimestep()
{
  return vtkPVAnimationScene::SafeDownCast(
      this->AnimationCue)->GetFramesPerTimestep();
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::TickInternal(
  void* info)
{
  vtkAnimationCue::AnimationCueInfo *cueInfo = 
    reinterpret_cast<vtkAnimationCue::AnimationCueInfo*>(info);
  if (this->TimeKeeper)
    {
    int prev = vtkSMDataObjectDisplayProxy::GetUseCache();
    if (this->Caching)
      {
      vtkSMDataObjectDisplayProxy::SetUseCache(1);
      }

    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->TimeKeeper->GetProperty("Time"));
    if (this->PlayMode == SNAP_TO_TIMESTEPS)
      {
      dvp->SetElement(0, cueInfo->AnimationTime);
      // When in SNAP_TO_TIMESTEPS mode, the tick time is same as the clock time.
      }
    else
      {
      // In any other mode, tick time depends on the number of frames or duration 
      // in seconds. We've to compute the clock time using the start-end
      // times for the clock.
      double ntime = cueInfo->AnimationTime/(cueInfo->EndTime - cueInfo->StartTime);
      double current_time = this->ClockTimeRange[0] + (this->ClockTimeRange[1] -
        this->ClockTimeRange[0])*ntime;
      dvp->SetElement(0, current_time);
      vtkSMDataObjectDisplayProxy::SetUseCache(prev);
      }
    }

  this->Superclass::TickInternal(info);
}

//-----------------------------------------------------------------------------
void vtkSMPVAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfFrames: " << this->NumberOfFrames << endl;
  os << indent << "Duration: " << this->Duration << endl;
  os << indent << "ClockTimeRange: " << this->ClockTimeRange[0]
    << ", " << this->ClockTimeRange[1] << endl;
}
