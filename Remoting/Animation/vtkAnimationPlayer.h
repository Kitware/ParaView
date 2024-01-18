// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnimationPlayer
 *
 * Abstract superclass for an animation player.
 */

#ifndef vtkAnimationPlayer_h
#define vtkAnimationPlayer_h

#include "vtkObject.h"
#include "vtkRemotingAnimationModule.h" // needed for export macro
#include "vtkWeakPointer.h"             // needed for vtkWeakPointer.

class vtkSMAnimationScene;
class VTKREMOTINGANIMATION_EXPORT vtkAnimationPlayer : public vtkObject
{
public:
  vtkTypeMacro(vtkAnimationPlayer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the animation scene that is to be played by this player.
   * Note that the animation scene is not reference counted to avoid loops.
   */
  virtual void SetAnimationScene(vtkSMAnimationScene*);
  vtkSMAnimationScene* GetAnimationScene();
  ///@}

  /**
   * Start playing the animation.
   * Fires StartEvent when play begins and EndEvent when play stops.
   * If dir = 0, play the animation in reverse back to the beginning,
   * else play it in the forward direction.
   */
  void Play(int dir = 1);

  /**
   * Stop playing the animation.
   */
  void Stop();

  /**
   * Returns if the animation is currently playing.
   */
  int IsInPlay() { return this->InPlay ? 1 : 0; }
  vtkGetMacro(InPlay, bool);

  ///@{
  /**
   * Set to true to play the animation in a loop.
   */
  vtkSetMacro(Loop, bool);
  vtkGetMacro(Loop, bool);
  ///@}

  /**
   * Take the animation scene to next frame.
   */
  void GoToNext();

  /**
   * Take animation scene to previous frame.
   */
  void GoToPrevious();

  /**
   * Take animation scene to first frame.
   */
  void GoToFirst();

  /**
   * Take animation scene to last frame.
   */
  void GoToLast();

  ///@{
  /**
   * Get/Set the stride value fot the animation player. This will cause the player to skip
   * (n - 1) when GetNextTime, GoToNext and GoToPrevious are previous are called.
   *
   * Stride is clamped between 1 and the number of frames of the current animation scene.
   */
  vtkGetMacro(Stride, int);
  vtkSetClampMacro(Stride, int, 1, VTK_INT_MAX);
  ///@}

protected:
  vtkAnimationPlayer();
  ~vtkAnimationPlayer() override;

  friend class vtkCompositeAnimationPlayer;

  ///@{
  /**
   * Manage loop inside playbackwindow.
   */
  // initialize inner variables. Call it before any GetNextTime/GetPreviousTime call.
  virtual void StartLoop(
    double starttime, double endtime, double curtime, double* playbackWindow) = 0;
  // finalize loop
  virtual void EndLoop() = 0;
  // Return the next time in the loop given the current time.
  virtual double GetNextTime(double currentime) = 0;
  // Return the previous time in the loop given the current time.
  virtual double GetPreviousTime(double currenttime) = 0;
  ///@}

  ///@{
  /**
   * Return next/previous time knowing start, end and current.
   */
  virtual double GoToNext(double start, double end, double currenttime) = 0;
  virtual double GoToPrevious(double start, double end, double currenttime) = 0;
  ///@}

private:
  vtkAnimationPlayer(const vtkAnimationPlayer&) = delete;
  void operator=(const vtkAnimationPlayer&) = delete;

  vtkWeakPointer<vtkSMAnimationScene> AnimationScene;
  bool InPlay;
  bool StopPlay;
  bool Loop;
  double CurrentTime;
  int Stride = 1;
};

#endif
