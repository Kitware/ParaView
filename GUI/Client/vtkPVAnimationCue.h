/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCue.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationCue - the GUI for an Animation Cue.
// .SECTION Description
// vtkPVAnimationCue is manages the GUI for an Animation Cue. 
// The GUI has two sections, the navgation interface: which shows the label
// of the cue and the timeline: which is used to modify keyframes. ParaView
// puts these two sections in two panes of a split frame so that the label 
// length does not reduce the visible area for the timelines. The parent of an
// object of this class acts as the parent for the Navigation section,
// while the TimeLineParent is the parent for the timeline. Both of which need
// to be set before calling create.
// This class has Virtual mode. In this mode, no proxies are created for this class.
// This mode is used by the Subclass vtkPVAnimationCueTree which represents a GUI
// element which has child cues eg. the cue for the PVSource or for a property
// with multiple elements. Thus, Virtual cue is used merely to group the 
// chidlren. The support for adding children and managing them is provided by the
// subclass vtkPVAnimationCueTree.
//
// .SECTION See Also
// vtkPVAnimationCueTree vtkSMAnimationCueProxy

#ifndef __vtkPVAnimationCue_h
#define __vtkPVAnimationCue_h

#include "vtkKWWidget.h"
class vtkKWWidget;
class vtkKWLabel;
class vtkPVTimeLine;
class vtkKWFrame;
class vtkPVAnimationCueObserver;
class vtkSMAnimationCueProxy;
class vtkSMKeyFrameAnimationCueManipulatorProxy;
class vtkPVKeyFrame;
class vtkCollection;
class vtkCollectionIterator;
class vtkPVAnimationScene;
class vtkPVSource;
class vtkSMPropertyStatusManager;
class vtkSMProxy;

class VTK_EXPORT vtkPVAnimationCue : public vtkKWWidget
{
public:
  static vtkPVAnimationCue* New();
  vtkTypeRevisionMacro(vtkPVAnimationCue, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);

  // Description:
  // TimeLineParent is the frame that contains the timelines.
  // this->Parent is the frame which contains the labels (or the Navgation
  // widget) for the cue.
  void SetTimeLineParent(vtkKWWidget* frame);

  // Description:
  // Label Text is the text shown for this cue.
  void SetLabelText(const char* label);
  const char* GetLabelText();

  // Description:
  // Get the timeline object.
  vtkGetObjectMacro(TimeLine, vtkPVTimeLine);

  virtual void PackWidget();
  virtual void UnpackWidget();

  // Description:
  // Set the timeline parameter bounds. This moves the timeline end points.
  // Depending upon is enable_scaling is set, the internal nodes
  // are scaled.
  virtual void SetTimeBounds(double bounds[2], int enable_scaling=0);
  virtual int GetTimeBounds(double* bounds);

  // Description:
  // Provides for highlighting of the selected cue.
  virtual void GetFocus();
  virtual void RemoveFocus();
  virtual int HasFocus() {return this->Focus;}

  // Description:
  // Virtual indicates if this cue is a actual cue, which has a proxy associated with it
  // or merely a grouping GUI element.
  vtkGetMacro(Virtual, int);

  // Description:
  // Get the MTime of the Keyframes.
  unsigned long GetKeyFramesMTime();

  // Description:
  // Get the number of key frames in this cue.
  int GetNumberOfKeyFrames();

  // Description:
  // Remove All Key frames from this cue.
  virtual void RemoveAllKeyFrames();

  // Description:
  // Returns the time for the keyframe at the given id.
  // Time is normalized to the span of the cue [0,1].
  double GetKeyFrameTime(int id);

  // Description:
  // Change the keyframe time for a keyframe at the given id.
  // Time is normalized to the span of the cue [0,1].
  void SetKeyFrameTime(int id, double time);

  // Description:
  // Add a new key frame to the cue at the given time. If this cue is Virtual, this
  // can add upto two keyframes. If the cue is Non-Virtual, it creates a key frame of
  // the type vtkPVAnimationManager::RAMP and adds it to the cue at the specified time.
  // NOTE: It does not verify is a key frame already exists at the same time. Time is
  // normalized to the span of the cue [0,1].
  int AddNewKeyFrame(double time);

