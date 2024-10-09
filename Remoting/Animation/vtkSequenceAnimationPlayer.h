// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSequenceAnimationPlayer
 *
 *
 */

#ifndef vtkSequenceAnimationPlayer_h
#define vtkSequenceAnimationPlayer_h

#include "vtkAnimationPlayer.h"
#include "vtkRemotingAnimationModule.h" // needed for export macro

class VTKREMOTINGANIMATION_EXPORT vtkSequenceAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkSequenceAnimationPlayer* New();
  vtkTypeMacro(vtkSequenceAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetClampMacro(NumberOfFrames, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfFrames, int);

protected:
  vtkSequenceAnimationPlayer();
  ~vtkSequenceAnimationPlayer() override;

  ///@{
  /**
   * Manage loop inside playbackwindow.
   */
  // initialize inner variables. Call it before any GetNextTime/GetPreviousTime call.
  void StartLoop(double start, double end, double, double* playbackwindow) override;
  void EndLoop() override{};
  // Get next time in loop. Overriden to update FrameNo, and use StartTime, EndTime.
  double GetNextTime(double currentime) override;
  // Get previous time in loop. Overriden to update FrameNo, and use StartTime, EndTime.
  double GetPreviousTime(double currenttime) override;
  ///@}

  ///@{
  /**
   * Return previous / next time, using Stride.
   * Always compute it from `start + newTimestep * deltaTime`
   * to avoid numerical errors that can be occured if we just
   * do `deltaTime + currenttime`
   */
  double GoToNext(double start, double end, double currenttime) override;
  double GoToPrevious(double start, double end, double currenttime) override;
  ///@}

  /**
   * Return timestep associated to "current" time.
   * Compute the duration of a timestep so it cuts [start, end] interval into NumberOfFrames
   * element. Divide (current-start) length by this duration, to get the timestep.
   */
  int GetTimestep(double start, double end, double current);

  /**
   * Compute time value from timestep, start and end.
   * Use NumberOfFrames.
   */
  double GetTimeFromTimestep(double start, double end, int timestep);

  int NumberOfFrames;
  int MaxFrameWindow;
  double StartTime;
  double EndTime;
  int FrameNo;

private:
  vtkSequenceAnimationPlayer(const vtkSequenceAnimationPlayer&) = delete;
  void operator=(const vtkSequenceAnimationPlayer&) = delete;
};

#endif
