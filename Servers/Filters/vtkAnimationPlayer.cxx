/*=========================================================================

  Program:   ParaView
  Module:    vtkAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnimationPlayer.h"

#include "vtkPVAnimationScene.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkAnimationPlayer, "1.1");
vtkCxxSetObjectMacro(vtkAnimationPlayer, AnimationScene, vtkPVAnimationScene);
//----------------------------------------------------------------------------
vtkAnimationPlayer::vtkAnimationPlayer()
{
  this->AnimationScene = 0;
  this->InPlay = false;
  this->CurrentTime = 0;
  this->StopPlay = false;
  this->Loop = false;
}

//----------------------------------------------------------------------------
vtkAnimationPlayer::~vtkAnimationPlayer()
{
  this->SetAnimationScene(0);
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::Play()
{
  if (!this->AnimationScene)
    {
    vtkErrorMacro("No animation scene to play.");
    return;
    }

  if (this->InPlay)
    {
    vtkErrorMacro("Cannot play while playing.");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent);

  double starttime = this->AnimationScene->GetStartTime();
  double endtime = this->AnimationScene->GetEndTime();

  this->CurrentTime = this->AnimationScene->GetCurrentTime();
  this->CurrentTime = (this->CurrentTime < starttime || 
    this->CurrentTime >= endtime)? starttime : this->CurrentTime;
 
  this->InPlay = true;
  this->StopPlay = false;

  do 
    {
    this->StartLoop(starttime, endtime);
    this->AnimationScene->Initialize();
    double deltatime = 0.0;
    while (!this->StopPlay && this->CurrentTime <= endtime)
      {
      this->AnimationScene->Tick(this->CurrentTime, deltatime, this->CurrentTime);
      double progress = (this->CurrentTime-starttime)/(endtime-starttime);
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

      double nexttime = this->GetNextTime(this->CurrentTime);
      deltatime = nexttime - this->CurrentTime;
      this->CurrentTime = nexttime; 
      }

    // Finalize will get called when Tick() is called with time>=endtime on the
    // cue. However,no harm is calling this method again since it has any effect 
    // only the first time it gets called.
    // TODO: not sure what's the right thing here.
    // this->AnimationScene->Finalize();

    this->CurrentTime = starttime;
    this->EndLoop();
    // loop when this->Loop is true.
    } while (this->Loop && !this->StopPlay);

  this->InPlay = false;
  this->StopPlay = false;

  this->InvokeEvent(vtkCommand::EndEvent);
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
  if (this->AnimationScene)
    {
    this->AnimationScene->SetCurrentTime(
      this->AnimationScene->GetStartTime());
    }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToLast()
{
  if (this->AnimationScene)
    {
    this->AnimationScene->SetCurrentTime(
      this->AnimationScene->GetEndTime());
    }
}


//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToNext()
{
  double starttime = this->AnimationScene->GetStartTime();
  double endtime = this->AnimationScene->GetEndTime();
  double time = this->GoToNext(starttime, endtime, 
    this->AnimationScene->GetCurrentTime());

  if (time >= starttime && time < endtime)
    {
    this->AnimationScene->SetCurrentTime(time);
    }
  else 
    {
    this->AnimationScene->SetCurrentTime(endtime);
    }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::GoToPrevious()
{
  double starttime = this->AnimationScene->GetStartTime();
  double endtime = this->AnimationScene->GetEndTime();
  double time = this->GoToPrevious(starttime, endtime, 
    this->AnimationScene->GetCurrentTime());

  if (time >= starttime && time < endtime)
    {
    this->AnimationScene->SetCurrentTime(time);
    }
  else 
    {
    this->AnimationScene->SetCurrentTime(starttime);
    }
}

//----------------------------------------------------------------------------
void vtkAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


