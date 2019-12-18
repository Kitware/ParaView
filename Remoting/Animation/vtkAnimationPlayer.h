/*=========================================================================

  Program:   ParaView
  Module:    vtkAnimationPlayer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

  //@{
  /**
   * Set the animation scene that is to be played by this player.
   * Note that the animation scene is not reference counted to avoid loops.
   */
  virtual void SetAnimationScene(vtkSMAnimationScene*);
  vtkSMAnimationScene* GetAnimationScene();
  //@}

  /**
   * Start playing the animation.
   * Fires StartEvent when play begins and EndEvent when play stops.
   */
  void Play();

  /**
   * Stop playing the animation.
   */
  void Stop();

  /**
   * Returns if the animation is currently playing.
   */
  int IsInPlay() { return this->InPlay ? 1 : 0; }
  vtkGetMacro(InPlay, bool);

  //@{
  /**
   * Set to true to play the animation in a loop.
   */
  vtkSetMacro(Loop, bool);
  vtkGetMacro(Loop, bool);
  //@}

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

protected:
  vtkAnimationPlayer();
  ~vtkAnimationPlayer() override;

  friend class vtkCompositeAnimationPlayer;

  virtual void StartLoop(
    double starttime, double endtime, double curtime, double* playbackWindow) = 0;
  virtual void EndLoop() = 0;

  /**
   * Return the next time given the current time.
   */
  virtual double GetNextTime(double currentime) = 0;

  virtual double GoToNext(double start, double end, double currenttime) = 0;
  virtual double GoToPrevious(double start, double end, double currenttime) = 0;

private:
  vtkAnimationPlayer(const vtkAnimationPlayer&) = delete;
  void operator=(const vtkAnimationPlayer&) = delete;

  vtkWeakPointer<vtkSMAnimationScene> AnimationScene;
  bool InPlay;
  bool StopPlay;
  bool Loop;
  double CurrentTime;
};

#endif
