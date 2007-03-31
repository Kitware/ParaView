/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVAnimationSceneProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPVAnimationSceneProxy - vtkSMAnimationSceneProxy subclass
// that have ParaView Client specific API.
// .SECTION Description
// vtkSMPVAnimationSceneProxy is a specialization of Animation Scene proxy
// which is suited to the implementation in ParaView client. It is
// not advisable to use the superclass API to set end times, frame rate etc
// directly.

#ifndef __vtkSMPVAnimationSceneProxy_h
#define __vtkSMPVAnimationSceneProxy_h

#include "vtkSMAnimationSceneProxy.h"

class VTK_EXPORT vtkSMPVAnimationSceneProxy : public vtkSMAnimationSceneProxy
{
public:
  static vtkSMPVAnimationSceneProxy* New();
  vtkTypeRevisionMacro(vtkSMPVAnimationSceneProxy, vtkSMAnimationSceneProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set number of frames to generate in the animation. This is used
  // when the play mode is SEQUENCE.
  void SetNumberOfFrames(int);
  vtkGetMacro(NumberOfFrames, int);

  // Description:
  // Set the duration (in secs) for which the animation should play.
  // This is used when play mode it REALTIME.
  void SetDuration(int);
  vtkGetMacro(Duration, int);

  // Description:
  // Get/Set the "ClockTime" range. The time is not same as Animation time,
  // it is the time min/max values through which the timekeeper's time
  // is animated in course of the animation.
  vtkGetVector2Macro(ClockTimeRange, double);
  void SetClockTimeRange(double min, double max);
  void SetClockTimeRange(double* v2)
    {
    this->SetClockTimeRange(v2[0], v2[1]);
    }

  // Description:
  // Add/Remove timesteps. Adds the timesteps to the underlying
  // vtkPVAnimationScene object. These must be called
  // only after CreateVTKObjects();
  void AddTimeStep(double time);
  void RemoveTimeStep(double time);
  void RemoveAllTimeSteps();
  unsigned int GetNumberOfTimeSteps();

  //BTX
  enum 
    {
    SNAP_TO_TIMESTEPS=REALTIME+1
    };
  //ETX

  // Description:
  // Set the play mode. Overridden to update the underlying animation scenes
  // end times, frame rate etc etc.
  virtual void SetPlayMode(int mode);

  // Description:
  // Set the time keeper proxy. The "Time" property on this
  // proxy will be animated when the scene is played or the 
  // scene time is changed.
  void SetTimeKeeper(vtkSMProxy* proxy);

  // Description:
  // Mechanism to goto start/end/next/prev depending upon
  // the choosen play mode.
  void GoToFirst();
  void GoToLast();
  void GoToNext();
  void GoToPrevious();

  // Description:
  // API to set the current clock time. This will convert the clock 
  // time to animation time and call SetAnimationTime() internally.
  // The conversion from clock time to animation time depends on 
  // ClockTimeRange and PlayMode.
  void SetClockTime(double time);

  // Description:
  // Get/Set the number of frames per timstep. Used only when 
  // play mode is PLAYMODE_TIMESTEPS. Note that this replaces
  // the FrameRate when playing in PLAYMODE_TIMESTEPS mode.
  void SetFramesPerTimestep(int);
  int GetFramesPerTimestep();
  
protected:
  vtkSMPVAnimationSceneProxy();
  ~vtkSMPVAnimationSceneProxy();

  virtual void CreateVTKObjects(int numObjects);
  virtual void TickInternal(void* info);


  double ClockTimeRange[2];
  int NumberOfFrames;
  int Duration;
  bool InSetClockTime;

  vtkSMProxy* TimeKeeper;
private:
  vtkSMPVAnimationSceneProxy(const vtkSMPVAnimationSceneProxy&); // Not implemented.
  void operator=(const vtkSMPVAnimationSceneProxy&); // Not implemented.
};

#endif

