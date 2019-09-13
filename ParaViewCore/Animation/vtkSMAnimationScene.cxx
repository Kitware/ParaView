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
#include "vtkSMAnimationScene.h"

#include "vtkCompositeAnimationPlayer.h"
#include "vtkEventForwarderCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCameraAnimationCue.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"

#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
#include "vtkPythonAnimationCue.h"
#endif

#include <algorithm>
#include <cassert>
#include <vector>

//----------------------------------------------------------------------------
class vtkSMAnimationScene::vtkInternals
{
  vtkNew<vtkSMTransferFunctionManager> TransferFunctionManager;

public:
  typedef std::vector<vtkSmartPointer<vtkAnimationCue> > VectorOfAnimationCues;
  VectorOfAnimationCues AnimationCues;

  typedef std::vector<vtkSmartPointer<vtkSMViewProxy> > VectorOfViews;
  VectorOfViews ViewModules;

  void UpdateAllViews()
  {
    if (this->ViewModules.size() == 0)
    {
      return;
    }

    vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "update-all-views for animation");

    vtkSMSessionProxyManager* pxm = NULL;
    for (VectorOfViews::iterator iter = this->ViewModules.begin(); iter != this->ViewModules.end();
         ++iter)
    {
      // save the proxy manager for later.
      if (pxm)
      {
        assert(iter->GetPointer()->GetSessionProxyManager() == pxm);
      }
      else
      {
        pxm = iter->GetPointer()->GetSessionProxyManager();
      }
      iter->GetPointer()->Update();
    }

    vtkVLogStartScope(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "reset transfer functions");
    this->TransferFunctionManager->ResetAllTransferFunctionRangesUsingCurrentData(
      pxm, true /*animating*/);
    vtkLogEndScope("reset transfer functions");
  }

  void StillRenderAllViews()
  {
    vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "still-render-all-views for animation");
    for (VectorOfViews::iterator iter = this->ViewModules.begin(); iter != this->ViewModules.end();
         ++iter)
    {
      iter->GetPointer()->StillRender();
    }
  }

  void PassCacheTime(double cachetime)
  {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
    {
      vtkSMPropertyHelper((*iter), "CacheKey").Set(cachetime);
      iter->GetPointer()->UpdateProperty("CacheKey");
    }
  }

  void PassUseCache(bool usecache)
  {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
    {
      vtkSMPropertyHelper((*iter), "UseCache").Set(usecache);
      iter->GetPointer()->UpdateProperty("UseCache");
    }
  }
};

namespace
{
// Helper class used by for_each() to call Tick on all cues if they are not
// one of the "exception" classes.
class vtkTickOnGenericCue
{
  double StartTime;
  double EndTime;
  double CurrentTime;
  double DeltaTime;
  double ClockTime;

protected:
  virtual bool IsAcceptable(vtkAnimationCue* cue) const
  {
    return (cue &&
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
      (vtkPythonAnimationCue::SafeDownCast(cue) == NULL) &&
#endif
      (vtkPVCameraAnimationCue::SafeDownCast(cue) == NULL));
  }

public:
  vtkTickOnGenericCue(
    double starttime, double endtime, double currenttime, double deltatime, double clocktime)
    : StartTime(starttime)
    , EndTime(endtime)
    , CurrentTime(currenttime)
    , DeltaTime(deltatime)
    , ClockTime(clocktime)
  {
  }
  virtual ~vtkTickOnGenericCue() {}
  virtual void operator()(vtkAnimationCue* cue) const
  {
    if (!this->IsAcceptable(cue))
    {
      return;
    }

    switch (cue->GetTimeMode())
    {
      case vtkAnimationCue::TIMEMODE_RELATIVE:
        cue->Tick(this->CurrentTime - this->StartTime, this->DeltaTime, this->ClockTime);
        break;
      case vtkAnimationCue::TIMEMODE_NORMALIZED:
        cue->Tick((this->CurrentTime - this->StartTime) / (this->EndTime - this->StartTime),
          this->DeltaTime / (this->EndTime - this->StartTime), this->ClockTime);
        break;
      default:
        vtkGenericWarningMacro("Invalid cue time mode");
    }
  }
};

class vtkTickOnCameraCue : public vtkTickOnGenericCue
{
protected:
  bool IsAcceptable(vtkAnimationCue* cue) const override
  {
    return (vtkPVCameraAnimationCue::SafeDownCast(cue) != NULL);
  }

