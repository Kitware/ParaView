/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrackEditor - Editor for an animation track (to add/edit/delete).
// .SECTION Description
//  Editor for an animation track (to add/edit/delete) key frames without 
//  using the tracks GUI. It also houses the GUI for Key frame. 

#ifndef __vtkPVTrackEditor_h
#define __vtkPVTrackEditor_h

#include "vtkPVTracedWidget.h"

class vtkKWFrame;
class vtkKWFrameLabeled;
class vtkKWLabel;
class vtkPVAnimationManager;
class vtkKWPushButton;
class vtkKWMenuButton;
class vtkKWScale;
class vtkPVSimpleAnimationCue;
class vtkPVKeyFrame;
class vtkPVTrackEditorObserver;

class VTK_EXPORT vtkPVTrackEditor : public vtkPVTracedWidget
{
public:
  static vtkPVTrackEditor* New();
  vtkTypeRevisionMacro(vtkPVTrackEditor, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char*args);

  // Description:
  // Animation Manager must be set before calling create.
  // This is not reference counted.
  void SetAnimationManager(vtkPVAnimationManager* pvAM) 
    { this->AnimationManager  = pvAM; }

  // Description:
  // This is the frame which should be used as parent for the vtkPVKeyFrame
  // (and subclasses) i.e. the GUI for the key frame.
  vtkGetObjectMacro(PropertiesFrame, vtkKWFrame);

  // Description:
  // This is the AnimationCue .
  void SetAnimationCue(vtkPVSimpleAnimationCue*);
 
  // Description:
  // Display the keyframe at given index.
  void SetKeyFrameIndex(int index);

  // Description:
  // Set the type of the active Key frame. If the present type of the keyframe
  // is different than the one specified, the keyframe gets replaced 
  // with a keyframe of the specified type.
  void SetKeyFrameType(int type);

  // Description:
  // Determines whether the key frame label is shown or not. Should be
  // set before Create().
  vtkSetMacro(ShowKeyFrameLabel, int);
  vtkGetMacro(ShowKeyFrameLabel, int);
  vtkBooleanMacro(ShowKeyFrameLabel, int);

  // Callbacks
  void IndexChangedCallback();
  void AddKeyFrameButtonCallback();
  void DeleteKeyFrameButtonCallback();

  // Description:
  // Updates the GUI.
  void Update();

  virtual void UpdateEnableState();
protected:
  vtkPVTrackEditor();
  ~vtkPVTrackEditor();

  vtkPVAnimationManager *AnimationManager;
  vtkPVSimpleAnimationCue* SimpleAnimationCue;
  vtkPVKeyFrame* ActiveKeyFrame;
  
  vtkKWFrameLabeled* KeyFramePropertiesFrame;
  vtkKWLabel* TitleLabelLabel;
  vtkKWLabel* TitleLabel; // label to show the cue text representation.
  vtkKWFrame* PropertiesFrame;
  vtkKWFrame* TypeFrame; //frame containing the selection for differnt types of waveforms.
  vtkKWPushButton* TypeImage;
  vtkKWMenuButton* TypeMenuButton;
  vtkKWPushButton* AddKeyFrameButton;
  vtkKWPushButton* DeleteKeyFrameButton;
  vtkKWLabel* TypeLabel;
  vtkKWScale* IndexScale;
  vtkKWLabel* SelectKeyFrameLabel;
 
  int ShowKeyFrameLabel;

  int InterpolationValid; 
  // flag indicating if the Interpolation should be enabled for the
  // current key frame. It is disabled for the last key frame.

  void BuildTypeMenu();
  void UpdateTypeImage(vtkPVKeyFrame*);
  void SetActiveKeyFrame(vtkPVKeyFrame* kf);

  // Description:
  // Set the current key frame index.
  // AnimationCueProxy must be set before calling this method.
  void ShowKeyFrame(int index);
  
  void SetAddDeleteButtonVisibility(int visible);

  vtkPVTrackEditorObserver* Observer;
private:
  vtkPVTrackEditor(const vtkPVTrackEditor&); // Not implemented.
  void operator=(const vtkPVTrackEditor&); // Not implemented.
};


#endif


