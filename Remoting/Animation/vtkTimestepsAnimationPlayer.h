// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTimestepsAnimationPlayer
 * @brief   vtkAnimationPlayer subclass
 * that plays through a discrete set of time values.
 *
 * Player to play an animation scene through a discrete set of time values.
 * FramesPerTimestep controls how many frames are generated for each time value.
 */

#ifndef vtkTimestepsAnimationPlayer_h
#define vtkTimestepsAnimationPlayer_h

#include "vtkAnimationPlayer.h"
#include "vtkRemotingAnimationModule.h" // needed for export macro

class vtkTimestepsAnimationPlayerSetOfDouble;

class VTKREMOTINGANIMATION_EXPORT vtkTimestepsAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkTimestepsAnimationPlayer* New();
  vtkTypeMacro(vtkTimestepsAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Add/Remove timesteps.
   */
  void AddTimeStep(double time);
  void RemoveTimeStep(double time);
  ///@}

  /**
   * Remove all timesteps.
   */
  void RemoveAllTimeSteps();

  /**
   * Get number of timesteps.
   */
  unsigned int GetNumberOfTimeSteps();

  ///@{
  /**
   * Get/Set the number of frames per timstep.
   */
  vtkSetClampMacro(FramesPerTimestep, unsigned long, 1, VTK_UNSIGNED_LONG_MAX);
  vtkGetMacro(FramesPerTimestep, unsigned long);
  ///@}

  /**
   * Returns the timestep value after the given timestep.
   * If no value exists, returns the argument \c time itself.
   */
  double GetNextTimeStep(double time);

  ///@{
  /**
   * Returns the timestep value before the given timestep.
   * If no value exists, returns the argument \c time itself.
   */
  double GetPreviousTimeStep(double time);

protected:
  vtkTimestepsAnimationPlayer();
  ~vtkTimestepsAnimationPlayer() override;
  ///@}

  ///@{
  /**
   * Manage loop inside playbackwindow.
   */
  // Initialize inner variables. Call it before any GetNextTime/GetPreviousTime call.
  void StartLoop(double, double, double, double* playbackWindow) override;
  void EndLoop() override{};
  // Get next time in loop. Take Count and Stride into account.
  double GetNextTime(double currentime) override;
  // Get previous time in loop. Take Count and Stride into account.
  double GetPreviousTime(double currenttime) override;
  ///@}

  double GoToNext(double, double, double currenttime) override
  {
    return this->GetNextTimeStep(currenttime);
  }

  double GoToPrevious(double, double, double currenttime) override
  {
    return this->GetPreviousTimeStep(currenttime);
  }

  double PlaybackWindow[2];
  unsigned long FramesPerTimestep;
  unsigned long Count;

private:
  vtkTimestepsAnimationPlayer(const vtkTimestepsAnimationPlayer&) = delete;
  void operator=(const vtkTimestepsAnimationPlayer&) = delete;

  /**
   * Return next time from TimeSteps, using Stride.
   */
  double GetNextInternal(double time, double defaultVal);

  vtkTimestepsAnimationPlayerSetOfDouble* TimeSteps;
};

#endif
