/*=========================================================================

  Program:   ParaView
  Module:    vtkRealtimeAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRealtimeAnimationPlayer
// .SECTION Description
// Animation player that plays in real time.

#ifndef __vtkRealtimeAnimationPlayer_h
#define __vtkRealtimeAnimationPlayer_h

#include "vtkAnimationPlayer.h"

class vtkTimerLog;
class VTK_EXPORT vtkRealtimeAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkRealtimeAnimationPlayer* New();
  vtkTypeMacro(vtkRealtimeAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Get/Set the duration for playing the animation in seconds.
  vtkGetMacro(Duration, unsigned long);
  vtkSetMacro(Duration, unsigned long);

//BTX
protected:
  vtkRealtimeAnimationPlayer();
  ~vtkRealtimeAnimationPlayer();

  virtual void StartLoop(double, double, double);
  virtual void EndLoop() {}

  // Description:
  // Return the next time given the current time.
  virtual double GetNextTime(double currentime);

  virtual double GoToNext(double start, double end, double currenttime);
  virtual double GoToPrevious(double start, double end, double currenttime);

  unsigned long Duration;
  double StartTime;
  double EndTime;
  double ShiftTime;
  double Factor;
  vtkTimerLog* Timer;
private:
  vtkRealtimeAnimationPlayer(const vtkRealtimeAnimationPlayer&); // Not implemented
  void operator=(const vtkRealtimeAnimationPlayer&); // Not implemented
//ETX
};

#endif

