/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationManager
// .SECTION Description
// vtkPVAnimationManager manages the Animation system.
// Here, we describe the working of animation in ParaView.
// Support for Animation in ParaView is split into three layers.
// 1) Support in VTK.
//    VTK provides support for Animation with vtkAnimationCue and vtkAnimationScene.
//    A Cue (or vtkAnimationCue) is a entity that is animated over time. The Cue has no
//    knowledge of what is being animated or how. All it knows is what are the start and 
//    end times for which the animated entity is animated. These times can be relative or
//    normalized (based on its Time mode). 
//    A Scene (or vtkAnimationScene) is the animation setup. One can add several cues to a
//    scene. Scene provides for playing and stopping the animation. In play, the clock time
//    is periodically incremented (depending upon the play mode of the scene) and reported to
//    each constituent cue. The Cue then decides if the current clock time is valid for that
//    particular cue and fires StartAnimationCueEvent, EndAnimationCueEvent and AnimationCueTickEvent
//    events accordingly.
// 2) Support in ServerManager.
//    ParaView support animation first at the ServerManager level. There are proxies for 
//    cue and scene (vtkSMAnimationCueProxy and vtkSMAnimationSceneProxy). However,
//    unlike most other proxies, these are client side proxies i.e. they don't create any
//    objects on any servers and hence never use ClientServerStreams for any communication.
//    vtkSMAnimationCueProxy can have a Manipulator associated with it. A manipulator is
//    a vtkSMAnimationCueManipulatorProxy derrived class which know how the animated entity 
//    it to be changed. On every tick event that the vtkSMAnimationCueProxy receives from the
//    corresponding vtkAnimationCue, the proxy checks if has a Manipulator object, and if so
//    calls UpdateValue() on the Manipulator. A concrete manipulator overrides this method to
//    use the current time to perform some change (animation) in the visualization.
//    vtkSMKeyFrameAnimationCueManipulatorProxy is a special manipulator that manages
//    key frames (vtkSMKeyFrameProxy derrived class). A keyframe is associated with a time 
//    (key time) and a value (key value). The key frame is responsible to performing the interpolation
//    of the value from the start of the key frame (i.e. the key time) to the next consecutive
//    key frame maintained by the KeyFrameManipulator. There are different types of key frame
//    depending upon the nature of interpolation eg. linear, exponential, sinusoidal.
// 3) Support in the GUI.
//    vtkPVAnimationManager forms the central point that brings togther the GUI support for animation.
//    The GUI supports creation/modification of cues with Key frame manipulators alone. Also,
//    the Scene start time is 0 and end time is the duration of the animation. Also, all cues
//    added to the Scene have normalized times and have start times 0 and end time 1 irrespective of
//    when the first key frame starts (or last key frame ends). 
//    GUI has two parts, the Vertical interface and the Horizontal Interface. The former 
//    shows the scene properties, selected key frame properties while the later shows the  
//    GUI to add/modify keyframes. 

#ifndef __vtkPVAnimationManager_h
#define __vtkPVAnimationManager_h

#include "vtkKWWidget.h"

class vtkPVVerticalAnimationInterface;
class vtkPVHorizontalAnimationInterface;
class vtkPVAnimationScene;
class vtkSMProxyIterator;
class vtkPVAnimationManagerInternals;
class vtkSMProxy;
class vtkPVAnimationCueTree;
class vtkPVAnimationManagerObserver;
class vtkPVAnimationCue;
class vtkPVSource;
class vtkPVKeyFrame;
class vtkSMStringVectorProperty;
class vtkKWToolbar;
class vtkKWPushButton;

class VTK_EXPORT vtkPVAnimationManager : public vtkKWWidget
{
public:
  static vtkPVAnimationManager* New();
  vtkTypeRevisionMacro(vtkPVAnimationManager, vtkKWWidget);
  void PrintSelf(ostream& os ,vtkIndent indent);

  virtual void Create(vtkKWApplication* app, const char* args);
  
  // Description:
  // Set the parent frames for the vertical and horizontal animation guis.
  void SetVerticalParent(vtkKWWidget* parent);
  void SetHorizantalParent(vtkKWWidget* parent);

  // Get the Vertical and Horizontal animation interface objects.
  vtkGetObjectMacro(VAnimationInterface, vtkPVVerticalAnimationInterface);
  vtkGetObjectMacro(HAnimationInterface, vtkPVHorizontalAnimationInterface);

  // Description:
  // Pack both the animation interfaces.
  void ShowAnimationInterfaces();
  void ShowVAnimationInterface();
  void ShowHAnimationInterface();

  // Description:
  // Iterates over the animatable proxies registered with the Proxy Manager
  // and updates the gui. If new proxies have been added, cue are added for those,
  // and old once have been removed, cue are removed.
  void Update();

  // Description:
  // Get the animation scene object which can be used to play/stop the animation.
  vtkGetObjectMacro(AnimationScene, vtkPVAnimationScene);

  // Description:
  // Returns is the animation is currently being played.
  int GetInPlay();

  // Description:
  // Time Marker is the vertical line over the time lines. This method sets
  // the time marker for all the timelines in the Horizontal Interface. 
  // The argument is normalized time which is 0 at the start of the scene (which
  // is same as the start of all the timelines) and 1 at the end of the scene (or 
  // end of each of the timelines).
  void SetTimeMarker(double normalized_time);

