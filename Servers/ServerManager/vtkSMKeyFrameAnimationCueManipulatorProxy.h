/*=========================================================================

  Program:   ParaView
  Module:    vtkSMKeyFrameAnimationCueManipulatorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMKeyFrameAnimationCueManipulatorProxy
// .SECTION Description
// Base class for manipulators that support key framing.
// Key frames are stored in a vector ordered by their keyframe time. Ordering
// of keyframes with same key time is arbritary.
// 
// .SECTION See Also
// vtkSMAnimationCueProxy vtkSMAnimationCueManipulatorProxy

#ifndef __vtkSMKeyFrameAnimationCueManipulatorProxy_h
#define __vtkSMKeyFrameAnimationCueManipulatorProxy_h

#include "vtkSMAnimationCueManipulatorProxy.h"

class vtkSMKeyFrameProxy;
class vtkSMKeyFrameAnimationCueManipulatorProxyInternals;
class vtkSMKeyFrameAnimationCueManipulatorProxyObserver;

class VTK_EXPORT vtkSMKeyFrameAnimationCueManipulatorProxy : 
  public vtkSMAnimationCueManipulatorProxy
{
public:
  vtkTypeRevisionMacro(vtkSMKeyFrameAnimationCueManipulatorProxy, 
    vtkSMAnimationCueManipulatorProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMKeyFrameAnimationCueManipulatorProxy* New();
  
  virtual void SaveInBatchScript(ofstream* file);

  // Description:
  // Add a key frame. Key frames are stored in a map, keyed by the
  // KeyFrameTime. If two keyframes have the same
  // key time, only one of then will be considered. It returns the index
  // of the added frame in the collection.
  int AddKeyFrame (vtkSMKeyFrameProxy* keyframe);

  // Description:
  // Removes a key frame at the specified time, if any. 
  void RemoveKeyFrame(vtkSMKeyFrameProxy* keyframe);
 
  // Description:
  // Removes all key frames, if any.
  void RemoveAllKeyFrames();

  // Description:
  // Returns a pointer to the key frame at the given time.
  // If no key frame exists at the given time, it returns NULL.
  vtkSMKeyFrameProxy* GetKeyFrame(double time);

  // Description:
  // Given the current time, determine the key frames
  // between which the current time lies. 
  // Returns the key frame time.
  // If the current time 
  // coincides with a key frame, both methods (GetStartKeyFrameTime
  // and GetEndKeyFrameTime) return that key keyframes time which is 
  // same as time. If the current time is before the first key frame
  // or after the last key frame, then this method return -1.
  vtkSMKeyFrameProxy* GetStartKeyFrame(double time);
  vtkSMKeyFrameProxy* GetEndKeyFrame(double time);

  // Description:
  // Get the next/previous key frame relative to argument key frame.
  // Returns NULL when no next/previous frame exists.
  vtkSMKeyFrameProxy* GetNextKeyFrame(vtkSMKeyFrameProxy* keyFrame);
  vtkSMKeyFrameProxy* GetPreviousKeyFrame(vtkSMKeyFrameProxy* keyFrame);

  // Description:
  // Get the number of keyframes.
  unsigned int GetNumberOfKeyFrames();

  // Description:
  // Access the keyframe collection using the indices.
  // Keyframes are sorted according to increasing key frame time.
  vtkSMKeyFrameProxy* GetKeyFrameAtIndex(int index);
//BTX
  // Description:
  // StateModifiedEvent - This event is fired when the manipulator modifies the animated proxy.
  // vtkCommand::Modified - is fired when the keyframes are changed i.e. added/removed/modified.
  enum
    {
    StateModifiedEvent = 2000,
    };
//ETX
protected:
  vtkSMKeyFrameAnimationCueManipulatorProxy();
  ~vtkSMKeyFrameAnimationCueManipulatorProxy();

  // Description:
  // This method is called when the AnimationCue's StartAnimationCueEvent is
  // triggerred, to let the animation manipulator know that the cue has
  // been restarted. This is here for one major reason: after the last key frame,
  // the state of the scene must be as it was left a the the last key frame. This does not
  // happend automatically, since if while animating the currentime never coincides with the 
  // last key frame's key time, then it never gets a chance to update the properties value.
  // Hence, we note when the cue begins. Then, if the currentime is beyond that of the last key 
  // frame we pretend that the current time coincides with that of the last key frame and let
  // it update the properties. This is done only once per Animation cycle. The Initialize method
  // is used to indicate that a new animation cycle has begun.
  virtual void Initialize();

  vtkSMKeyFrameAnimationCueManipulatorProxyInternals* Internals;
  // Description:
  // This updates the values based on currenttime. 
  // currenttime is normalized to the time range of the Cue.
  virtual void UpdateValue(double currenttime, 
    vtkSMAnimationCueProxy* cueproxy);

  int AddKeyFrameInternal(vtkSMKeyFrameProxy* keyframe);
  int RemoveKeyFrameInternal(vtkSMKeyFrameProxy* keyframe);

  //BTX
  friend class vtkSMKeyFrameAnimationCueManipulatorProxyObserver;
  vtkSMKeyFrameAnimationCueManipulatorProxyObserver* Observer;
  void ExecuteEvent(vtkObject* obj, unsigned long event, void* calldata);
  //ETX
  int SendEndEvent;
private:
  vtkSMKeyFrameAnimationCueManipulatorProxy(const vtkSMKeyFrameAnimationCueManipulatorProxy&); // Not implemented.
  void operator=(const vtkSMKeyFrameAnimationCueManipulatorProxy&); // Not implemented.
};

#endif