  vtkSMProxy* TimeKeeper;

public:
  vtkTickOnCameraCue(double starttime, double endtime, double currenttime, double deltatime,
    double clocktime, vtkSMProxy* timeKeeper)
    : vtkTickOnGenericCue(starttime, endtime, currenttime, deltatime, clocktime)
    , TimeKeeper(timeKeeper)
  {
  }

  void operator()(vtkAnimationCue* cue) const override
  {
    vtkPVCameraAnimationCue* cameraCue = vtkPVCameraAnimationCue::SafeDownCast(cue);
    if (cameraCue)
    {
      cameraCue->SetTimeKeeper(this->TimeKeeper);
    }
    vtkTickOnGenericCue::operator()(cue);
    if (cameraCue)
    {
      cameraCue->SetTimeKeeper(nullptr);
    }
  }
};

class vtkTickOnPythonCue : public vtkTickOnGenericCue
{
protected:
  bool IsAcceptable(vtkAnimationCue* cue) const override
  {
    (void)cue;
    return (false
#if VTK_MODULE_ENABLE_VTK_PythonInterpreter
      || (vtkPythonAnimationCue::SafeDownCast(cue) != NULL)
#endif
        );
  }

public:
  vtkTickOnPythonCue(
    double starttime, double endtime, double currenttime, double deltatime, double clocktime)
    : vtkTickOnGenericCue(starttime, endtime, currenttime, deltatime, clocktime)
  {
  }
};
}

vtkStandardNewMacro(vtkSMAnimationScene);
//----------------------------------------------------------------------------
vtkSMAnimationScene::vtkSMAnimationScene()
{
  this->SceneTime = 0;
  this->PlaybackTimeWindow[0] = 1.0;
  this->PlaybackTimeWindow[1] = -1.0;
  this->ForceDisableCaching = false;
  this->InTick = false;
  this->LockEndTime = false;
  this->LockStartTime = false;
  this->OverrideStillRender = false;
  this->TimeKeeper = NULL;
  this->TimeRangeObserverID = 0;
  this->TimestepValuesObserverID = 0;
  this->AnimationPlayer = vtkCompositeAnimationPlayer::New();
  // vtkAnimationPlayer::SetAnimationScene() is not reference counted.
  this->AnimationPlayer->SetAnimationScene(this);
  this->Internals = new vtkInternals();

  this->Forwarder = vtkEventForwarderCommand::New();
  this->Forwarder->SetTarget(this);
  this->AnimationPlayer->AddObserver(vtkCommand::StartEvent, this->Forwarder);
  this->AnimationPlayer->AddObserver(vtkCommand::EndEvent, this->Forwarder);
}

