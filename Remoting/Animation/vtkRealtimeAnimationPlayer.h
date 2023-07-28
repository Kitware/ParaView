// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRealtimeAnimationPlayer
 *
 * Animation player that plays in real time.
 */

#ifndef vtkRealtimeAnimationPlayer_h
#define vtkRealtimeAnimationPlayer_h

#include "vtkAnimationPlayer.h"
#include "vtkRemotingAnimationModule.h" // needed for export macro

#include "vtkParaViewDeprecation.h"

class vtkTimerLog;

class PARAVIEW_DEPRECATED_IN_5_12_0(
  "Use `vtkSequenceAnimationPlayer` instead") VTKREMOTINGANIMATION_EXPORT vtkRealtimeAnimationPlayer
  : public vtkAnimationPlayer
{
public:
  static vtkRealtimeAnimationPlayer* New();
  vtkTypeMacro(vtkRealtimeAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the duration for playing the animation in seconds.
   */
  vtkGetMacro(Duration, unsigned long);
  vtkSetMacro(Duration, unsigned long);
  ///@}

  ///@{
  /**
   * Setter is noop, getter return 1.
   *
   * Stride is not relevant for realtime animation so let it be always 1.
   */
  void SetStride(int) final {}
  int GetStride() final { return 1; }
  ///@}

protected:
  vtkRealtimeAnimationPlayer();
  ~vtkRealtimeAnimationPlayer() override;

  void StartLoop(double, double, double, double*) override;
  void EndLoop() override {}

  /**
   * Return the next time given the current time.
   */
  double GetNextTime(double currentime) override;

  /**
   * Return the previous time given the current time.
   */
  double GetPreviousTime(double currenttime) override;

  double GoToNext(double start, double end, double currenttime) override;
  double GoToPrevious(double start, double end, double currenttime) override;

  unsigned long Duration;
  double StartTime;
  double EndTime;
  double ShiftTime;
  double Factor;
  vtkTimerLog* Timer;

private:
  vtkRealtimeAnimationPlayer(const vtkRealtimeAnimationPlayer&) = delete;
  void operator=(const vtkRealtimeAnimationPlayer&) = delete;
};

#endif
