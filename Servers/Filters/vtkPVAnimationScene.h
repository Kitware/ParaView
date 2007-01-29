/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationScene.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationScene - vtkAnimationScene subclass
// that adds a new play mode to play through available timesteps.
// .SECTION Description
// vtkAnimationScene subclass
// that adds a new play mode to play through available timesteps.

#ifndef __vtkPVAnimationScene_h
#define __vtkPVAnimationScene_h

#include "vtkAnimationScene.h"

class vtkPVAnimationSceneSetOfDouble;

class VTK_EXPORT vtkPVAnimationScene : public vtkAnimationScene
{
public:
  static vtkPVAnimationScene* New();
  vtkTypeRevisionMacro(vtkPVAnimationScene, vtkAnimationScene);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add/Remove timesteps. 
  void AddTimeStep(double time);
  void RemoveTimeStep(double time);
  void RemoveAllTimeSteps();

  // Description:
  // Get number of timesteps.
  unsigned int GetNumberOfTimeSteps();

  // Description:
  // Play the animation.
  virtual void Play();

  // Description:
  // Returns the timestep value after the given timestep.
  // If no value exists, returns the argument \c time itself.
  double GetNextTimeStep(double time);

  // Description:
  // Returns the timestep value before the given timestep.
  // If no value exists, returns the argument \c time itself.
  double GetPreviousTimeStep(double time);

  //BTX
  enum 
    {
    PLAYMODE_TIMESTEPS = PLAYMODE_REALTIME+1
    };
  //ETX
protected:
  vtkPVAnimationScene();
  ~vtkPVAnimationScene();

private:
  vtkPVAnimationScene(const vtkPVAnimationScene&); // Not implemented.
  void operator=(const vtkPVAnimationScene&); // Not implemented.

  vtkPVAnimationSceneSetOfDouble* TimeSteps;
};

#endif


