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
// .NAME vtkAnimationPlayer
// .SECTION Description
// Abstract superclass for an animation player.

#ifndef __vtkAnimationPlayer_h
#define __vtkAnimationPlayer_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.
#include "vtkPVAnimationModule.h" // needed for export macro

class vtkSMAnimationScene;
class VTKPVANIMATION_EXPORT vtkAnimationPlayer : public vtkObject
{
public:
  vtkTypeMacro(vtkAnimationPlayer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the animation scene that is to be played by this player.
  // Note that the animation scene is not reference counted to avoid loops.
  virtual void SetAnimationScene(vtkSMAnimationScene*);
  vtkSMAnimationScene* GetAnimationScene();

  // Description:
  // Start playing the animation.
  // Fires StartEvent when play begins and EndEvent when play stops.
  void Play();

  // Description:
  // Stop playing the animation.
  void Stop();

  // Description:
  // Returns if the animation is currently playing.
  int IsInPlay() { return this->InPlay? 1 : 0; }
  vtkGetMacro(InPlay, bool);

  // Description:
  // Set to true to play the animation in a loop.
  vtkSetMacro(Loop, bool);
  vtkGetMacro(Loop, bool);

  // Description:
  // Take the animation scene to next frame.
  void GoToNext();

  // Description:
  // Take animation scene to previous frame.
  void GoToPrevious();

  // Description:
  // Take animation scene to first frame.
  void GoToFirst();

  // Description:
  // Take animation scene to last frame.
  void GoToLast();

//BTX
protected:
  vtkAnimationPlayer();
  ~vtkAnimationPlayer();

  friend class vtkCompositeAnimationPlayer;

  virtual void StartLoop(double starttime, double endtime, double* playbackWindow)=0;
  virtual void EndLoop()=0;

  // Description:
  // Return the next time given the current time.
  virtual double GetNextTime(double currentime) = 0;

  virtual double GoToNext(double start, double end, double currenttime)=0;
  virtual double GoToPrevious(double start, double end, double currenttime)=0;
private:
  vtkAnimationPlayer(const vtkAnimationPlayer&); // Not implemented
  void operator=(const vtkAnimationPlayer&); // Not implemented

  vtkWeakPointer<vtkSMAnimationScene> AnimationScene;
  bool InPlay;
  bool StopPlay;
  bool Loop;
  double CurrentTime;

//ETX
};

#endif
