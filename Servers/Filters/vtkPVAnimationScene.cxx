/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationScene.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationScene.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

#include <vtkstd/set>

class vtkPVAnimationSceneSetOfDouble : public vtkstd::set<double> {};

vtkStandardNewMacro(vtkPVAnimationScene);
vtkCxxRevisionMacro(vtkPVAnimationScene, "1.3");
//-----------------------------------------------------------------------------
vtkPVAnimationScene::vtkPVAnimationScene()
{
  this->TimeSteps = new vtkPVAnimationSceneSetOfDouble;
  this->FramesPerTimestep = 1;
}

//-----------------------------------------------------------------------------
vtkPVAnimationScene::~vtkPVAnimationScene()
{
  delete this->TimeSteps;
}


//-----------------------------------------------------------------------------
void vtkPVAnimationScene::AddTimeStep(double time)
{
  this->TimeSteps->insert(time);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveTimeStep(double time)
{
  vtkPVAnimationSceneSetOfDouble::iterator iter =
    this->TimeSteps->find(time);
  if (iter != this->TimeSteps->end())
    {
    this->TimeSteps->erase(iter);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveAllTimeSteps()
{
  this->TimeSteps->clear();
}

//-----------------------------------------------------------------------------
unsigned int vtkPVAnimationScene::GetNumberOfTimeSteps()
{
  return static_cast<unsigned int>(this->TimeSteps->size());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Play()
{
  if (this->PlayMode != PLAYMODE_TIMESTEPS)
    {
    this->Superclass::Play();
    return;
    }

  if (this->InPlay)
    {
    return;
    }

  if (this->TimeMode == vtkAnimationCue::TIMEMODE_NORMALIZED)
    {
    vtkErrorMacro("Cannot play a scene with normalized time mode");
    return;
    }
  if (this->EndTime <= this->StartTime)
    {
    vtkErrorMacro("Scene start and end times are not suitable for playing");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent);

  this->InPlay = 1;
  this->StopPlay = 0;

  double frame_count = this->FramesPerTimestep>1? this->FramesPerTimestep: 1.0;

  do
    {
    this->Initialize(); // Set the Scene in unintialized mode.

    vtkPVAnimationSceneSetOfDouble::iterator iter = this->TimeSteps->lower_bound(this->StartTime);
    if (iter == this->TimeSteps->end())
      {
      break;
      }
    double deltatime = 0.0;
    do
      {
      this->Tick((*iter), deltatime);

      // needed to compute delta times.
      double previous_tick_time = (*iter);
      iter++;

      if (iter == this->TimeSteps->end())
        {
        break;
        }

      if (frame_count> 1)
        {
        double increment = ((*iter)-previous_tick_time)/frame_count;
        double itime = previous_tick_time+increment;
        for (int cc=0; cc < frame_count-1; cc++)
          {
          this->Tick(itime, increment);
          previous_tick_time = itime;
          itime += increment;
          }
        }

      deltatime = (*iter) - previous_tick_time;
      deltatime = (deltatime < 0)? -1*deltatime : deltatime;
      } while (!this->StopPlay && this->CueState != vtkAnimationCue::INACTIVE);
    // End of loop for 1 cycle.

    } while (this->Loop && !this->StopPlay);

  this->StopPlay = 0;
  this->InPlay = 0;

  this->InvokeEvent(vtkCommand::EndEvent);
}


//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetNextTimeStep(double timestep)
{
  vtkPVAnimationSceneSetOfDouble::iterator iter = 
    this->TimeSteps->upper_bound(timestep);
  if (iter == this->TimeSteps->end())
    {
    return timestep;
    }
  return (*iter);
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetPreviousTimeStep(double timestep)
{
  double value = timestep;
  vtkPVAnimationSceneSetOfDouble::iterator iter = this->TimeSteps->begin();
  for (;iter != this->TimeSteps->end(); ++iter)
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
void vtkPVAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FramesPerTimestep: " << this->FramesPerTimestep << endl;
}
