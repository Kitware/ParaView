/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkSequenceAnimationPlayer;
class vtkRealtimeAnimationPlayer;
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
    REAL_TIME = 1,
    SNAP_TO_TIMESTEPS = 2
  };

  //@{
  /**
   * Get/Set the play mode
   */
  vtkSetMacro(PlayMode, int);
  vtkGetMacro(PlayMode, int);
  //@}

  /**
   * Forwarded to vtkSequenceAnimationPlayer
   */
  void SetNumberOfFrames(int val);

  /**
   * Forwarded to vtkRealtimeAnimationPlayer.
   */
  void SetDuration(int val);

  //@{
  /**
   * Forwarded to vtkTimestepsAnimationPlayer.
   */
  void AddTimeStep(double val);
  void RemoveAllTimeSteps();
  void SetFramesPerTimestep(int val);
  //@}

protected:
  vtkCompositeAnimationPlayer();
  ~vtkCompositeAnimationPlayer() override;

  //@{
  /**
   * Delegated to the active animation player.
   */
  void StartLoop(double starttime, double endtime, double curtime, double* playbackWindow) override;
  void EndLoop() override;
  double GetNextTime(double currentime) override;
  //@}

  double GoToNext(double start, double end, double currenttime) override;
  double GoToPrevious(double start, double end, double currenttime) override;

  vtkAnimationPlayer* GetActivePlayer();

  vtkSequenceAnimationPlayer* SequenceAnimationPlayer;
  vtkRealtimeAnimationPlayer* RealtimeAnimationPlayer;
  vtkTimestepsAnimationPlayer* TimestepsAnimationPlayer;

  int PlayMode;

private:
  vtkCompositeAnimationPlayer(const vtkCompositeAnimationPlayer&) = delete;
  void operator=(const vtkCompositeAnimationPlayer&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
