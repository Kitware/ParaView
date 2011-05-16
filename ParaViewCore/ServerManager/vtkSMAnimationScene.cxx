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

#include "vtkCacheSizeKeeper.h"
#include "vtkCompositeAnimationPlayer.h"
#include "vtkEventForwarderCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
class vtkSMAnimationScene::vtkInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkSMViewProxy> > VectorOfViews;
  VectorOfViews ViewModules;
  void StillRenderAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
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

vtkStandardNewMacro(vtkSMAnimationScene);
//----------------------------------------------------------------------------
vtkSMAnimationScene::vtkSMAnimationScene()
{
  this->Caching = false;
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
  this->AnimationPlayer->AddObserver(
    vtkCommand::StartEvent, this->Forwarder);
  this->AnimationPlayer->AddObserver(
    vtkCommand::EndEvent, this->Forwarder);
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
void vtkSMAnimationScene::SetTimeKeeper(vtkSMProxy* tkp)
{
  if (this->TimeKeeper == tkp)
    {
    return;
    }

  if (this->TimeKeeper && this->TimeRangeObserverID)
    {
    this->TimeKeeper->GetProperty("TimeRange")->RemoveObserver(
      this->TimeRangeObserverID);
    }
  if (this->TimeKeeper && this->TimestepValuesObserverID)
    {
    this->TimeKeeper->GetProperty("TimestepValues")->RemoveObserver(
      this->TimestepValuesObserverID);
    }
  this->TimeRangeObserverID = 0;
  this->TimestepValuesObserverID = 0;
  vtkSetObjectBodyMacro(TimeKeeper, vtkSMProxy, tkp);
  if (this->TimeKeeper)
    {
    this->TimeRangeObserverID =
      this->TimeKeeper->GetProperty("TimeRange")->AddObserver(
        vtkCommand::ModifiedEvent, this,
        &vtkSMAnimationScene::TimeKeeperTimeRangeChanged);
    this->TimestepValuesObserverID =
      this->TimeKeeper->GetProperty("TimestepValues")->AddObserver(
        vtkCommand::ModifiedEvent, this,
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
  for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
    {
    this->AnimationPlayer->AddTimeStep(helper.GetAsDouble(cc));
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::TimeKeeperTimeRangeChanged()
{
  // If time keeper has a non-trivial time range and the times are not locked,
  // then we change the times to match the time range.
  double min = vtkSMPropertyHelper(this->TimeKeeper,"TimeRange").GetAsDouble(0);
  double max = vtkSMPropertyHelper(this->TimeKeeper,"TimeRange").GetAsDouble(1);
  if (max > min)
    {
    if (!this->LockStartTime)
      {
      this->SetStartTime(min);
      }
    if (!this->LockEndTime)
      {
      this->SetEndTime(max);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::AddViewProxy(vtkSMViewProxy* view)
{
  vtkInternals::VectorOfViews::iterator iter =
    this->Internals->ViewModules.begin();
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
void vtkSMAnimationScene::RemoveViewProxy(
  vtkSMViewProxy* view)
{
  vtkInternals::VectorOfViews::iterator iter =
    this->Internals->ViewModules.begin();
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
void vtkSMAnimationScene::TickInternal(
  double currenttime, double deltatime, double clocktime)
{
  // We see that here we don't check if the cache is full at all. Views have
  // logic in them to periodically check and synchronize the "fullness" of cache
  // among all participating processes. So we don't have to manage that here at
  // all.
  if (this->Caching)
    {
    this->Internals->PassUseCache(true);
    this->Internals->PassCacheTime(currenttime);
    }
  this->Superclass::TickInternal(currenttime, deltatime, clocktime);
  if (!this->OverrideStillRender)
    {
    this->Internals->StillRenderAllViews();
    }
  if (this->Caching)
    {
    this->Internals->PassUseCache(false);
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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

//----------------------------------------------------------------------------
void vtkSMAnimationScene::SetCacheLimit(unsigned long kbs)
{
  // Note since vtkSMAnimationScene is created on the client alone this will
  // affect only the client-side setting. To get around this problem, we create
  // a special "GlobalAnimationProperties" that we use to change this setting on
  // all processes.
  vtkCacheSizeKeeper::GetInstance()->SetCacheLimit(kbs);
}
