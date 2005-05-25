/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVerticalAnimationInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVerticalAnimationInterface - Vertical subpart of the animation interface.
// .SECTION Description
//

#ifndef __vtkPVVerticalAnimationInterface_h
#define __vtkPVVerticalAnimationInterface_h

#include "vtkPVTracedWidget.h"

class vtkKWFrame;
class vtkKWFrameWithScrollbar;
class vtkKWFrameLabeled;
class vtkKWLabel;
class vtkPVAnimationCue;
class vtkPVVerticalAnimationInterfaceObserver;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWCheckButton;
class vtkPVKeyFrame;
class vtkPVAnimationManager;
class vtkKWScale;
class vtkPVTrackEditor;
class VTK_EXPORT vtkPVVerticalAnimationInterface : public vtkPVTracedWidget
{
public:
  static vtkPVVerticalAnimationInterface* New();
  vtkTypeRevisionMacro(vtkPVVerticalAnimationInterface, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates the widget.
  virtual void Create(vtkKWApplication* app, const char* args);

  // Description:
  // Set active PVAnimationCue.
  // The interface shows the details about the active PVAnimationCue.
  void SetAnimationCue(vtkPVAnimationCue*);

  // Description:
  // This is the frame to which key frame properties are to be added.
  vtkKWFrame* GetPropertiesFrame(); 
  
  // Description:
  // Frame for the Animation Control and where the scene properties
  // are shown.
  vtkKWFrame* GetScenePropertiesFrame();

  // Description:
  // Frame for the Selection interface. This contains the widget 
  // allowing the user to choose the active track to animate 
  // without using the tracks interface.
  vtkKWFrame* GetSelectorFrame();

  // Description:
  // For trace, to get the TrackEditor.
  vtkGetObjectMacro(TrackEditor, vtkPVTrackEditor);
  
  void SetAnimationManager(vtkPVAnimationManager* am) { this->AnimationManager = am;}

  // Callbacks for GUI elements.
  void RecordAllChangedCallback();
  void CacheGeometryCheckCallback();
  void AdvancedAnimationViewCallback();

  void SetAdvancedAnimationView(int advanced);

  void SaveState(ofstream* file);

  virtual void UpdateEnableState();

  void SetCacheGeometry(int cache);
  vtkGetMacro(CacheGeometry, int);

  // Description:
  // Update the GUI. Internally calls Update on
  // vtkPVTrackEditor.
  void Update();

  // Description:
  // Cache check button can be enabled/disabled.
  // It is disbled when play mode is realtime
  // and enabled when play mode is sequence.
  // Note that when enabled, it syncronizes the caching state 
  // of the AnimationManager with the current check box state.
  // Also, when disabled, the AnimationManager will set the
  // cacheing flag on Animation Scene to 0.
  void EnableCacheCheck();
  void DisableCacheCheck();
protected:
  vtkPVVerticalAnimationInterface();
  ~vtkPVVerticalAnimationInterface();

  vtkPVAnimationManager* AnimationManager;
  vtkPVTrackEditor* TrackEditor;

  vtkKWFrameWithScrollbar* TopFrame;
  vtkKWFrameLabeled* ScenePropertiesFrame;
  vtkKWFrameLabeled* SelectorFrame;

  vtkKWCheckButton* RecordAllButton;

  vtkKWFrameLabeled* SaveFrame;
  vtkKWCheckButton* CacheGeometryCheck;
  vtkKWCheckButton* AdvancedAnimationCheck;
  
  int EnableCacheCheckButton;
  int CacheGeometry;


private:
  vtkPVVerticalAnimationInterface(const vtkPVVerticalAnimationInterface&); // Not implemented.
  void operator=(const vtkPVVerticalAnimationInterface&); // Not implemented.
  
};

#endif
