/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationScene.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationScene
// .SECTION Description
// GUI for vtkSMAnimationSceneProxy.
#ifndef __vtkPVAnimationScene_h
#define __vtkPVAnimationScene_h

#include "vtkKWWidget.h"
//class vtkCollection;
//class vtkCollectionIterator;
class vtkPVAnimationSceneObserver;
class vtkKWFrame;
class vtkKWPushButton;
class vtkKWCheckButton;
class vtkKWScale;
class vtkKWMenuButton;
class vtkKWThumbWheel;
class vtkKWLabel;
class vtkSMAnimationSceneProxy;
class vtkPVAnimationCue;
class vtkPVRenderView;
class vtkPVAnimationManager;
class vtkPVWindow;
class vtkImageWriter;
class vtkKWGenericMovieWriter;
class vtkWindowToImageFilter;
class vtkSMXMLPVAnimationWriterProxy;


class VTK_EXPORT vtkPVAnimationScene : public vtkKWWidget
{
public:
  static vtkPVAnimationScene* New();
  vtkTypeRevisionMacro(vtkPVAnimationScene, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);


  // Description:
  // Callbacks
  void SetPlayModeToSequence();
  void SetPlayModeToRealTime();
  void DurationChangedCallback();
  void LoopCheckButtonCallback();
  void TimeScaleCallback();
  void FrameRateChangedCallback();
    

  // Description:
  int  IsInPlay();
  void Play();
  void Stop();
  void GoToBeginning();
  void GoToEnd();

  // Description:
  // Goes to the next/previous frame (incremented by 1/framerate).
  void GoToNext();
  void GoToPrevious();

  // Description:
  // Get/Set the duration for which the scene is played in seconds.
  void SetDuration(double seconds);
  double GetDuration();

  // Description:
  // Set the play mode to RealTime(1) or Sequence(0).
  void SetPlayMode(int mode);
  int GetPlayMode();

  // Description:
  // Set the frame rate.
  void SetFrameRate(double fps);
  double GetFrameRate();

  // Description:
  // Set if to play the animation in a loop.
  void SetLoop(int loop);
  int GetLoop();
  
  // Description:
  // Sets the current time for the animation state.
  // Note that this time is not normalized time. It extends from
  // [0, Duration].
  void SetCurrentTime( double time);

  // Description:
  // Sets the current time for the animation state. 
  // This is normalized time [0,1], normalized to the duration 
  // of the scene.
  void SetNormalizedCurrentTime(double ntime);

  // Description:
  // Add/Remove animation cues from the scene. PVAnimationCues are not
  // reference counted. Hence, PVAnimationCue must ensure that it is
  // removed from the PVScene before it is deleted.
  void AddAnimationCue(vtkPVAnimationCue* cue);
  void RemoveAnimationCue(vtkPVAnimationCue* cue);

  virtual void SaveInBatchScript(ofstream* file);

  void SaveState(ofstream* file);

  // Description:
  // Access to the render view. Needed to build up the source list.
  // SetWindow can't be an object macro because that sets up a circular
  // reference.
  virtual void SetWindow(vtkPVWindow *window);
  vtkGetObjectMacro(Window, vtkPVWindow);

  void SetRenderView(vtkPVRenderView* pvRenderView);
  vtkGetObjectMacro(RenderView, vtkPVRenderView);

  void SetAnimationManager(vtkPVAnimationManager*);

  virtual void UpdateEnableState();


  void SaveImages(const char* fileRoot, const char* ext, int width, int height, 
                                         int aspectRatio);
  void SaveGeometry(const char* filename);

  void InvalidateAllGeometries();

protected:
  vtkPVAnimationScene();
  ~vtkPVAnimationScene();

  vtkPVRenderView* RenderView;
  vtkPVWindow* Window;
  vtkPVAnimationManager* AnimationManager;

  // Animation Control.
  vtkKWFrame* ControlButtonFrame;
  vtkKWPushButton *PlayButton;
  vtkKWPushButton *StopButton;
  vtkKWPushButton *GoToBeginningButton;
  vtkKWPushButton *GoToEndButton;
  vtkKWPushButton *GoToPreviousButton;
  vtkKWPushButton *GoToNextButton;
  vtkKWCheckButton *LoopCheckButton;

  vtkKWLabel* TimeLabel;
  vtkKWScale* TimeScale;

  vtkKWLabel* FrameRateLabel;
  vtkKWThumbWheel* FrameRateThumbWheel; 

  vtkKWLabel* DurationLabel;
  vtkKWThumbWheel* DurationThumbWheel; 

  vtkKWLabel* PlayModeLabel;
  vtkKWMenuButton* PlayModeMenuButton;
  
  vtkImageWriter* ImageWriter;
  vtkKWGenericMovieWriter* MovieWriter;
  vtkWindowToImageFilter* WindowToImageFilter;
  char* FileRoot;
  char* FileExtension;
  int FileCount;
  int SaveFailed;
  vtkSetStringMacro(FileRoot);
  vtkSetStringMacro(FileExtension);

  vtkSMXMLPVAnimationWriterProxy* GeometryWriter;
  
  vtkSMAnimationSceneProxy* AnimationSceneProxy;
  char* AnimationSceneProxyName;
  vtkSetStringMacro(AnimationSceneProxyName);

  // Description:
  // Called to dump frame into images/movie.
  void SaveImages();

  void SaveGeometry(double time);

  virtual void ExecuteEvent(vtkObject* , unsigned long event, void* calldata);
//BTX
  vtkPVAnimationSceneObserver* Observer;
  friend class vtkPVAnimationSceneObserver;
//ETX
  void CreateProxy();
  int InPlay;

  
  unsigned long ErrorEventTag;
//  vtkCollection* AnimationCues;
//  vtkCollectionIterator* AnimationCuesIterator;
private:
  vtkPVAnimationScene(const vtkPVAnimationScene&); // Not implemented.
  void operator=(const vtkPVAnimationScene&); // Not implemented.
};

#endif
