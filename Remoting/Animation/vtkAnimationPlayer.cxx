// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnimationPlayer.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationScene.h"

//----------------------------------------------------------------------------
vtkAnimationPlayer::vtkAnimationPlayer()
{
  this->AnimationScene = nullptr;
  this->InPlay = false;
  this->CurrentTime = 0;
  this->StopPlay = false;
  this->Loop = false;
}

//----------------------------------------------------------------------------
vtkAnimationPlayer::~vtkAnimationPlayer()
{
  this->SetAnimationScene(nullptr);
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::SetAnimationScene(vtkSMAnimationScene* scene)
{
  if (this->AnimationScene != scene)
  {
    this->AnimationScene = scene;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkSMAnimationScene* vtkAnimationPlayer::GetAnimationScene()
{
  return this->AnimationScene;
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::Play(int dir /*=1*/)
{
  bool reverse = (dir == static_cast<int>(vtkAnimationCue::PlayDirection::BACKWARD));
  if (!this->AnimationScene)
  {
    vtkErrorMacro("No animation scene to play.");
    return;
  }

  if (this->InPlay)
  {
    const char* currentDir = reverse ? "backwards" : "forwards";
    vtkErrorMacro("Cannot play " << currentDir << " during an active playback.");
    return;
  }

  // tell animation components that playback started in a certain play direction.
  // typical observers would want to update their button icon and tool tip texts.
  this->InvokeEvent(vtkCommand::StartEvent, &reverse);

  double starttime = this->AnimationScene->GetStartTime();
  double endtime = this->AnimationScene->GetEndTime();
  this->CurrentTime = this->AnimationScene->GetSceneTime();
  double playbackWindow[2];
  this->AnimationScene->GetPlaybackTimeWindow(playbackWindow);
  if (playbackWindow[0] > playbackWindow[1])
  {
    playbackWindow[0] = this->AnimationScene->GetStartTime();
    playbackWindow[1] = endtime;
  }
  else
  {
    this->CurrentTime = playbackWindow[0];
  }

  // clamp current time to range
  const double& srcTime = reverse ? endtime : starttime;
  const bool needClamp = reverse ? (this->CurrentTime <= starttime || this->CurrentTime > endtime)
                                 : (this->CurrentTime < starttime || this->CurrentTime >= endtime);
  this->CurrentTime = needClamp ? srcTime : this->CurrentTime;

  // prepare to start animation.
  const double windowLength = playbackWindow[1] - playbackWindow[0];
  this->InPlay = true;
  this->StopPlay = false;

  auto withinTimeRange = [&]() -> bool { // returns true if animation can continue, false otherwise
    return reverse ? this->CurrentTime >= playbackWindow[0]
                   : this->CurrentTime <= playbackWindow[1];
  };
  auto getElapsedPercent = [&]() -> double { // return percent of animation completed.
    return 100. *
      (reverse ? playbackWindow[1] - this->CurrentTime : this->CurrentTime - playbackWindow[0]) /
      windowLength;
  };

  do
  {
    this->StartLoop(starttime, endtime, this->CurrentTime, playbackWindow);
    this->AnimationScene->Initialize();
    double deltatime = 0.0;
    while (!this->StopPlay && withinTimeRange())
    {
      this->AnimationScene->Tick(this->CurrentTime, deltatime, this->CurrentTime);
      double progress = getElapsedPercent() / 100.;
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

      double t =
        reverse ? this->GetPreviousTime(this->CurrentTime) : this->GetNextTime(this->CurrentTime);
      deltatime = t - this->CurrentTime;
      this->CurrentTime = t;
    }

    // Finalize will get called when Tick() is called with time>=endtime on the
    // cue. However,no harm is calling this method again since it has any effect
    // only the first time it gets called.
    // TODO: not sure what's the right thing here.
    // this->AnimationScene->Finalize();

    this->CurrentTime = reverse ? endtime : starttime;
    this->EndLoop();
    // loop when this->Loop is true.
  } while (this->Loop && !this->StopPlay);

  this->InPlay = false;
  this->StopPlay = false;

  // tell animation components that playback ended in a certain play direction.
  // typical observers would want to update their button icon and tool tip texts.
  this->InvokeEvent(vtkCommand::EndEvent, &reverse);
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::Stop()
{
  if (this->InPlay)
  {
    this->StopPlay = true;
  }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToFirst()
{
  this->Stop();
  if (this->AnimationScene)
  {
    this->AnimationScene->SetSceneTime(this->AnimationScene->GetStartTime());
  }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToLast()
{
  this->Stop();
  if (this->AnimationScene)
  {
    this->AnimationScene->SetSceneTime(this->AnimationScene->GetEndTime());
  }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToNext()
{
  this->Stop();
  double starttime = this->AnimationScene->GetStartTime();
  double endtime = this->AnimationScene->GetEndTime();
  double time = this->GoToNext(starttime, endtime, this->AnimationScene->GetSceneTime());

  if (time >= starttime && time < endtime)
  {
    this->AnimationScene->SetSceneTime(time);
  }
  else
  {
    this->AnimationScene->SetSceneTime(endtime);
  }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToPrevious()
{
  this->Stop();
  double starttime = this->AnimationScene->GetStartTime();
  double endtime = this->AnimationScene->GetEndTime();
  double time = this->GoToPrevious(starttime, endtime, this->AnimationScene->GetSceneTime());

  if (time >= starttime && time < endtime)
  {
    this->AnimationScene->SetSceneTime(time);
  }
  else
  {
    this->AnimationScene->SetSceneTime(starttime);
  }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
