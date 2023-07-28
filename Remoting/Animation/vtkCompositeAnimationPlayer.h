// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeAnimationPlayer
 *
 * This is composite animation player that can me made to play an animation
 * using the active player. It provides API to add animation players and then
 * set one of them as the active one.
 */

#ifndef vtkCompositeAnimationPlayer_h
#define vtkCompositeAnimationPlayer_h

#include "vtkAnimationPlayer.h"
#include "vtkRemotingAnimationModule.h" // needed for export macro

#include "vtkParaViewDeprecation.h"

class vtkSequenceAnimationPlayer;
class vtkTimestepsAnimationPlayer;

class VTKREMOTINGANIMATION_EXPORT vtkCompositeAnimationPlayer : public vtkAnimationPlayer
{
public:
  static vtkCompositeAnimationPlayer* New();
  vtkTypeMacro(vtkCompositeAnimationPlayer, vtkAnimationPlayer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum Modes
  {
    SEQUENCE = 0,
    SNAP_TO_TIMESTEPS = 2
  };

  ///@{
  /**
   * Get/Set the play mode
   */
  vtkSetMacro(PlayMode, int);
  vtkGetMacro(PlayMode, int);
  ///@}

  /**
   * Forwarded to vtkSequenceAnimationPlayer
   */
  void SetNumberOfFrames(int val);

  /**
   * Forwarded to vtkRealtimeAnimationPlayer.
   */
  PARAVIEW_DEPRECATED_IN_5_12_0("Use `SetStride` and vtkSequenceAnimationPlayer instead")
  void SetDuration(int val);

  ///@{
  /**
   * Forwarded to vtkTimestepsAnimationPlayer.
   */
  void AddTimeStep(double val);
  void RemoveAllTimeSteps();
  void SetFramesPerTimestep(int val);
  ///@}

  ///@{
  /**
   * Forwarded to vtkTimestepsAnimationPlayer.and vtkSequenceAnimationPlayer.
   */
  void SetStride(int _val) override;
  ///@}

protected:
  vtkCompositeAnimationPlayer();
  ~vtkCompositeAnimationPlayer() override;

  ///@{
  /**
   * Delegated to the active animation player.
   */
  void StartLoop(double starttime, double endtime, double curtime, double* playbackWindow) override;
  void EndLoop() override;
  double GetNextTime(double currentime) override;
  double GetPreviousTime(double currenttime) override;
  double GoToNext(double start, double end, double currenttime) override;
  double GoToPrevious(double start, double end, double currenttime) override;
  ///@}

  vtkAnimationPlayer* GetActivePlayer();

  vtkSequenceAnimationPlayer* SequenceAnimationPlayer;
  vtkTimestepsAnimationPlayer* TimestepsAnimationPlayer;

  int PlayMode;

private:
  vtkCompositeAnimationPlayer(const vtkCompositeAnimationPlayer&) = delete;
  void operator=(const vtkCompositeAnimationPlayer&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
