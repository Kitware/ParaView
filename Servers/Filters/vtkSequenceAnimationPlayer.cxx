/*=========================================================================

  Program:   ParaView
  Module:    vtkSequenceAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSequenceAnimationPlayer.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSequenceAnimationPlayer);
vtkCxxRevisionMacro(vtkSequenceAnimationPlayer, "1.3");
//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::vtkSequenceAnimationPlayer()
{
  this->NumberOfFrames = 10;
  this->FrameNo = 0;
}

//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::~vtkSequenceAnimationPlayer()
{
}

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::StartLoop(double starttime, double endtime)
{
  this->FrameNo = 0;
  this->StartTime = starttime;
  this->EndTime = endtime;
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GetNextTime(double vtkNotUsed(curtime))
{
  this->FrameNo++;
  double time = this->StartTime + 
    ((this->EndTime - this->StartTime)*this->FrameNo)/(this->NumberOfFrames-1);
  return time;
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToNext(double start, double end, double curtime)
{
  double delta = static_cast<double>(end-start)/(this->NumberOfFrames-1);
  return (curtime + delta);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToPrevious(double start, double end, double curtime)
{
  double delta = static_cast<double>(end-start)/(this->NumberOfFrames-1);
  return (curtime - delta);
}

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


