/*=========================================================================

  Program:   ParaView
  Module:    vtkPVKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVKeyFrame - superclass for Key Frame GUIs
// .SECTION Description
//

#ifndef __vtkPVKeyFrame_h
#define __vtkPVKeyFrame_h

#include "vtkKWWidget.h"
class vtkSMKeyFrameProxy;
class vtkKWThumbWheel;
class vtkPVWidget;
class vtkKWLabel;
class vtkKWPushButton;
class vtkPVKeyFrameObserver;
class vtkPVAnimationCue;

class VTK_EXPORT vtkPVKeyFrame : public vtkKWWidget
{
public:
  vtkTypeRevisionMacro(vtkPVKeyFrame, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);

  vtkGetObjectMacro(KeyFrameProxy, vtkSMKeyFrameProxy);

  // Description:
  // Get/Set the key time: the time for this key frame.
  void SetKeyTime(double time);
  double GetKeyTime();

  void SetKeyValue(double value);
  double GetKeyValue();

  // Description:
  // Initialized Key Value using current animated property value.
  virtual void InitializeKeyValueUsingCurrentState();
 
  // Description:
  // Initialize the Key Value bounds using current animatied property value
  // and domain state.
  virtual void InitializeKeyValueDomainUsingCurrentState();

  // Description:
  // If set, these ranges can be used to bound the values.
  // Timebounds are in normalized time.
  void SetTimeMinimumBound(double min);
  void SetTimeMaximumBound(double max);
  void ClearTimeBounds();
  
  void ValueChangedCallback();
  void TimeChangedCallback();
  void MinimumCallback();
  void MaximumCallback();

  virtual void PrepareForDisplay();
 
  // Description:
  // A pointer to the vtkPVAnimationCue is need so that the keyframe
  // can obtain information about the animated property. Must be set before
  // calling create and should not be changed thereafter.
  void SetAnimationCue(vtkPVAnimationCue* cue) { this->AnimationCue = cue; }
  vtkGetObjectMacro(AnimationCue, vtkPVAnimationCue);

  virtual void SaveState(ofstream* file);

  virtual void UpdateEnableState();


  // Description:
  // Name is used for trace alone. Do not use this directly.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
protected:
  vtkPVKeyFrame();
  ~vtkPVKeyFrame();

  virtual void ChildCreate(vtkKWApplication* app);
  
  vtkSMKeyFrameProxy* KeyFrameProxy;
  char* KeyFrameProxyName;
  char* KeyFrameProxyXMLName; // must be set by subclasses before call to create.
  vtkSetStringMacro(KeyFrameProxyName);
  vtkSetStringMacro(KeyFrameProxyXMLName);

  vtkKWLabel* TimeLabel;
  vtkKWThumbWheel* TimeThumbWheel;

  vtkKWLabel* ValueLabel;
  vtkKWWidget* ValueWidget; // the type of this widget will be decided at runtime.
  vtkKWPushButton* MinButton;
  vtkKWPushButton* MaxButton;
 
  vtkPVAnimationCue* AnimationCue;
  char* Name;
//BTX
  friend class vtkPVKeyFrameObserver;
  vtkPVKeyFrameObserver* Observer;
  virtual void ExecuteEvent(vtkObject* , unsigned long event, void*);
//ETX
  void CreateValueWidget();  

  // Description:
  // Update the values from the vtkSMKeyFrameProxy.
  virtual void UpdateValuesFromProxy();

  double GetRelativeTime(double ntime);
  double GetNormalizedTime(double rtime);

  double TimeBounds[2];
private:
  vtkPVKeyFrame(const vtkPVKeyFrame&); // Not implemented.
  void operator=(const vtkPVKeyFrame&); // Not implemented.
};


#endif

