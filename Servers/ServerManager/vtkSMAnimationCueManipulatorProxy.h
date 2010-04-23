/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationCueManipulatorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAnimationCueManipulatorProxy - abstract proxy for manipulators
// used in animation.
// .SECTION Description
// An animation cue proxy delegates the operation of modifying the property 
// on the proxy being animated to a \b Manipulator. An example of a manipulator 
// is a vtkSMKeyFrameAnimationCueManipulatorProxy. Subclasses must override
// \c UpdateValue to perform the actual property manipulation. 
// Just like all proxies involved in Animation, this is a client side proxy, 
// with no VTK objects created on the server.
// A manipulator fires two kinds of events:
// \li \b vtkSMAnimationCueManipulatorProxy::StateModifiedEvent is fired when 
// the manipulator modifies the animated proxy.
// \li \b vtkCommand::Modified is fired when properties of the manipulator 
// are changed which affects the way the animation is generated e.g in case 
// of vtkSMKeyFrameAnimationCueManipulatorProxy, this event is fired when
// the key frames are changed i.e. added/removed/modified.
// .SECTION See Also
// vtkSMAnimationCueProxy vtkAnimationCue

#ifndef __vtkSMAnimationCueManipulatorProxy_h
#define __vtkSMAnimationCueManipulatorProxy_h

#include "vtkSMProxy.h"
class vtkSMAnimationCueProxy;
struct vtkClientServerID;

class VTK_EXPORT vtkSMAnimationCueManipulatorProxy : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMAnimationCueManipulatorProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  // Description:
  // Returns the SelfID for this proxy. This method is a residue
  // from the old days when GetSelfID() was not a public method.
  // Eventually this should be removed in favour of GetSelfID().
  vtkClientServerID GetID() {return this->GetSelfID();}
//ETX

//BTX
  // Description:
  // StateModifiedEvent - This event is fired when the manipulator modifies the animated proxy.
  // vtkCommand::Modified - is fired when the keyframes are changed i.e. added/removed/modified.
  enum
    {
    StateModifiedEvent = 2000
    };
//ETX

  // Description:
  // Overridden to call MarkAllPropertiesAsModified().
  virtual void Copy(vtkSMProxy* src, const char* exceptionClass, 
    int proxyPropertyCopyFlag);
protected:

  // Description:
  // Overridden simply to set ObjectsCreated to 1, since this class does 
  // not create any server side objects.
  virtual void CreateVTKObjects();

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
  virtual void Initialize(vtkSMAnimationCueProxy*){ }

  // Description:
  // This method is called when the AnimationCue's EndAnimationCueEvent is triggerred.
  // Typically, the Manipulator will set the state of the Cue to that at the
  // end of the cue.
  virtual void Finalize(vtkSMAnimationCueProxy*) { }
  
  // Description:
  // This updates the values based on currenttime.
  // currenttime is normalized to the time range of the Cue.
  virtual void UpdateValue(double currenttime, 
    vtkSMAnimationCueProxy* cueproxy)=0;

  vtkSMAnimationCueManipulatorProxy();
  ~vtkSMAnimationCueManipulatorProxy();
//BTX
  friend class vtkSMAnimationCueProxy;
//ETX
private:
  vtkSMAnimationCueManipulatorProxy(const vtkSMAnimationCueManipulatorProxy&); // Not implemented.
  void operator=(const vtkSMAnimationCueManipulatorProxy&); // Not implemented.
  
};

#endif

