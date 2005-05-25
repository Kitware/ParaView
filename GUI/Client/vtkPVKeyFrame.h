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

#include "vtkPVTracedWidget.h"
class vtkSMKeyFrameProxy;
class vtkKWThumbWheel;
class vtkPVWidget;
class vtkKWLabel;
class vtkKWPushButton;
class vtkPVKeyFrameObserver;
class vtkSMAnimationCueProxy;
class vtkSMProperty;

class VTK_EXPORT vtkPVKeyFrame : public vtkPVTracedWidget
{
public:
  vtkTypeRevisionMacro(vtkPVKeyFrame, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);

  vtkGetObjectMacro(KeyFrameProxy, vtkSMKeyFrameProxy);

  // Description:
  // Get/Set the key time: the time for this key frame.
  void SetKeyTime(double time);
  double GetKeyTime();

  void SetKeyValue(double value) { this->SetKeyValue(0, value); }
  void SetKeyValue(int index, double value);
  double GetKeyValue() { return this->GetKeyValue(0); }
  double GetKeyValue(int index);

  // Description:
  // Set the number of key values for this keyt frame.
  void SetNumberOfKeyValues(int num);

  // Description:
  // Initialized Key Value using current animated property value.
  virtual void InitializeKeyValueUsingCurrentState();

  // Description:
  // Initilizes the Key Value using the property element at given index.
  virtual void InitializeKeyValueUsingProperty(vtkSMProperty* property, int index);
 
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

 
  // Description:
  // These methods set the current Key value to min or max if
  // they exist. Otherwise the value remains unchanged.
  void SetValueToMinimum();
  void SetValueToMaximum();

  virtual void PrepareForDisplay();
 
  // Description:
  // A pointer to the AnimationCueProxy is need so that the keyframe
  // can obtain information about the animated property. Must be set before
  // calling create and should not be changed thereafter. Note that it is not 
  // reference counted for fear of loops.
  void SetAnimationCueProxy(vtkSMAnimationCueProxy* cueproxy) 
    { this->AnimationCueProxy = cueproxy; }
  vtkGetObjectMacro(AnimationCueProxy, vtkSMAnimationCueProxy);

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

  vtkSMAnimationCueProxy* AnimationCueProxy;
  char* Name;
//BTX
  friend class vtkPVKeyFrameObserver;
  vtkPVKeyFrameObserver* Observer;
  virtual void ExecuteEvent(vtkObject* , unsigned long event, void*);
//ETX

  // Description:
  // This methods creates the widget to display the keyframe value.
  // It can be of 3 types: vtkPVSelectionList, vtkKWCheckButton, vtkKWThumbWheel.
  // Which to create depends on the domain of the animated property.
  // This merely creates the widget.
  void CreateValueWidget();  

  // Description:
  // This method updates the domain for the key frame value using the current domain
  // for the animated property.
  void UpdateDomain();
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

