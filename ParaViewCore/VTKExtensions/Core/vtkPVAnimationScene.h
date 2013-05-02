/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationScene - the animation scene manager.
// .SECTION Description
// vtkAnimationCue and vtkPVAnimationScene provide the framework to support
// animations in VTK. vtkAnimationCue represents an entity that changes/
// animates with time, while vtkPVAnimationScene represents scene or setup 
// for the animation, which consists of individual cues or other scenes.
//
// The main difference between vtkAnimationScene and vtkPVAnimationScene is that
// vtkPVAnimationScene does not include any of the scene playing logic. All that
// has been moved to the vtkAnimationPlayer (and subclasses).
// .SECTION See Also
// vtkAnimationCue

#ifndef __vtkPVAnimationScene_h
#define __vtkPVAnimationScene_h

#include "vtkAnimationCue.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

class vtkAnimationCue;
class vtkCollection;
class vtkCollectionIterator;
class vtkTimerLog;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVAnimationScene: public vtkAnimationCue
{
public:
  vtkTypeMacro(vtkPVAnimationScene, vtkAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVAnimationScene* New();

  // Description:
  // Add/Remove an AnimationCue to/from the Scene.
  // It's an error to add a cue twice to the Scene.
  void AddCue(vtkAnimationCue* cue);
  void RemoveCue(vtkAnimationCue* cue);
  void RemoveAllCues();
  int  GetNumberOfCues();
 
  // Description:
  // Sets the current animation time.
  void SetSceneTime(double time)
    {
    if (this->InTick)
      {
      // Since this method can be called during a Tick() event handler.
      return;
      }
    this->Initialize();
    this->Tick(time, 0, time); 
    }

  // Get the time of the most recent tick.
  // The only difference between this and AnimationTime (or ClockTime) defined
  // in the superclass is that, unlike the latter this is defined even outside
  // AnimationCueTickEvent handlers.
  vtkGetMacro(SceneTime, double);

  // Description:
  // Get/Set the Playback Window for this cue.
  // The Playback Window is use to mask out time that belong to a given cue
  // but that we don't want to play back.
  // This is particulary useful when we want to export a subset of an animation
  // without recomputing any start and end value relative to the cue and the
  // number of frame associated to it.
  // This is used by the Animation Player to only play a subset of the cue.
  // To disable it just make the lower bound bigger than the upper one.
  vtkSetVector2Macro(PlaybackTimeWindow, double);
  vtkGetVector2Macro(PlaybackTimeWindow, double);

protected:
  vtkPVAnimationScene();
  ~vtkPVAnimationScene();

  // Description:
  // Called on every valid tick.
  // Calls ticks on all the contained cues.
  virtual void StartCueInternal();
  virtual void TickInternal(double currenttime, double deltatime, double clocktime);
  virtual void EndCueInternal();

  void InitializeChildren();
  void FinalizeChildren();
  
  vtkCollection* AnimationCues;
  vtkCollectionIterator* AnimationCuesIterator;

  bool InTick;
  double SceneTime;
  double PlaybackTimeWindow[2];
private:
  vtkPVAnimationScene(const vtkPVAnimationScene&); // Not implemented.
  void operator=(const vtkPVAnimationScene&); // Not implemented.
};

#endif
