/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationCueProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAnimationCueProxy - proxy for vtkAnimationCue.
// .SECTION Description
// This is a proxy for vtkAnimationCue. All animation proxies are client 
// side proxies.
// This class needs a vtkSMAnimationCueManipulatorProxy. The \b Manipulator
// performs the actual interpolation.
// .SECTION See Also
// vtkAnimationCue vtkSMAnimationSceneProxy 
//

#ifndef __vtkSMAnimationCueProxy_h
#define __vtkSMAnimationCueProxy_h

#include "vtkSMProxy.h"

class vtkAnimationCue;
class vtkCommand;
class vtkSMAnimationCueManipulatorProxy;
class vtkSMAnimationCueProxyObserver;
class vtkSMDomain;
class vtkSMProperty;

class VTK_EXPORT vtkSMAnimationCueProxy : public vtkSMProxy
{
public:
  static vtkSMAnimationCueProxy* New();
  vtkTypeRevisionMacro(vtkSMAnimationCueProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Pointer to the proxy whose property is being animated by this cue.
  void SetAnimatedProxy(vtkSMProxy* proxy);
  vtkGetObjectMacro(AnimatedProxy, vtkSMProxy);

  // Description:
  // Removes the animated proxy reference.
  void RemoveAnimatedProxy();

  // Description:
  // the XMLName of the property of the AnimatedProxy that is being
  // animated by this cue.
  vtkGetStringMacro(AnimatedPropertyName);
  vtkSetStringMacro(AnimatedPropertyName);

  // Description:
  // The domain name for the domain of the property to be used
  // to change the property value when animating. If domain is 
  // not set the first domain on the property is used. Note that 
  // domain is essential since the change of value on the property
  // is not directly performed by changing the property
  // instead delegated to the domain. This makes it possible to
  // using \c double (or \c int) values even for String 
  // properties such as Filenames.
  vtkGetStringMacro(AnimatedDomainName);
  vtkSetStringMacro(AnimatedDomainName);

  // Description:
  // The index of the element of the property this cue animates.
  // If the index is -1, the cue will animate all the elements
  // of the animated property.
  vtkSetMacro(AnimatedElement, int);
  vtkGetMacro(AnimatedElement, int);
    
  // Description:
  // Get/Set the manipulator used to compute values 
  // for each instance in the animation.
  // Note that the time passed to the Manipulator is normalized [0,1]
  // to the extents of this cue.
  void SetManipulator(vtkSMAnimationCueManipulatorProxy*);
  vtkGetObjectMacro(Manipulator, vtkSMAnimationCueManipulatorProxy);

  // Description:
  // Get the property being animated.
  vtkSMProperty* GetAnimatedProperty();

  // Description:
  // Get the domain being animated.
  vtkSMDomain* GetAnimatedDomain();

  // Description:
  // This copies a clone of the AnimationCue. The AnimatedProxy
  // is shallow copied, while the Manipulator, and the keyframes
  // are deep copied.
  virtual void CloneCopy(vtkSMAnimationCueProxy* src);

  // Description:
  // This is valid only in a AnimationCueTickEvent handler. 
  // Before firing the event the animation cue sets the AnimationTime to
  // the time of the tick.
  double GetAnimationTime();

  // Description:
  // This is valid only in a AnimationCueTickEvent handler.
  // Before firing the event the animation cue sets the DeltaTime
  // to the difference in time between the current tick and the last tick.
  double GetDeltaTime();

  // Description:
  // This is valid only in a AnimationCueTickEvent handler. 
  // Before firing the event the animation cue sets the ClockTime to
  // the time of the tick. ClockTime is directly the time from the animation
  // scene neither normalized nor offsetted to the start of the scene.
  double GetClockTime();

  // Description:
  // Enable/Disable this cue.
  vtkSetMacro(Enabled, int);
  vtkGetMacro(Enabled, int);
  vtkBooleanMacro(Enabled, int);

protected:
  vtkSMAnimationCueProxy();
  ~vtkSMAnimationCueProxy();

  // Overridden to mark the AnimatedProxy dirty as well. That ensures that any
  // cache is cleared.
  virtual void MarkConsumersAsDirty(vtkSMProxy* modifiedProxy);

  virtual void CreateVTKObjects();

  virtual void InitializeObservers(vtkAnimationCue* cue); 

  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  // Description:
  // Callbacks for corresponding Cue events. The argument must be 
  // casted to vtkAnimationCue::AnimationCueInfo.
  virtual void StartCueInternal(void* info);
  virtual void TickInternal(void* info);
  virtual void EndCueInternal(void* info);

  // Description;
  // Since animation Cue has no server side objects, reviving an animation
  // Cue is no different from create a new animation Cue. Hence, we
  // override this method to simply call CreateVTKObjects().
  virtual void ReviveVTKObjects()
    { this->CreateVTKObjects(); }

//BTX
  vtkCommand* Observer;
  friend class vtkSMAnimationCueProxyObserver;
  friend class vtkSMAnimationSceneProxy;
//ETX

  vtkSMProxy* AnimatedProxy;
  int AnimatedElement;
  char *AnimatedPropertyName;
  char *AnimatedDomainName;

  int Enabled;

  vtkAnimationCue *AnimationCue;
  vtkSMAnimationCueManipulatorProxy* Manipulator;
private:
  vtkSMAnimationCueProxy(const vtkSMAnimationCueProxy&); // Not implemented
  void operator=(const vtkSMAnimationCueProxy&); // Not implemented
};

#endif

