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
#include "vtkSMAnimationSceneProxy.h"

#include "vtkCompositeAnimationPlayer.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMAnimationScene.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMTrace.h"
#include "vtkVector.h"

namespace
{
// This is a terrible way to address BUG #0015407. What's happening is that
// when state is being loaded the timekeeper gets updated timesteps which are
// then correctly propagated to the animation scene, which then, correctly (I
// might add) updates the start and end times. However all this happends
// before the "EndTime"/"StartTime" properties from the XML state are loaded.
// Hence they get set and loose the values just updated by the application,
// instead use the old values from the state files.
// This function will prune the EndTime/StartTime XML elements from the state
// being loaded if they should not be loaded.
void PruneEndTimesIfNeeded(vtkPVXMLElement* element, vtkSMAnimationScene* self)
{
  vtkPVXMLElement* startTimeElement = NULL;
  vtkPVXMLElement* endTimeElement = NULL;
  int playMode = self ? self->GetPlayMode() : vtkCompositeAnimationPlayer::SEQUENCE;
  int lockEndTime = self ? (self->GetLockEndTime() ? 1 : 0) : 0;
  int lockStartTime = self ? (self->GetLockStartTime() ? 1 : 0) : 0;
  for (unsigned int cc = 0, max = element->GetNumberOfNestedElements(); cc < max; cc++)
  {
    vtkPVXMLElement* cur = element->GetNestedElement(cc);
    if (cur && cur->GetName() && strcmp(cur->GetName(), "Property") == 0)
    {
      const char* name = cur->GetAttributeOrDefault("name", "");
      if (strcmp(name, "EndTime") == 0)
      {
        endTimeElement = cur;
      }
      else if (strcmp(name, "StartTime") == 0)
      {
        startTimeElement = cur;
      }
      else if (strcmp(name, "PlayMode") == 0)
      {
        if (vtkPVXMLElement* valueElem = cur->FindNestedElementByName("Element"))
        {
          valueElem->GetScalarAttribute("value", &playMode);
        }
      }
      else if (strcmp(name, "LockEndTime") == 0)
      {
        if (vtkPVXMLElement* valueElem = cur->FindNestedElementByName("Element"))
        {
          valueElem->GetScalarAttribute("value", &lockEndTime);
        }
      }
      else if (strcmp(name, "LockStartTime") == 0)
      {
        if (vtkPVXMLElement* valueElem = cur->FindNestedElementByName("Element"))
        {
          valueElem->GetScalarAttribute("value", &lockStartTime);
        }
      }
    }
  }
  if (playMode == vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS)
  {
    if (lockStartTime != 1 && startTimeElement)
    {
      element->RemoveNestedElement(startTimeElement);
    }
    if (lockEndTime != 1 && endTimeElement)
    {
      element->RemoveNestedElement(endTimeElement);
    }
  }
}
}

