/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeAnimationPlayer.h"

#include "vtkObjectFactory.h"
#include "vtkRealtimeAnimationPlayer.h"
#include "vtkSequenceAnimationPlayer.h"
#include "vtkSmartPointer.h"
#include "vtkTimestepsAnimationPlayer.h"

vtkStandardNewMacro(vtkCompositeAnimationPlayer);
//----------------------------------------------------------------------------
vtkCompositeAnimationPlayer::vtkCompositeAnimationPlayer()
{
  this->PlayMode = SEQUENCE;
  this->SequenceAnimationPlayer = vtkSequenceAnimationPlayer::New();
  this->TimestepsAnimationPlayer = vtkTimestepsAnimationPlayer::New();
  this->RealtimeAnimationPlayer = vtkRealtimeAnimationPlayer::New();
}

//----------------------------------------------------------------------------
vtkCompositeAnimationPlayer::~vtkCompositeAnimationPlayer()
{
  this->SequenceAnimationPlayer->Delete();
  this->TimestepsAnimationPlayer->Delete();
  this->RealtimeAnimationPlayer->Delete();
}

//----------------------------------------------------------------------------
vtkAnimationPlayer* vtkCompositeAnimationPlayer::GetActivePlayer()
{
  switch (this->PlayMode)
  {
    case SEQUENCE:
      return this->SequenceAnimationPlayer;

    case REAL_TIME:
      return this->RealtimeAnimationPlayer;

    case SNAP_TO_TIMESTEPS:
      return this->TimestepsAnimationPlayer;
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::StartLoop(
  double starttime, double endtime, double curtime, double* playbackWindow)
{
  vtkAnimationPlayer* player = this->GetActivePlayer();
  if (player)
  {
    player->StartLoop(starttime, endtime, curtime, playbackWindow);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::EndLoop()
{
  vtkAnimationPlayer* player = this->GetActivePlayer();
  if (player)
  {
    player->EndLoop();
  }
}

//----------------------------------------------------------------------------
double vtkCompositeAnimationPlayer::GetNextTime(double currentime)
{
  vtkAnimationPlayer* player = this->GetActivePlayer();
  if (player)
  {
    return player->GetNextTime(currentime);
  }

  return VTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------------
double vtkCompositeAnimationPlayer::GoToNext(double start, double end, double currenttime)
{
  vtkAnimationPlayer* player = this->GetActivePlayer();
  if (player)
  {
    return player->GoToNext(start, end, currenttime);
  }

  return VTK_DOUBLE_MAX;
}

//----------------------------------------------------------------------------
double vtkCompositeAnimationPlayer::GoToPrevious(double start, double end, double currenttime)
{
  vtkAnimationPlayer* player = this->GetActivePlayer();
  if (player)
  {
    return player->GoToPrevious(start, end, currenttime);
  }

  return VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PlayMode: " << this->PlayMode << endl;
}

// Forwarded to vtkSequenceAnimationPlayer
//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::SetNumberOfFrames(int val)
{
  this->SequenceAnimationPlayer->SetNumberOfFrames(val);
}

// Forwarded to vtkRealtimeAnimationPlayer.
//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::SetDuration(int val)
{
  this->RealtimeAnimationPlayer->SetDuration(val);
}

// Forwarded to vtkTimestepsAnimationPlayer.
//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::AddTimeStep(double val)
{
  this->TimestepsAnimationPlayer->AddTimeStep(val);
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::RemoveAllTimeSteps()
{
  this->TimestepsAnimationPlayer->RemoveAllTimeSteps();
}

//----------------------------------------------------------------------------
void vtkCompositeAnimationPlayer::SetFramesPerTimestep(int val)
{
  this->TimestepsAnimationPlayer->SetFramesPerTimestep(val);
}
