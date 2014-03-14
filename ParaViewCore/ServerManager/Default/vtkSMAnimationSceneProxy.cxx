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

#include "vtkCommand.h"
#include "vtkCompositeAnimationPlayer.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"

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
    object->AddObserver(vtkCommand::ModifiedEvent,
      this, &vtkSMAnimationSceneProxy::UpdatePropertyInformation);
    }
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

  vtkSMPropertyHelper timestepsHelper(timeKeeper, "TimestepValues");
  if (timestepsHelper.GetNumberOfElements() > 1)
    {
    vtkSMPropertyHelper(this, "PlayMode").Set(vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS);
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
  double minTime = vtkSMPropertyHelper(this, "StartTimeInfo").GetAsDouble();
  double maxTime = vtkSMPropertyHelper(this, "EndTimeInfo").GetAsDouble();
  double animationTime = vtkSMPropertyHelper(this, "AnimationTime").GetAsDouble();

  vtkSMPropertyHelper(this, "StartTime").Set(minTime);
  vtkSMPropertyHelper(this, "EndTime").Set(maxTime);
  if (animationTime < minTime || animationTime > maxTime)
    {
    vtkSMPropertyHelper(this, "AnimationTime").Set(minTime);
    }
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
