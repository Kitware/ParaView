/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneWriter.h"

#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

#include <algorithm>

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWriter::vtkSMAnimationSceneWriter()
{
  this->AnimationScene = 0;
  this->Saving = false;
  this->ObserverID = 0;
  this->FileName = 0;
  this->SaveFailed = false;
  this->StartFileCount = 0;

  this->PlaybackTimeWindow[0] = 1.0;
  this->PlaybackTimeWindow[1] = -1.0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWriter::~vtkSMAnimationSceneWriter()
{
  this->SetAnimationScene((vtkSMAnimationScene*)NULL);
  this->SetFileName(0);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetAnimationScene(vtkSMProxy* proxy)
{
  // Store the corresponding session
  this->SetSession(proxy->GetSession());

  this->SetAnimationScene(
    proxy ? vtkSMAnimationScene::SafeDownCast(proxy->GetClientSideObject()) : NULL);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetAnimationScene(vtkSMAnimationScene* scene)
{
  if (this->AnimationScene && this->ObserverID)
  {
    this->AnimationScene->RemoveObserver(this->ObserverID);
  }

  vtkSetObjectBodyMacro(AnimationScene, vtkSMAnimationScene, scene);

  if (this->AnimationScene)
  {
    this->ObserverID = this->AnimationScene->AddObserver(
      vtkCommand::AnimationCueTickEvent, this, &vtkSMAnimationSceneWriter::ExecuteEvent);
  }
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::ExecuteEvent(
  vtkObject* vtkNotUsed(caller), unsigned long eventid, void* calldata)
{
  if (!this->Saving)
  {
    // ignore all events if we aren't currently saving the animation.
    return;
  }

  if (eventid == vtkCommand::AnimationCueTickEvent)
  {
    const vtkAnimationCue::AnimationCueInfo* cueInfo =
      reinterpret_cast<vtkAnimationCue::AnimationCueInfo*>(calldata);
    if (!this->SaveFrame(cueInfo->AnimationTime))
    {
      // Save failed, abort.
      this->AnimationScene->Stop();
      this->SaveFailed = true;

      double progress = 1.0;
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    }
    else
    {
      // invoke progress event.
      // use playback window, if valid, else  use the animation time to convert
      // current time to progress in range [0, 1].
      double window[2];
      this->GetPlaybackTimeWindow(window);
      if (window[0] <= window[1])
      {
        window[0] = cueInfo->StartTime;
        window[1] = cueInfo->EndTime;
      }

      double progress = (cueInfo->AnimationTime - window[0]) / (window[1] - window[0]);
      progress = std::max(0.0, progress);
      progress = std::min(progress, 1.0);
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneWriter::Save()
{
  if (this->Saving)
  {
    vtkErrorMacro("Already saving an animation. "
      << "Wait till that is done before calling Save again.");
    return false;
  }

  if (!this->AnimationScene)
  {
    vtkErrorMacro("Cannot save, no AnimationScene.");
    return false;
  }

  if (!this->FileName)
  {
    vtkErrorMacro("FileName not set.");
    return false;
  }

  // Take the animation scene to the beginning.
  this->AnimationScene->GoToFirst();

  /*
  int play_mode = this->AnimationScene->GetPlayMode();

  // If play mode is real time, we switch it to sequence.
  // We are assuming that the frame rate has been set correctly
  // on the scene, even in real time mode.
  if (play_mode == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
    }
    */

  // Disable looping.
  int loop = this->AnimationScene->GetLoop();
  this->AnimationScene->SetLoop(0);

  bool status = this->SaveInitialize(this->StartFileCount);
  bool cachingFlag = this->AnimationScene->GetForceDisableCaching();
  this->AnimationScene->SetForceDisableCaching(true);

  if (status)
  {
    this->Saving = true;
    this->SaveFailed = false;

    this->AnimationScene->SetPlaybackTimeWindow(this->GetPlaybackTimeWindow());
    this->AnimationScene->Play();
    this->AnimationScene->SetPlaybackTimeWindow(1.0, -1.0); // Reset to full range
    this->Saving = false;
  }

  status = this->SaveFinalize() && status;

  /*
  if (play_mode == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    this->AnimationScene->SetPlayMode(play_mode);
    }
    */

  // Restore scene parameters, if changed.
  this->AnimationScene->SetLoop(loop);
  this->AnimationScene->SetForceDisableCaching(cachingFlag);

  return status && (!this->SaveFailed);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimationScene: " << this->AnimationScene << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << endl;
}