  void SaveAnimation();
  void SaveGeometry();

  //BTX
  // Description:
  // These are different types of KeyFrames recognized by the Manager.
  enum {
    RAMP = 0,
    STEP,
    EXPONENTIAL,
    SINUSOID,
    LAST_NOT_USED
  };
  //ETX

  // Description:
  // Creates a new key frame of the sepecified type and adds it to the cue.
  // If replaceFrame is specified, the new key frame replaces that frame in the cue.
  // Basic properties from replaceFrame are copied over to the newly created frame.
  vtkPVKeyFrame* ReplaceKeyFrame(vtkPVAnimationCue* pvCue, int type, 
    vtkPVKeyFrame* replaceFrame = NULL);


  // Description:
  // Returns a new Key frame of the specified type. Note that this method does not 
  // "Create" the key frame (by calling Create).
  vtkPVKeyFrame* NewKeyFrame(int type);

  // Description:
  // Returns the type of the key frame.
  int GetKeyFrameType(vtkPVKeyFrame* kf);

  // Description:
  // Save the animation in batch script.
  virtual void SaveInBatchScript(ofstream* file);

  // Description:
  // Save the state of the animation interface, so that the 
  // animation it can be restored at a later point.
  void SaveState(ofstream* file);

  // Description:
  // Get/Set if recording records only those cue's that have focus or
  // all of them. If not set, then only the changes in teh property that 
  // has the focus are key framed.
  vtkSetMacro(RecordAll, int);
  vtkGetMacro(RecordAll, int);
 
  // Description:
  // This methods runs over all the animatable properties (property cues)
  // and notes their current values. Call to KeyFrameChanges compares
  // the property values to these noted values to generate key frames.
  void InitializeAnimatedPropertyStatus();

  // Description:
  // Changes in animatable properties since the last call to 
  // InitializeAnimatedPropertyStatus are recorded as a keyframes.
  void KeyFramePropertyChanges();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Sets the animation time. Changes the system state to reflect the one
  // at the point in time. Time is normalized Scene time.
  void SetCurrentTime(double ntime);

  // Description:
  // Indicates if the Geometry cache is to be used. Cached geometry can only be 
  // used when the Scene play mode is Sequence.
  int GetUseGeometryCache();

  // Description:
  // Can be called to set up a default animation for the given type of
  // pvSource. Presently, a default animation is added to only to a
  // reader with multiple timesteps.
  void AddDefaultAnimation(vtkPVSource* pvSource);

  virtual void ValidateAndAddSpecialCues();

  // Description:
  // Save/Restore window geometry
  virtual void SaveWindowGeometry();
  virtual void RestoreWindowGeometry();

protected:
  vtkPVAnimationManager();
  ~vtkPVAnimationManager();

  int RecordAll;
  vtkKWWidget* VerticalParent;
  vtkKWWidget* HorizantalParent;

  vtkSMProxyIterator* ProxyIterator;

  vtkPVVerticalAnimationInterface* VAnimationInterface;
  vtkPVHorizontalAnimationInterface* HAnimationInterface;
  vtkPVAnimationScene* AnimationScene;

  vtkPVAnimationManagerInternals* Internals;

  
  // Description:
  // NOTE: these methods allocated memory. It is up to the caller to delete it.
  char* GetSourceListName(const char* proxyname);
  char* GetSourceName(const char* proxyname);
  char* GetSubSourceName(const char* proxyname);
  char* GetSourceKey(const char* proxyname);

  // Description:
  // Iterates over properties of the proxy and add animation cues.
  int AddProperties(vtkPVSource* pvSource, vtkSMProxy* proxy, 
    vtkPVAnimationCueTree* pvCueTree);

  // Description:
  // Checks to see if any of the animation cues points to a deleted PVSource.
  // Such cues are removed.
  void ValidateOldSources();

  // Description:
  // Runs over the proxies registered as "animateable" with the ProxyManager and
  // checks to see if they are added to the Animation interace. If not, they are 
  // added to the interface.
  void AddNewSources();

//BTX
  friend class vtkPVAnimationManagerObserver;
  vtkPVAnimationManagerObserver* Observer;
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* calldata);
//ETX
  void SetupCue(vtkPVSource* pvSource, vtkPVAnimationCueTree* parent, vtkSMProxy* proxy, 
    const char* propertyname, const char* domainname, int element, 
    const char* label);

  int AddStringVectorProperty(vtkPVSource* pvSource, vtkSMProxy* proxy, 
    vtkPVAnimationCueTree* pvCueTree, vtkSMStringVectorProperty* svp);
  
  void InitializeObservers(vtkPVAnimationCue* cue);

  vtkPVAnimationCueTree* GetAnimationCueTreeForSource(vtkPVSource* pvSource);

  vtkKWToolbar* KeyFramesToolbar;
  vtkKWPushButton* InitStateButton;
  vtkKWPushButton* AddKeyFramesButton;
private:
  vtkPVAnimationManager(const vtkPVAnimationManager&); // Not implemented.
  void operator=(const vtkPVAnimationManager&); // Not implemented.
};

#endif