vtkStandardNewMacro(vtkSMAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  if (vtkObject* object = vtkObject::SafeDownCast(this->GetClientSideObject()))
  {
    object->AddObserver(vtkSMAnimationScene::UpdateStartEndTimesEvent, this,
      &vtkSMAnimationSceneProxy::OnUpdateStartEndTimesEvent);
  }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::OnUpdateStartEndTimesEvent(
  vtkObject* object, unsigned long, void* calldata)
{
  const vtkVector2d& range = *(reinterpret_cast<vtkVector2d*>(calldata));
  vtkSMAnimationScene* caller = vtkSMAnimationScene::SafeDownCast(object);
  assert(caller == this->GetClientSideObject());

  vtkSMPropertyHelper startTime(this, "StartTime");
  vtkSMPropertyHelper endTime(this, "EndTime");
  vtkVector2d newRange(startTime.GetAsDouble(0), endTime.GetAsDouble(0));
  const auto current_delta = (newRange[1] - newRange[0]);
  if (!caller->GetLockStartTime())
  {
    newRange.SetX(range.GetX());
  }
  if (!caller->GetLockEndTime())
  {
    newRange.SetY(range.GetY());
  }

  // if range[0] == range[1], which can happen when there's only 1 timestep in
  // the dataset, we push back end time to avoid having an animation with no
  // range.
  if (newRange[0] == newRange[1])
  {
    newRange[1] += current_delta > 0 ? current_delta : 1.0;
  }
  startTime.Set(newRange.GetX());
  endTime.Set(newRange.GetY());
  // XXX: What to do if the start or end time was locked and the suggested start
  // or end time would make the range invalid?
  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps()
{
  vtkSMProxy* timeKeeper = vtkSMPropertyHelper(this, "TimeKeeper").GetAsProxy();
  if (!timeKeeper)
  {
    vtkWarningMacro("Failed to locate TimeKeeper proxy.");
    return false;
  }

  SM_SCOPED_TRACE(CallMethodIfPropertiesModified)
    .arg("proxy", this)
    .arg("methodname", "UpdateAnimationUsingDataTimeSteps")
    .arg("comment", "update animation scene based on data timesteps");

  // ensure that the timekeeper has up-to-date time information (BUG #17849).
  vtkSMTimeKeeperProxy::UpdateTimeInformation(timeKeeper);

  bool using_snap_to_timesteps_mode = false;
  vtkSMPropertyHelper timestepsHelper(timeKeeper, "TimestepValues");
  if (timestepsHelper.GetNumberOfElements() > 1)
  {
    vtkSMPropertyHelper(this, "PlayMode").Set(vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS);
    using_snap_to_timesteps_mode = true;
  }
  else
  {
    vtkSMPropertyHelper playmodeHelper(this, "PlayMode");
    if (playmodeHelper.GetAsInt() == vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS)
    {
      playmodeHelper.Set(vtkCompositeAnimationPlayer::SEQUENCE);
    }
  }

  this->UpdateVTKObjects();
  // This will internally adjust the Start and End times for the animation scene
  // based of the Locks for the times. We not simply need to copy the info
  // property values.

  /// If the animation time is not in the scene time range, set it to the min
  /// value.
  double minTime = vtkSMPropertyHelper(this, "StartTime").GetAsDouble();
  double maxTime = vtkSMPropertyHelper(this, "EndTime").GetAsDouble();
  double animationTime = vtkSMPropertyHelper(this, "AnimationTime").GetAsDouble();
  if (animationTime < minTime || animationTime > maxTime)
  {
    vtkSMPropertyHelper(this, "AnimationTime").Set(minTime);
  }
  else if (using_snap_to_timesteps_mode)
  {
    // BUG #15060. When using SNAP_TO_TIMESTEPS mode, ensure that the timestep
    // is "snapped".
    vtkSMPropertyHelper(this, "AnimationTime")
      .Set(vtkSMTimeKeeperProxy::GetLowerBoundTimeStep(timeKeeper, animationTime));
  }
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMAnimationSceneProxy::FindAnimationCue(
  vtkSMProxy* animatedProxy, const char* animatedPropertyName)
{
  if (!animatedProxy || !animatedPropertyName)
  {
    return NULL;
  }

  vtkSMPropertyHelper cuesHelper(this, "Cues");
  for (unsigned int cc = 0, max = cuesHelper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMProxy* cueProxy = cuesHelper.GetAsProxy(cc);
    if (cueProxy)
    {
      vtkSMPropertyHelper proxyHelper(cueProxy, "AnimatedProxy");
      if (proxyHelper.GetAsProxy() == animatedProxy)
      {
        vtkSMPropertyHelper pnameHelper(cueProxy, "AnimatedPropertyName");
        if (pnameHelper.GetAsString() &&
          strcmp(animatedPropertyName, pnameHelper.GetAsString()) == 0)
        {
          return cueProxy;
        }
      }
    }
  }

  // Check if we're animating a helper proxy on the proxy.
  std::string groupname = vtkSMParaViewPipelineController::GetHelperProxyGroupName(animatedProxy);

  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(animatedProxy->GetSessionProxyManager());
  iter->SetModeToOneGroup();
  for (iter->Begin(groupname.c_str()); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProxy* cue = this->FindAnimationCue(iter->GetProxy(), animatedPropertyName);
    if (cue)
    {
      return cue;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator)
{
  PruneEndTimesIfNeeded(element,
    (this->ObjectsCreated ? vtkSMAnimationScene::SafeDownCast(this->GetClientSideObject()) : NULL));
  return this->Superclass::LoadXMLState(element, locator);
  ;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
