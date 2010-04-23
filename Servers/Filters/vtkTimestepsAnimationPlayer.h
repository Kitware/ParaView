/*=========================================================================

  Program:   ParaView
  Module:    vtkTimestepsAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTimestepsAnimationPlayer - vtkAnimationPlayer subclass
// that plays through a discrete set of time values.
// .SECTION Description
// Player to play an animation scene through a discrete set of time values.
// FramesPerTimestep controls how many frames are generated for each time value.

#ifndef __vtkTimestepsAnimationPlayer_h
#define __vtkTimestepsAnimationPlayer_h

#include "vtkAnimationPlayer.h"

class vtkTimestepsAnimationPlayerSetOfDouble;

class VTK_EXPORT vtkTimestepsAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkTimestepsAnimationPlayer* New();
  vtkTypeMacro(vtkTimestepsAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add/Remove timesteps. 
  void AddTimeStep(double time);
  void RemoveTimeStep(double time);

  // Description:
  // Remove all timesteps.
  void RemoveAllTimeSteps();

  // Description:
  // Get number of timesteps.
  unsigned int GetNumberOfTimeSteps();

  // Description:
  // Get/Set the number of frames per timstep. 
  vtkSetClampMacro(FramesPerTimestep, unsigned long, 1, VTK_UNSIGNED_LONG_MAX);
  vtkGetMacro(FramesPerTimestep, unsigned long);

  // Description:
  // Returns the timestep value after the given timestep.
  // If no value exists, returns the argument \c time itself.
  double GetNextTimeStep(double time);

  // Description:
  // Returns the timestep value before the given timestep.
  // If no value exists, returns the argument \c time itself.
  double GetPreviousTimeStep(double time);
protected:
  vtkTimestepsAnimationPlayer();
  ~vtkTimestepsAnimationPlayer();

  virtual void StartLoop(double, double, double);
  virtual void EndLoop() {};

  // Description:
  // Return the next time given the current time.
  virtual double GetNextTime(double currentime);


  virtual double GoToNext(double, double, double currenttime)
    {
    return this->GetNextTimeStep(currenttime);
    }

  virtual double GoToPrevious(double, double, double currenttime)
    {
    return this->GetPreviousTimeStep(currenttime);
    }

  unsigned long FramesPerTimestep;
  unsigned long Count;
private:
  vtkTimestepsAnimationPlayer(const vtkTimestepsAnimationPlayer&); // Not implemented.
  void operator=(const vtkTimestepsAnimationPlayer&); // Not implemented.

  vtkTimestepsAnimationPlayerSetOfDouble* TimeSteps;
};

#endif


