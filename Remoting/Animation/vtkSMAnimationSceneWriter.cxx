// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMAnimationSceneWriter.h"

#include "vtkAnimationCue.h"
#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMProxy.h"

#include <algorithm>

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWriter::vtkSMAnimationSceneWriter()
{
  this->AnimationScene = nullptr;
  this->Saving = false;
  this->ObserverID = 0;
  this->FileName = nullptr;
  this->SaveFailed = false;
  this->StartFileCount = 0;

  this->PlaybackTimeWindow[0] = 1.0;
  this->PlaybackTimeWindow[1] = -1.0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWriter::~vtkSMAnimationSceneWriter()
{
  this->SetAnimationScene((vtkSMAnimationScene*)nullptr);
  this->SetFileName(nullptr);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetAnimationScene(vtkSMProxy* proxy)
{
  // Store the corresponding session
  this->SetSession(proxy->GetSession());

  this->SetAnimationScene(
    proxy ? vtkSMAnimationScene::SafeDownCast(proxy->GetClientSideObject()) : nullptr);
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
      if (window[0] > window[1])
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
  vtkLogScopeF(TRACE, "Save animation scene.");
  if (this->Saving)
  {
    vtkErrorMacro("Already saving an animation. "
      << "Wait till that is done before calling Save again.");
    return false;
  }

  if (!this->FileName)
  {
    vtkErrorMacro("FileName not set.");
    return false;
  }

  if (!this->AnimationScene || !this->WriteTimeSteps)
  {
    vtkLogScopeF(TRACE, "Save only current scene time. Reason:");
    vtkLogIf(TRACE, !this->AnimationScene, "No animation scene set.");
    vtkLogIf(TRACE, !this->WriteTimeSteps, "Only current time requested.");

    bool result = this->SaveInitialize(0);
    result =
      result && this->SaveFrame(this->AnimationScene ? this->AnimationScene->GetSceneTime() : 0.);
    result = result && this->SaveFinalize();
    return result;
  }

  vtkLogF(TRACE, "Prepare animation scene.");
  // Take the animation scene to the beginning.
  this->AnimationScene->GoToFirst();

  // Disable looping.
  int loop = this->AnimationScene->GetLoop();
  this->AnimationScene->SetLoop(0);

  bool status = this->SaveInitialize(this->StartFileCount);
  if (status)
  {
    this->Saving = true;
    this->SaveFailed = false;
    double* currentTimeWindow = this->AnimationScene->GetPlaybackTimeWindow();
    int currentStride = this->AnimationScene->GetStride();
    this->AnimationScene->SetPlaybackTimeWindow(this->GetPlaybackTimeWindow());
    this->AnimationScene->SetStride(this->GetStride());
    vtkLogF(TRACE, "Play animation scene.");
    this->AnimationScene->Play();
    this->AnimationScene->SetPlaybackTimeWindow(currentTimeWindow); // Restore previous range
    this->AnimationScene->SetStride(currentStride);                 // Restore previous stride
    this->Saving = false;
  }

  vtkLogF(TRACE, "Restore animation scene.");
  status = this->SaveFinalize() && status;

  // Restore scene parameters, if changed.
  this->AnimationScene->SetLoop(loop);

  return status && (!this->SaveFailed);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetFrameWindow(int window[2])
{
  if (!this->AnimationScene)
  {
    return;
  }
  double timeWindow[2];
  this->AnimationScene->SanitizeFrameWindow(window, timeWindow);
  this->SetPlaybackTimeWindow(timeWindow);
  this->SetStartFileCount(window[0]);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetFrameWindow(int min, int max)
{
  int window[2] = { min, max };
  this->SetFrameWindow(window);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimationScene: " << this->AnimationScene << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)") << endl;
  os << indent << "FrameExporterDelegate: " << this->FrameExporterDelegate << endl;
}

//-----------------------------------------------------------------------------
vtkSMExporterProxy* vtkSMAnimationSceneWriter::GetFrameExporterDelegate()
{
  return this->FrameExporterDelegate;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetFrameExporterDelegate(vtkSMExporterProxy* exporter)
{
  this->FrameExporterDelegate = exporter;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneWriter::AnimationEnabled()
{
  return this->GetWriteTimeSteps() && this->GetAnimationScene();
}