  // Description:
  // Creates a new key frame of the specified type and add it to the cue at the given time.
  // Time is normalized to the span of the cue [0,1]. This method also does not verify is a 
  // key frame already exists at the specified time. 
  int CreateAndAddKeyFrame(double time, int type);

 
  // Description:
  // Removes a particular key frame from the cue.
  void RemoveKeyFrame(vtkPVKeyFrame* keyframe);

  // Description:
  // Removes a keyframe at the given id from the cue.
  int RemoveKeyFrame(int id);

  // Description:
  // Returns a key frame at the given id in the cue.
  vtkPVKeyFrame* GetKeyFrame(int id);

  // Description:
  // Returns a key frame with the givenn name. This is only for trace
  // and should never be used otherwise.
  vtkPVKeyFrame* GetKeyFrame(const char* name);

  // Description:
  // Replaces a keyframe with another. The Key time and key value of
  // the oldFrame and copied over to the newFrame;
  void ReplaceKeyFrame(vtkPVKeyFrame* oldFrame, vtkPVKeyFrame* newFrame);


  // Description:
  // Methods to set the animated proxy/property/domain/element information.
  void SetAnimatedProxy(vtkSMProxy* proxy);
  void SetAnimatedPropertyName(const char* name);
  const char* GetAnimatedPropertyName();

  void SetAnimatedDomainName(const char* name);
  void SetAnimatedElement(int index);

  // Description:
  // Start Recording. Once recording has been started new key frames cannot be added directly.
  virtual void StartRecording();

  // Description:
  // Stop Recording.
  virtual void StopRecording();

  virtual void RecordState(double ntime, double offset, int onlyFocus);

  // Description:
  // Adds a new key frame is the property animated by this cue has changed since last
  // call to InitializeStatus(). ntime is the time at which this key frame will be added.
  // If onlyFocus is 1, the new key frame is added only if this cue has the focus.
//  virtual void KeyFramePropertyChanges(double ntime, double offset, int onlyFocus);

  // Description:
  // Get the animation cue proxy associated with this cue. If this cue is Virtual, 
  // this method returns NULL.
  vtkGetObjectMacro(CueProxy, vtkSMAnimationCueProxy);

  // Description:
  // Set a pointer to the AnimationScene. This is not reference counted. A cue
  // adds itself to the scene when it has two or more key frames (i.e. the is 
  // animatable), and it removes itself from the Scene is the number of keyframes
  // reduces.
  void SetAnimationScene(vtkPVAnimationScene* scene);

  // Description:
  // Pointer to the PVSource that this cue stands for.
  // When the property is modifed (during animation), the cue calls
  // MarkSourcesForUpdate() on the PVSource so the the pipeline is
  // re-rendered.
  void SetPVSource(vtkPVSource*);
  vtkGetObjectMacro(PVSource, vtkPVSource);

  // Description:
  // Time marker is a vartical line used to indicate the current time.
  // This method sets the timemarker of the timeline for this cue alone.
  virtual void SetTimeMarker(double time);
  double GetTimeMarker();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  virtual void SaveState(ofstream* file);

  // Description:
  // Each cue is assigned a unique name. This name is used to indentify
  // the cue in trace/ state. Names are assigned my the vtkPVAnimationManager
  // while creating the PVAnimationCue. A child cue can be obtained from the
  // parent vtkPVAnimationCueTree using this name.
  // Note that this method does not ensure that the name is indeed unique.
  // It is responsibility of the vtkPVAnimationManager to set unique names
  // (atleast among the siblings) for the trace/ state to work properly. Also,
  // only vtkPVAnimationManager must set the name of the cue.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Sets up the keyframe state (key value/ value bounds etc). using the current state of 
  // of the property.
  void InitializeKeyFrameUsingCurrentState(vtkPVKeyFrame* keyframe);

  // Description:
  // Enable horizontal zooming of the timeline.
  void SetEnableZoom(int zoom);

  // Description:
  virtual void Zoom(double range[2]);
  void Zoom(double s, double e) 
    {
    double r[2]; 
    r[0]=s; r[1] = e;
    this->Zoom(r);
    }

  // Description:
  // The name of a cue depends on the PVSource's name. 
  // For trace to work reliably, the cue's name must be 
  // constructed on using the PVSource (not it's name).
  // This returns the tcl script/string that evaluates the
  // name correctly at runtime.
  const char* GetTclNameCommand();

