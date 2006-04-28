/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSimpleAnimationCue.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSimpleAnimationCue - minimal GUI for vtkSMAnimationCueProxy
// .SECTION Description
// This class provides the minimalistic GUI stuff for the
// vtkSMAnimationCueProxy.  The subclasses can provide the GUI for the cue
// eg. animation tracks or otherwise.  This class provides methods to
// manage vtkSMAnimationCueProxy and
// vtkSMKeyFrameAnimationCueManipulatorProxy and the keyframes associated
// with it.

#ifndef __vtkPVSimpleAnimationCue_h
#define __vtkPVSimpleAnimationCue_h

#include "vtkPVTracedWidget.h"
class vtkPVSimpleAnimationCueObserver;
class vtkCollectionIterator;
class vtkCollection;
class vtkSMAnimationCueProxy;
class vtkSMKeyFrameAnimationCueManipulatorProxy;
class vtkPVKeyFrame;
class vtkSMProxy;
class vtkSMProperty;
class vtkSMPropertyStatusManager;

class VTK_EXPORT vtkPVSimpleAnimationCue : public vtkPVTracedWidget
{
public:
  static vtkPVSimpleAnimationCue* New();
  vtkTypeRevisionMacro(vtkPVSimpleAnimationCue, vtkPVTracedWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Virtual indicates if this cue is a actual cue, which has a proxy
  // associated with it or merely a grouping GUI element.
  vtkGetMacro(Virtual, int);

  // Description:
  // Returns a readable text for the cue. Note that memory is 
  // allocated, so the caller must clean it up.
  virtual char* GetTextRepresentation();

  // Description:
  // Pointer to the parent animation cue , if any.  Note that parent is not
  // reference counted. This is needed to build a text representation for
  // the cue (as returned by GetTextRepresentation())
  void SetParentAnimationCue(vtkPVSimpleAnimationCue* cue)
    { this->ParentCue = cue; }

  // Description:
  // Label Text is the label for this cue.
  vtkSetStringMacro(LabelText);
  vtkGetStringMacro(LabelText);

  // Description:
  // Get the MTime of the Keyframes.
  unsigned long GetKeyFramesMTime();

  // Description:
  // Get the number of key frames in this cue.
  int GetNumberOfKeyFrames();

  // Description:
  // Returns the time for the keyframe at the given id.  Time is normalized
  // to the span of the cue [0,1].
  double GetKeyFrameTime(int id);

  // Description:
  // Change the keyframe time for a keyframe at the given id.
  // Time is normalized to the span of the cue [0,1].
  void SetKeyFrameTime(int id, double time);

  // Description:
  // Add a new key frame to the cue at the given time. If this cue is
  // Virtual, this can add upto two keyframes. If the cue is Non-Virtual,
  // it creates a key frame of the type vtkPVAnimationManager::RAMP and
  // adds it to the cue at the specified time.  NOTE: It does not verify is
  // a key frame already exists at the same time. Time is normalized to the
  // span of the cue [0,1].
  int AddNewKeyFrame(double time);

  // Description:
  // Creates a new key frame of the specified type and add it to the cue at
  // the given time.  Time is normalized to the span of the cue [0,1]. This
  // method also does not verify is a key frame already exists at the
  // specified time.
  virtual int CreateAndAddKeyFrame(double time, int type);

  // Description:
  // Determine a time to append a new keyframe (the old keyframes in this
  // cue may get shrunk to accomadate the new keyframe) and calls
  // AddNewKeyFrame.
  int AppendNewKeyFrame();
 
  // Description:
  // Remove All Key frames from this cue.
  virtual void RemoveAllKeyFrames();

  // Description:
  // Removes a particular key frame from the cue.
  // This method merely removes the keyframe. It does not
  // lead to changing of the selection on the timeline and raising of
  // appriate events. For all  that to happen one must use 
  // DeleteKeyFrame.
  void RemoveKeyFrame(vtkPVKeyFrame* keyframe);

  // Description:
  // Removes a keyframe at the given id from the cue.
  // This method merely removes the keyframe. It does not
  // lead to changing of the selection on the timeline and raising of
  // appriate events. For all  that to happen one must use 
  // DeleteKeyFrame.
  int RemoveKeyFrame(int id);

  // Description:
  // Deletes the keyframe at given index. If the deleted key frame is the
  // currenly selected keyframe, it changes the selection and the timeline is
  // updated.
  void DeleteKeyFrame(int id);

  // Description:
  // Returns true if the selected keyframe can be deleted.
  int CanDeleteKeyFrame(int index);

  // Description:
  // Returns true if the selected keyframe can be deleted.
  int CanDeleteSelectedKeyFrame();

  // Description:
  // Returns a key frame at the given id in the cue.
  vtkPVKeyFrame* GetKeyFrame(int id);

  // Description:
  // Returns a key frame with the givenn name. This is only for trace
  // and should never be used otherwise.
  // OBSOLETE: trace no longer replies on keyframe names. Instead 
  // it relies on selection of the appropriate keyframe.
  vtkPVKeyFrame* GetKeyFrame(const char* name);

  // Description:
  // Returns the currently selected key frame (as indicated by 
  // SelectedKeyFrameIndex), if any, otherwise NULL.
  vtkPVKeyFrame* GetSelectedKeyFrame();

  // Description:
  // Replaces a keyframe with another. The Key time and key value of
  // the oldFrame and copied over to the newFrame;
  virtual void ReplaceKeyFrame(vtkPVKeyFrame* oldFrame, vtkPVKeyFrame* newFrame);

  // Description:
  // Method to query if the animation cue supports the given type of
  // key frame. Default implementatio returns true for all but
  // Camera keyframes.
  virtual int IsKeyFrameTypeSupported(int type);
  
  // Description:
  // Methods to set the animated proxy/property/domain/element information.
  virtual void SetAnimatedProxy(vtkSMProxy* proxy);
  vtkSMProxy* GetAnimatedProxy();
  virtual void SetAnimatedPropertyName(const char* name);
  const char* GetAnimatedPropertyName();
  void SetAnimatedDomainName(const char* name);
  const char* GetAnimatedDomainName();
  void SetAnimatedElement(int index);
  int GetAnimatedElement();

  // Description:
  // Start Recording. Once recording has been started new key frames cannot
  // be added directly.
  virtual void StartRecording();

  // Description:
  // Stop Recording.
  virtual void StopRecording();

  virtual void RecordState(double ntime, double offset);

  // Description:
  // Get the animation cue proxy associated with this cue. If this cue is
  // Virtual, this method returns NULL.
  vtkGetObjectMacro(CueProxy, vtkSMAnimationCueProxy);

  // Description:
  // Set the animation cue proxy controlled by this cue. If
  // this GUI already had a Cue proxy associated with it which it had 
  // registered with the vtkSMProxyManager, this call unregisters the 
  // old proxy and registers the new one. If the old proxy had keyframes
  // in it which had GUI associated with it, then, the keyframe GUI is
  // also destroyed. If the new cue proxy doesn't have a manipulator associated
  // with it, a new vtkSMKeyFrameAnimationCueManipulatorProxy will be created
  // and set as the manipulator for the cueProxy.
  // If this cue is Virtual, this method has no effect.
  // Can be called before Create is called in which case this class
  // does not create the proxies.
  void SetCueProxy(vtkSMAnimationCueProxy* cueProxy);

  // Description:
  // Sets up the keyframe state (key value/ value bounds etc). using the current state of 
  // of the property.
  void InitializeKeyFrameUsingCurrentState(vtkPVKeyFrame* keyframe);

  virtual void UpdateEnableState();

  // Description:
  // This will select the keyframe. Fires a SelectionChangedEvent.
  virtual void SelectKeyFrame(int id);

  // Description:
  // Get the selected key frame index. -1, when none is selected.
  vtkGetMacro(SelectedKeyFrameIndex, int);
//BTX
  // Event saying that the Keyframes managed by this cue have changed.
  // In non-virtual mode, this is triggered when the KeyFrameManipulatorProxy
  // is modified. In Virtual mode, since there is no KeyFrameManipulatorProxy,
  // this class itself triggers this event when it modifies the end time points.
  enum {
    KeysModifiedEvent = 3001,
    SelectionChangedEvent
  };
//ETX

  // Description:
  // Set the timeline parameter bounds. 
  // This class provides a crude, inefficient implementation when PVTimeLine
  // object is not available. vtkPVAnimationCue overrides this method to provide
  // a better implementation.
  virtual void SetTimeBounds(double bounds[2], int enable_scaling=0);
  virtual int GetTimeBounds(double* bounds);

  // Description:
  // This is the parent frame which will contain the Keyframes.
  // Typically, this is an instance of vtkPVTrackEditor.
  void SetKeyFrameParent(vtkKWWidget* kfParent);
  vtkGetObjectMacro(KeyFrameParent, vtkKWWidget);

  // Description:
  // Forwarded to all created key frames.
  void SetDuration(double duration);
  vtkGetMacro(Duration, double);


  // Description:
  // Get/Set the default key frame type created by this Cue.
  vtkGetMacro(DefaultKeyFrameType, int);
  vtkSetMacro(DefaultKeyFrameType, int);

  //BTX
  // Description:
  // These are different types of KeyFrames.
  enum {
    RAMP = 0,
    STEP,
    EXPONENTIAL,
    SINUSOID,
    CAMERA,
    LAST_NOT_USED
  };
  //ETX

  // Description:
  // Creates a new key frame of the sepecified type and adds it to the cue.
  // If replaceFrame is specified, the new key frame replaces that frame in
  // the cue.  Basic properties from replaceFrame are copied over to the
  // newly created frame.
  vtkPVKeyFrame* ReplaceKeyFrame(int type, vtkPVKeyFrame* replaceFrame = NULL);

  // Description:
  // Returns a new Key frame of the specified type. Note that this method
  // does not "Create" the key frame (by calling Create), it merely
  // instantiates the right kind of vtkPVKeyFrame subclass.
  vtkPVKeyFrame* NewKeyFrame(int type);

  // Description:
  // Returns the type of the key frame.
  int GetKeyFrameType(vtkPVKeyFrame* kf);
  int GetKeyFrameType(vtkSMProxy* kf);

protected:
  vtkPVSimpleAnimationCue();
  ~vtkPVSimpleAnimationCue();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  virtual void SelectKeyFrameInternal(int id);
  // Description:
  // Creates the proxies for the Cue.
  virtual void CreateProxy();

  // Description:
  // Internal method to clean up keyframes GUI.
  void CleanupKeyFramesGUI();

  // Description:
  // Using the proxies for keyframes, creates the corresponding GUI.
  void InitializeGUIFromProxy();


  // Description:
  // Initantiates a new vtkPVKeyFrame subclass for the given type and
  // sets it's parent etc. Does not call Create on the object though.
  vtkPVKeyFrame* CreateNewKeyFrameAndInit(int type);

  // Description:
  // Set if the Cue is virtual i.e. it has no proxies associated with it,
  // instead is a dummy cue used as a container for other cues.  NOTE: this
  // property must not be changed after Create.
  vtkSetMacro(Virtual, int);

  // Description:
  // Internal method to add a new keyframe.
  int AddKeyFrame(vtkPVKeyFrame* keyframe);


  vtkKWWidget* KeyFrameParent;
  vtkCollection* PVKeyFrames;
  vtkCollectionIterator* PVKeyFramesIterator;

  vtkSMPropertyStatusManager* PropertyStatusManager;
  vtkPVSimpleAnimationCue* ParentCue;

  int Virtual;
  int NumberOfPoints;
  double PointParameters[2];
  vtkSMAnimationCueProxy* CueProxy;
  char* CueProxyName;
  vtkSetStringMacro(CueProxyName);

  vtkSMKeyFrameAnimationCueManipulatorProxy* KeyFrameManipulatorProxy;
  void SetKeyFrameManipulatorProxy(vtkSMKeyFrameAnimationCueManipulatorProxy*);

  // Description:
  // Obtains the Manip. proxy from the CueProxy. If CueProxy doesn't have a manip.
  // a new one is created using the KeyFrameManipulatorProxyXMLName.
  void SetupManipulatorProxy();
  
  char* KeyFrameManipulatorProxyName;
  vtkSetStringMacro(KeyFrameManipulatorProxyName);
  char* KeyFrameManipulatorProxyXMLName;
  vtkSetStringMacro(KeyFrameManipulatorProxyXMLName);

  char* LabelText;
  int ProxiesRegistered;

  int InRecording;
  int SelectedKeyFrameIndex;

  // Description:
  // This variable indicates if a keyframe was added in the previous call to
  // RecordState
  int PreviousStepKeyFrameAdded;

  // Description:
  // Keyframes assigned unique names. The names are dependent on the 
  // order for the cue in which they are created. KeyFramesCreatedCount
  // keeps track of the order.
  int KeyFramesCreatedCount;

  // Description:
  // The type of the keyframe created by default.
  int DefaultKeyFrameType;

  // Description:
  // A PVCue registers the proxies and adds it to the AnimationScene iff it
  // has atleast two keyframes and it is not virtual. Whenever this
  // criteria is not met, it is unregistered and removed form the
  // AnimationScene.  This ensures that SMState and BatchScript will have
  // only those cue proxies which actually constitute any animation.
  virtual void RegisterProxies();
  virtual void UnregisterProxies(); 
//BTX
  friend class vtkPVSimpleAnimationCueObserver;
  vtkPVSimpleAnimationCueObserver* Observer;
  void Observe(vtkObject* toObserver, unsigned long event);
  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void*data);
//ETX

  double Duration;

private:
  vtkPVSimpleAnimationCue(const vtkPVSimpleAnimationCue&); // Not implemented.
  void operator=(const vtkPVSimpleAnimationCue&); // Not implemented.
};


#endif
