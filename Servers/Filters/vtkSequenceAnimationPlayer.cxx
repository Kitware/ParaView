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
vtkCxxRevisionMacro(vtkSequenceAnimationPlayer, "1.1");
//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::vtkSequenceAnimationPlayer()
{
  this->NumberOfFrames = 10;
  this->Delta = 0;
}

//----------------------------------------------------------------------------
vtkSequenceAnimationPlayer::~vtkSequenceAnimationPlayer()
{
}

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::StartLoop(double starttime, double endtime)
{
  this->Delta = static_cast<double>(endtime-starttime)/(this->NumberOfFrames-1);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GetNextTime(double curtime)
{
  return (curtime + this->Delta);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToNext(double start, double end, double curtime)
{
  double delta = static_cast<double>(end-start)/this->NumberOfFrames;
  return (curtime + delta);
}

//----------------------------------------------------------------------------
double vtkSequenceAnimationPlayer::GoToPrevious(double start, double end, double curtime)
{
  double delta = static_cast<double>(end-start)/this->NumberOfFrames;
  return (curtime - delta);
}

//----------------------------------------------------------------------------
void vtkSequenceAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


