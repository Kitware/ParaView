/*=========================================================================

  Program:   ParaView
  Module:    vtkRealtimeAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRealtimeAnimationPlayer.h"

#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkRealtimeAnimationPlayer);
vtkCxxRevisionMacro(vtkRealtimeAnimationPlayer, "1.2");
//----------------------------------------------------------------------------
vtkRealtimeAnimationPlayer::vtkRealtimeAnimationPlayer()
{
  this->StartTime = 0;
  this->Factor = 1.0;
  this->Duration = 1;
  this->Timer = vtkTimerLog::New();
}

//----------------------------------------------------------------------------
vtkRealtimeAnimationPlayer::~vtkRealtimeAnimationPlayer()
{
  this->Timer->Delete();
}

//----------------------------------------------------------------------------
void vtkRealtimeAnimationPlayer::StartLoop(double start, double end, double curtime)
{
  this->StartTime = start;
  this->Factor = (end - start)/this->Duration;
  this->Timer->StartTimer();
}

//----------------------------------------------------------------------------
double vtkRealtimeAnimationPlayer::GetNextTime(double vtkNotUsed(curtime))
{
  this->Timer->StopTimer();
  double elapsed = this->Timer->GetElapsedTime();
  return (this->StartTime + this->Factor * elapsed);
}

//----------------------------------------------------------------------------
double vtkRealtimeAnimationPlayer::GoToNext(double, double, double currenttime)
{
  return (currenttime+1);
}

//----------------------------------------------------------------------------
double vtkRealtimeAnimationPlayer::GoToPrevious(double, double, double currenttime)
{
  return (currenttime-1);
}

//----------------------------------------------------------------------------
void vtkRealtimeAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


