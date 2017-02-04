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
/**
 * @class   vtkRealtimeAnimationPlayer
 *
 * Animation player that plays in real time.
*/

#ifndef vtkRealtimeAnimationPlayer_h
#define vtkRealtimeAnimationPlayer_h

#include "vtkAnimationPlayer.h"
#include "vtkPVAnimationModule.h" // needed for export macro

class vtkTimerLog;
class VTKPVANIMATION_EXPORT vtkRealtimeAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkRealtimeAnimationPlayer* New();
  vtkTypeMacro(vtkRealtimeAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the duration for playing the animation in seconds.
   */
  vtkGetMacro(Duration, unsigned long);
  vtkSetMacro(Duration, unsigned long);
  //@}

protected:
  vtkRealtimeAnimationPlayer();
  ~vtkRealtimeAnimationPlayer();

  virtual void StartLoop(double, double, double*) VTK_OVERRIDE;
  virtual void EndLoop() VTK_OVERRIDE {}

  /**
   * Return the next time given the current time.
   */
  virtual double GetNextTime(double currentime) VTK_OVERRIDE;

  virtual double GoToNext(double start, double end, double currenttime) VTK_OVERRIDE;
  virtual double GoToPrevious(double start, double end, double currenttime) VTK_OVERRIDE;

  unsigned long Duration;
  double StartTime;
  double EndTime;
  double ShiftTime;
  double Factor;
  vtkTimerLog* Timer;

private:
  vtkRealtimeAnimationPlayer(const vtkRealtimeAnimationPlayer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRealtimeAnimationPlayer&) VTK_DELETE_FUNCTION;
};

#endif