//----------------------------------------------------------------------------
vtkSMAnimationScene::~vtkSMAnimationScene()
{
  this->SetTimeKeeper(NULL);

  this->AnimationPlayer->RemoveObserver(this->Forwarder);
  this->AnimationPlayer->Delete();
  this->AnimationPlayer = NULL;

  this->Forwarder->SetTarget(NULL);
  this->Forwarder->Delete();

  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::AddCue(vtkAnimationCue* cue)
{
  vtkInternals::VectorOfAnimationCues& cues = this->Internals->AnimationCues;
  if (std::find(cues.begin(), cues.end(), cue) != cues.end())
  {
    vtkErrorMacro("Animation cue already present in the scene");
    return;
  }
  cues.push_back(cue);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::RemoveCue(vtkAnimationCue* cue)
{
  vtkInternals::VectorOfAnimationCues& cues = this->Internals->AnimationCues;
  vtkInternals::VectorOfAnimationCues::iterator iter = std::find(cues.begin(), cues.end(), cue);
  if (iter != cues.end())
  {
    cues.erase(iter);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::RemoveAllCues()
{
  if (this->Internals->AnimationCues.size() > 0)
  {
    this->Internals->AnimationCues.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkSMAnimationScene::GetNumberOfCues()
{
  return static_cast<int>(this->Internals->AnimationCues.size());
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetTimeKeeper(vtkSMProxy* tkp)
{
  if (this->TimeKeeper == tkp)
  {
    return;
  }

  if (this->TimeKeeper && this->TimeRangeObserverID)
  {
    this->TimeKeeper->GetProperty("TimeRange")->RemoveObserver(this->TimeRangeObserverID);
  }
  if (this->TimeKeeper && this->TimestepValuesObserverID)
  {
    this->TimeKeeper->GetProperty("TimestepValues")->RemoveObserver(this->TimestepValuesObserverID);
  }
  this->TimeRangeObserverID = 0;
  this->TimestepValuesObserverID = 0;
  vtkSetObjectBodyMacro(TimeKeeper, vtkSMProxy, tkp);
  if (this->TimeKeeper)
  {
    this->TimeRangeObserverID = this->TimeKeeper->GetProperty("TimeRange")
                                  ->AddObserver(vtkCommand::ModifiedEvent, this,
                                    &vtkSMAnimationScene::TimeKeeperTimeRangeChanged);
    this->TimestepValuesObserverID = this->TimeKeeper->GetProperty("TimestepValues")
                                       ->AddObserver(vtkCommand::ModifiedEvent, this,
                                         &vtkSMAnimationScene::TimeKeeperTimestepsChanged);
    this->TimeKeeperTimestepsChanged();
    this->TimeKeeperTimeRangeChanged();
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::TimeKeeperTimestepsChanged()
{
  this->AnimationPlayer->RemoveAllTimeSteps();
  vtkSMPropertyHelper helper(this->TimeKeeper, "TimestepValues");
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
  {
    this->AnimationPlayer->AddTimeStep(helper.GetAsDouble(cc));
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::TimeKeeperTimeRangeChanged()
{
  // If time keeper has a non-trivial time range and the times are not locked,
  // then we change the times to match the time range.
  vtkVector2d range(vtkSMPropertyHelper(this->TimeKeeper, "TimeRange").GetAsDouble(0),
    vtkSMPropertyHelper(this->TimeKeeper, "TimeRange").GetAsDouble(1));
  if (range[1] > range[0])
  {
    this->InvokeEvent(vtkSMAnimationScene::UpdateStartEndTimesEvent, &range);
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::AddViewProxy(vtkSMViewProxy* view)
{
  vtkInternals::VectorOfViews::iterator iter = this->Internals->ViewModules.begin();
  for (; iter != this->Internals->ViewModules.end(); ++iter)
  {
    if (iter->GetPointer() == view)
    {
      // already added.
      return;
    }
  }
  this->Internals->ViewModules.push_back(view);
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::RemoveViewProxy(vtkSMViewProxy* view)
{
  vtkInternals::VectorOfViews::iterator iter = this->Internals->ViewModules.begin();
  for (; iter != this->Internals->ViewModules.end(); ++iter)
  {
    if (iter->GetPointer() == view)
    {
      this->Internals->ViewModules.erase(iter);
      break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::RemoveAllViewProxies()
{
  this->Internals->ViewModules.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkSMAnimationScene::GetNumberOfViewProxies()
{
  return static_cast<unsigned int>(this->Internals->ViewModules.size());
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMAnimationScene::GetViewProxy(unsigned int cc)
{
  if (cc < this->GetNumberOfViewProxies())
  {
    return this->Internals->ViewModules[cc];
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::StartCueInternal()
{
  this->Superclass::StartCueInternal();

  // Initialize all the animation cues.
  vtkInternals::VectorOfAnimationCues& cues = this->Internals->AnimationCues;
  for (vtkInternals::VectorOfAnimationCues::iterator iter = cues.begin(); iter != cues.end();
       ++iter)
  {
    iter->GetPointer()->Initialize();
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::EndCueInternal()
{
  // finalize all the animation cues.
  vtkInternals::VectorOfAnimationCues& cues = this->Internals->AnimationCues;
  for (vtkInternals::VectorOfAnimationCues::iterator iter = cues.begin(); iter != cues.end();
       ++iter)
  {
    iter->GetPointer()->Finalize();
  }

  this->Superclass::EndCueInternal();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::TickInternal(double currenttime, double deltatime, double clocktime)
{
  assert(!this->InTick);

  // We see that here we don't check if the cache is full at all. Views have
  // logic in them to periodically check and synchronize the "fullness" of cache
  // among all participating processes. So we don't have to manage that here at
  // all.
  bool caching_enabled = (!this->ForceDisableCaching) &&
    vtkPVGeneralSettings::GetInstance()->GetCacheGeometryForAnimation();
  if (caching_enabled)
  {
    this->Internals->PassUseCache(true);
    this->Internals->PassCacheTime(currenttime);
  }

  // this ensures that if this->SetSceneTime() is called, we don't call Tick()
  // again.
  this->InTick = true;

  this->SceneTime = currenttime;

  vtkInternals::VectorOfAnimationCues& cues = this->Internals->AnimationCues;
  // Now the animation update loop is as follows:
  //    - Update all cues not explicitly listed here
  //    - Update all Python cues
  // UpdateAllViews()
  //    - Update all Camera cues
  // Superclass::TickInternal
  // RenderAllViews()

  std::for_each(cues.begin(), cues.end(),
    vtkTickOnGenericCue(this->StartTime, this->EndTime, currenttime, deltatime, clocktime));

  std::for_each(cues.begin(), cues.end(),
    vtkTickOnPythonCue(this->StartTime, this->EndTime, currenttime, deltatime, clocktime));

  this->Internals->UpdateAllViews();

  std::for_each(cues.begin(), cues.end(), vtkTickOnCameraCue(this->StartTime, this->EndTime,
                                            currenttime, deltatime, clocktime, this->TimeKeeper));

  this->Superclass::TickInternal(currenttime, deltatime, clocktime);

  if (!this->OverrideStillRender)
  {
    this->Internals->StillRenderAllViews();
  }
  this->InTick = false;

  if (caching_enabled)
  {
    this->Internals->PassUseCache(false);
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ForceDisableCaching: " << this->ForceDisableCaching << endl;
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetLoop(int val)
{
  this->AnimationPlayer->SetLoop(val != 0);
}

//----------------------------------------------------------------------------
int vtkSMAnimationScene::GetLoop()
{
  return this->AnimationPlayer->GetLoop();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::Play()
{
  this->AnimationPlayer->Play();
}
//----------------------------------------------------------------------------
void vtkSMAnimationScene::Stop()
{
  this->AnimationPlayer->Stop();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::GoToNext()
{
  static_cast<vtkAnimationPlayer*>(this->AnimationPlayer)->GoToNext();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::GoToPrevious()
{
  static_cast<vtkAnimationPlayer*>(this->AnimationPlayer)->GoToPrevious();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::GoToFirst()
{
  this->AnimationPlayer->GoToFirst();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::GoToLast()
{
  this->AnimationPlayer->GoToLast();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetPlayMode(int val)
{
  this->AnimationPlayer->SetPlayMode(val);
}

//----------------------------------------------------------------------------
int vtkSMAnimationScene::GetPlayMode()
{
  return this->AnimationPlayer->GetPlayMode();
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetNumberOfFrames(int val)
{
  this->AnimationPlayer->SetNumberOfFrames(val);
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetDuration(int val)
{
  this->AnimationPlayer->SetDuration(val);
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetFramesPerTimestep(int val)
{
  this->AnimationPlayer->SetFramesPerTimestep(val);
}
