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
  vtkGetObjectMacro(AnimationCue, vtkPVAnimationCue);

  // Description:
  // This is the frame to which key frame properties are to be added.
  vtkKWFrame* GetPropertiesFrame(); 
  
  // Description:
  // Frame for the Animation Control and where the scene properties
  // are shown.
  vtkKWFrame* GetScenePropertiesFrame();
  void SetKeyFrameType(int type);

  void SetAnimationManager(vtkPVAnimationManager* am) { this->AnimationManager = am;}

  void IndexChangedCallback();
  void RecordAllChangedCallback();
  void CacheGeometryCheckCallback();
  void AdvancedAnimationViewCallback();

  void SetAdvancedAnimationView(int advanced);

  void SaveState(ofstream* file);

  virtual void UpdateEnableState();

  void SetCacheGeometry(int cache);
  vtkGetMacro(CacheGeometry, int);

  void SetKeyFrameIndex(int index);

  // Update the display.
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
  vtkKWFrameWithScrollbar* TopFrame;
  vtkKWFrameLabeled* ScenePropertiesFrame;
  vtkKWFrameLabeled* KeyFramePropertiesFrame;
  vtkKWLabel* TitleLabelLabel;
  vtkKWLabel* TitleLabel; // label to show the cue text representation.
  vtkKWFrame* PropertiesFrame;
  vtkKWFrame* TypeFrame; //frame containing the selection for differnt types of waveforms.
  vtkKWPushButton* TypeImage;
  vtkKWMenuButton* TypeMenuButton;
  vtkKWLabel* TypeLabel;
  vtkKWScale* IndexScale;
  vtkKWLabel* SelectKeyFrameLabel;

  vtkKWCheckButton* RecordAllButton;

  vtkKWFrameLabeled* SaveFrame;
  vtkKWCheckButton* CacheGeometryCheck;
  vtkKWCheckButton* AdvancedAnimationCheck;
  
  vtkPVAnimationCue* AnimationCue;
  vtkPVKeyFrame* ActiveKeyFrame;
  void SetActiveKeyFrame(vtkPVKeyFrame*);

  int EnableCacheCheckButton;
  int CacheGeometry;

  //BTX
  friend class vtkPVVerticalAnimationInterfaceObserver;
  vtkPVVerticalAnimationInterfaceObserver* Observer;
  //ETX
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* calldata);
  void InitializeObservers(vtkPVAnimationCue* cue);
  void RemoveObservers(vtkPVAnimationCue* cue);

  void BuildTypeMenu();
  void UpdateTypeImage(vtkPVKeyFrame*);
  void ShowKeyFrame(int id);

private:
  vtkPVVerticalAnimationInterface(const vtkPVVerticalAnimationInterface&); // Not implemented.
  void operator=(const vtkPVVerticalAnimationInterface&); // Not implemented.
  
};

#endif