  // Description:
  // Updates the visibility of the cue.
  // If the animated property is not "animateable", then it is
  // visible only in Advanced mode.
  virtual void UpdateCueVisibility(int advanced);
  vtkGetMacro(CueVisibility, int);

  // Description:
  // Detachs the cue. i.e. removes it from scene etc. and prepares it
  // to be deleted.
  virtual void Detach();

  // Description:
  // Pointer to the parent animation Cue tree, if any.
  // Note that parent is not reference counted.
  void SetParentAnimationCue(vtkPVAnimationCue*);
 
  // Description:
  // Returns a readable text for the cue. Note that memory is 
  // allocated, so the caller must clean it up.
  char* GetTextRepresentation();
protected:
  vtkPVAnimationCue();
  ~vtkPVAnimationCue();
//BTX
  // Event saying that the Keyframes managed by this cue have changed.
  // In non-virtual mode, this is triggered when the KeyFrameManipulatorProxy
  // is modified. In Virtual mode, since there is no KeyFrameManipulatorProxy,
  // this class itself triggers this event when it modifies the end time points.
  enum {
    KeysModifiedEvent = 2001
  };

  // Description:
  // Set/Get the type of the image shown to the left of the label 
  // in the Navigation interface. This is useful esp for simulating
  // the apperance of a tree.
  void SetImageType(int type);
  vtkGetMacro(ImageType, int);

  enum {
    NONE=0,
    IMAGE_OPEN,
    IMAGE_CLOSE
  };

  vtkPVAnimationCueObserver* Observer;
  friend class vtkPVAnimationCueObserver;
//ETX
 
  // Description:
  // Internal method to add a new keyframe.
  int AddKeyFrame(vtkPVKeyFrame* keyframe);

  // Description:
  // Set if the Cue is virtual i.e. it has no proxies associated with it, instead 
  // is a dummy cue used as a container for other cues.
  // NOTE: this property must not be changed after Create.
  void SetVirtual(int v);

  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);
  void InitializeObservers(vtkObject* object);
  
 
  vtkKWWidget* TimeLineParent;
  vtkPVSource* PVSource;

  vtkKWLabel* Label; 
  vtkKWLabel* Image;
  vtkKWFrame* Frame;

  vtkKWFrame* TimeLineContainer;
  vtkKWFrame* TimeLineFrame;
  vtkPVTimeLine* TimeLine;

  vtkSMPropertyStatusManager* PropertyStatusManager;
  int ImageType;
  int ShowTimeLine;

  char* Name;
  char* TclNameCommand;
  vtkSetStringMacro(TclNameCommand);

  int Focus;

  int Virtual;
  int NumberOfPoints;
  double PointParameters[2];
 
  vtkCollection* PVKeyFrames;
  vtkCollectionIterator* PVKeyFramesIterator;
  
  vtkSMAnimationCueProxy* CueProxy;
  char* CueProxyName;
  vtkSetStringMacro(CueProxyName);

  vtkSMKeyFrameAnimationCueManipulatorProxy* KeyFrameManipulatorProxy;
  char* KeyFrameManipulatorProxyName;
  vtkSetStringMacro(KeyFrameManipulatorProxyName);

  vtkPVAnimationScene* PVAnimationScene;
  void CreateProxy();
  // Description:
  // Internal methods to change focus state of this cue.
  void GetSelfFocus();
  void RemoveSelfFocus();

  // Description:
  // A PVCue registers the proxies and adds it to the AnimationScene iff it 
  // has atleast two keyframes and it is not virtual. Whenever this
  // criteria is not met, it is unregistered and removed form the AnimationScene.
  // This ensures that SMState and BatchScript will have only those cue proxies
  // which actually constitute any animation.
  void RegisterProxies();
  void UnregisterProxies();
  int ProxiesRegistered;
  int CueVisibility;
  int InRecording;

  // Description:
  // Keyframes assigned unique names. The names are dependent on the 
  // order for the cue in which they are created. KeyFramesCreatedCount
  // keeps track of the order.
  int KeyFramesCreatedCount;

  // Description:
  // This variable indicates if a keyframe was added in the previous call to
  // RecordState
  int PreviousStepKeyFrameAdded;
  
  vtkPVAnimationCue* ParentCue;
private:
  vtkPVAnimationCue(const vtkPVAnimationCue&); // Not implemented.
  void operator=(const vtkPVAnimationCue&); // Not implemented.
};

#endif


