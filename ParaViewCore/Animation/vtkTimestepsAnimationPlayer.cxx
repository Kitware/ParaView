/*=========================================================================

  Program:   ParaView
  Module:    vtkTimestepsAnimationPlayer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTimestepsAnimationPlayer.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

#include <set>

class vtkTimestepsAnimationPlayerSetOfDouble : public std::set<double>
{
};

vtkStandardNewMacro(vtkTimestepsAnimationPlayer);
//-----------------------------------------------------------------------------
vtkTimestepsAnimationPlayer::vtkTimestepsAnimationPlayer()
{
  this->TimeSteps = new vtkTimestepsAnimationPlayerSetOfDouble;
  this->FramesPerTimestep = 1;
}

//-----------------------------------------------------------------------------
vtkTimestepsAnimationPlayer::~vtkTimestepsAnimationPlayer()
{
  delete this->TimeSteps;
}

//-----------------------------------------------------------------------------
void vtkTimestepsAnimationPlayer::AddTimeStep(double time)
{
  this->TimeSteps->insert(time);
}

//-----------------------------------------------------------------------------
void vtkTimestepsAnimationPlayer::RemoveTimeStep(double time)
{
  vtkTimestepsAnimationPlayerSetOfDouble::iterator iter = this->TimeSteps->find(time);
  if (iter != this->TimeSteps->end())
  {
    this->TimeSteps->erase(iter);
  }
}

//-----------------------------------------------------------------------------
void vtkTimestepsAnimationPlayer::RemoveAllTimeSteps()
{
  this->TimeSteps->clear();
}

//-----------------------------------------------------------------------------
unsigned int vtkTimestepsAnimationPlayer::GetNumberOfTimeSteps()
{
  return static_cast<unsigned int>(this->TimeSteps->size());
}

//-----------------------------------------------------------------------------
void vtkTimestepsAnimationPlayer::StartLoop(double, double, double, double* playbackWindow)
{
  this->PlaybackWindow[0] = playbackWindow[0];
  this->PlaybackWindow[1] = playbackWindow[1];
  this->Count = 0;
}

//-----------------------------------------------------------------------------
double vtkTimestepsAnimationPlayer::GetNextTime(double currentime)
{
  this->Count++;
  if (this->Count < this->FramesPerTimestep)
  {
    return currentime;
  }

  this->Count = 0;
  vtkTimestepsAnimationPlayerSetOfDouble::iterator iter = this->TimeSteps->upper_bound(currentime);
  if (iter == this->TimeSteps->end() || currentime >= this->PlaybackWindow[1])
  {
    return VTK_DOUBLE_MAX;
  }

  return (*iter);
}

//-----------------------------------------------------------------------------
double vtkTimestepsAnimationPlayer::GetNextTimeStep(double timestep)
{
  vtkTimestepsAnimationPlayerSetOfDouble::iterator iter = this->TimeSteps->upper_bound(timestep);
  if (iter == this->TimeSteps->end())
  {
    return timestep;
  }
  return (*iter);
}

//-----------------------------------------------------------------------------
double vtkTimestepsAnimationPlayer::GetPreviousTimeStep(double timestep)
{
  double value = timestep;
  vtkTimestepsAnimationPlayerSetOfDouble::iterator iter = this->TimeSteps->begin();
  for (; iter != this->TimeSteps->end(); ++iter)
  {
    if ((*iter) >= timestep)
    {
      return value;
    }
    value = (*iter);
  }
  return value;
}

//-----------------------------------------------------------------------------
void vtkTimestepsAnimationPlayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FramesPerTimestep: " << this->FramesPerTimestep << endl;
}
