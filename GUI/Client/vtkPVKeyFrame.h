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
class vtkPVAnimationScene;
class vtkPVKeyFrameObserver;
class vtkSMAnimationCueProxy;
class vtkSMProperty;

class VTK_EXPORT vtkPVKeyFrame : public vtkPVTracedWidget
{
public:
  vtkTypeRevisionMacro(vtkPVKeyFrame, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app);

  vtkGetObjectMacro(KeyFrameProxy, vtkSMKeyFrameProxy);

  // Description:
  // Get/Set the key time: the time for this key frame.
  void SetKeyTime(double time);
  double GetKeyTime();

  // Description:
  // Calls SetKeyTime() and adds it to the trace.
  void SetKeyTimeWithTrace(double time);

  // Description:
  // Initialized Key Value using current animated property value.
  virtual void InitializeKeyValueUsingCurrentState() = 0;

  // Description:
  // Initialize the Key Value bounds using current animatied property value
  // and domain state.
  virtual void InitializeKeyValueDomainUsingCurrentState() =0;

  // Description:
  // If set, these ranges can be used to bound the values.
  // Timebounds are in normalized time.
  void SetTimeMinimumBound(double min);
  void SetTimeMaximumBound(double max);
  void ClearTimeBounds();
 
  // Description:
  // Callbacks for GUI
  void TimeChangedCallback();
 
  // Description:
  // Prepares the Key frame GUI for display.
  // Typically, updates the GUI to reflect the state of the 
  // underlying proxy/property.
  virtual void PrepareForDisplay();
 
  // Description:
  // A pointer to the AnimationCueProxy is need so that the keyframe can
  // obtain information about the animated property. Must be set before
  // calling create and should not be changed thereafter. Note that it is
  // not reference counted for fear of loops.
  void SetAnimationCueProxy(vtkSMAnimationCueProxy* cueproxy) 
    { this->AnimationCueProxy = cueproxy; }
  vtkGetObjectMacro(AnimationCueProxy, vtkSMAnimationCueProxy);

  // Description:
  // These methods set the current Key value to min or max if
  // they exist. Otherwise the value remains unchanged.
  virtual void SetValueToMinimum() = 0;
  virtual void SetValueToMaximum() = 0;
  
  // Description:
  // Save state of the GUI.
  virtual void SaveState(ofstream* file);

  // Description:
  // Propagate widget enable state.
  virtual void UpdateEnableState();

  // Description:
  // Name is used for trace alone. Do not use this directly.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Used to obtain duration. If not set the Duration ivar will be used
  // in normalizing time.
  void SetAnimationScene(vtkPVAnimationScene* scene);
  vtkGetObjectMacro(AnimationScene, vtkPVAnimationScene);

  // Description:
  // If AnimationScene is not set, Duration is used to normalize time.
  void SetDuration(double duration);
  vtkGetMacro(Duration, double);

  // Description:
  // Copies the values from argument.
  // If the two differ in type, only corresponding properies are copied over;
  virtual void Copy(vtkPVKeyFrame* fromKF);
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

  vtkSMAnimationCueProxy* AnimationCueProxy;
  char* Name;
//BTX
  friend class vtkPVKeyFrameObserver;
  vtkPVKeyFrameObserver* Observer;
  virtual void ExecuteEvent(vtkObject* , unsigned long event, void*);
//ETX

  // Description:
  // Update the values from the vtkSMKeyFrameProxy.
  virtual void UpdateValuesFromProxy();

  double GetRelativeTime(double ntime);
  double GetNormalizedTime(double rtime);

  double TimeBounds[2];
  double Duration;
  vtkPVAnimationScene* AnimationScene;

private:
  vtkPVKeyFrame(const vtkPVKeyFrame&); // Not implemented.
  void operator=(const vtkPVKeyFrame&); // Not implemented.
};


#endif

