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

#include "vtkAnimationScene.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMAnimationSceneProxy.h"

//-----------------------------------------------------------------------------
class vtkSMAnimationSceneWriterObserver : public vtkCommand
{
public:
  static vtkSMAnimationSceneWriterObserver* New() 
    {
    return new vtkSMAnimationSceneWriterObserver;
    }

  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventId, callData);
      }
    }

  void SetTarget(vtkSMAnimationSceneWriter* target)
    {
    this->Target = target;
    }

protected:
  vtkSMAnimationSceneWriterObserver()
    {
    this->Target = 0;
    }
  vtkSMAnimationSceneWriter* Target;
};

vtkCxxRevisionMacro(vtkSMAnimationSceneWriter, "1.4");
//-----------------------------------------------------------------------------
vtkSMAnimationSceneWriter::vtkSMAnimationSceneWriter()
{
  this->AnimationScene = 0;
  this->Saving = false;
  this->Observer = vtkSMAnimationSceneWriterObserver::New();
  this->Observer->SetTarget(this);
  this->FileName = 0;
  this->SaveFailed = false;
  this->FrameRate = 0.0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneWriter::~vtkSMAnimationSceneWriter()
{
  this->SetAnimationScene(0);
  this->Observer->SetTarget(0);
  this->Observer->Delete();
  this->SetFileName(0);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::SetAnimationScene(vtkSMAnimationSceneProxy* scene)
{
  if (this->AnimationScene)
    {
    this->AnimationScene->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(AnimationScene, vtkSMAnimationSceneProxy, scene);

  if (this->AnimationScene)
    {
    //this->AnimationScene->AddObserver(vtkCommand::StartAnimationCueEvent,
    //  this->Observer);
    this->AnimationScene->AddObserver(vtkCommand::AnimationCueTickEvent,
      this->Observer);
    //this->AnimationScene->AddObserver(vtkCommand::EndAnimationCueEvent,
    //  this->Observer);
    }
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::ExecuteEvent(vtkObject* vtkNotUsed(caller),
  unsigned long eventid, void* calldata)
{
  if (!this->Saving)
    {
    // ignore all events if we aren't currently saving the animation.
    return;
    }

  if (eventid == vtkCommand::AnimationCueTickEvent)
    {
    vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
      vtkAnimationCue::AnimationCueInfo*>(calldata);
    if (!this->SaveFrame(cueInfo->AnimationTime))
      {
      // Save failed, abort.
      this->AnimationScene->Stop();
      this->SaveFailed = true;
      }
    }
}

#include "vtkTimerLog.h"
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
  double start_time =  this->AnimationScene->GetStartTime();
  this->AnimationScene->SetAnimationTime(start_time);

  int play_mode = this->AnimationScene->GetPlayMode();

  // If play mode is real time, we switch it to sequence.
  // We are assuming that the frame rate has been set correctly
  // on the scene, even in real time mode.
  if (play_mode == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    this->AnimationScene->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
    }

  // Disable looping.
  int loop = this->AnimationScene->GetLoop();
  this->AnimationScene->SetLoop(0);

  bool status = this->SaveInitialize();
  double frame_rate = this->AnimationScene->GetFrameRate();
  if (this->FrameRate > 0)
    {
    this->AnimationScene->SetFrameRate(this->FrameRate);
    }

  int caching = this->AnimationScene->GetCaching();
  this->AnimationScene->SetCaching(0);

  if (status)
    {
    vtkTimerLog* timer = vtkTimerLog::New();
    timer->StartTimer();
    this->Saving = true;
    this->SaveFailed = false;
    this->AnimationScene->Play();
    this->Saving = false;
    timer->StopTimer();

    cout << "Play Time: " << timer->GetElapsedTime() << " seconds" << endl;
    timer->Delete();
    }

  status = this->SaveFinalize() && status;

  // Restore scene parameters, if changed.
  if (play_mode == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    this->AnimationScene->SetPlayMode(play_mode);
    }
  this->AnimationScene->SetLoop(loop);
  this->AnimationScene->SetFrameRate(frame_rate);
  this->AnimationScene->SetCaching(caching);

  return status && (!this->SaveFailed);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AnimationScene: " << this->AnimationScene << endl;
  os << indent << "FileName: " << 
    (this->FileName? this->FileName : "(null)") << endl;
  os << indent << "FrameRate: " << this->FrameRate << endl;
}
