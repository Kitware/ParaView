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

#include "vtkKWWidget.h"

class vtkKWFrame;
class vtkKWLabeledFrame;
class vtkKWLabel;
class vtkPVAnimationCue;
class vtkPVVerticalAnimationInterfaceObserver;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWCheckButton;
class vtkPVKeyFrame;
class vtkPVAnimationManager;
class vtkKWScale;
class VTK_EXPORT vtkPVVerticalAnimationInterface : public vtkKWWidget
{
public:
  static vtkPVVerticalAnimationInterface* New();
  vtkTypeRevisionMacro(vtkPVVerticalAnimationInterface, vtkKWWidget);
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
  void InitStateCallback();
  void KeyFrameChangesCallback();
  void CacheGeometryCheckCallback();

  void SaveState(ofstream* file);

  virtual void UpdateEnableState();

  void SetCacheGeometry(int cache);
  vtkGetMacro(CacheGeometry, int);

  void SetKeyFrameIndex(int index);
protected:
  vtkPVVerticalAnimationInterface();
  ~vtkPVVerticalAnimationInterface();

  vtkPVAnimationManager* AnimationManager;
  vtkKWFrame* TopFrame;
  vtkKWLabeledFrame* ScenePropertiesFrame;
  vtkKWLabeledFrame* KeyFramePropertiesFrame;
  vtkKWFrame* PropertiesFrame;
  vtkKWFrame* TypeFrame; //frame containing the selection for differnt types of waveforms.
  vtkKWPushButton* TypeImage;
  vtkKWMenuButton* TypeMenuButton;
  vtkKWLabel* TypeLabel;
  vtkKWLabel* IndexLabel;
  vtkKWScale* IndexScale;
  vtkKWLabel* SelectKeyFrameLabel;

  vtkKWLabeledFrame* RecorderFrame;
  vtkKWPushButton* InitStateButton;
  vtkKWPushButton* KeyFrameChangesButton;
  vtkKWLabel* RecordAllLabel;
  vtkKWCheckButton* RecordAllButton;

  vtkKWLabeledFrame* SaveFrame;
  vtkKWCheckButton* CacheGeometryCheck;
  
  vtkPVAnimationCue* AnimationCue;
  vtkPVKeyFrame* ActiveKeyFrame;
  void SetActiveKeyFrame(vtkPVKeyFrame*);

  int CacheGeometry;

  //BTX
  friend class vtkPVVerticalAnimationInterfaceObserver;
  vtkPVVerticalAnimationInterfaceObserver* Observer;
  //ETX
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* calldata);
  void InitializeObservers(vtkPVAnimationCue* cue);
  void RemoveObservers(vtkPVAnimationCue* cue);

  // Update the display.
  void Update();

  void BuildTypeMenu();
  void UpdateTypeImage(vtkPVKeyFrame*);
  void ShowKeyFrame(int id);

private:
  vtkPVVerticalAnimationInterface(const vtkPVVerticalAnimationInterface&); // Not implemented.
  void operator=(const vtkPVVerticalAnimationInterface&); // Not implemented.
  
};

#endif
