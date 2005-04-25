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
// .NAME vtkSMAnimationCueProxy - proxy for vtkAnimationCue on a server.
// .SECTION Description
// .SECTION See Also
// vtkSMProxy vtkSMAnimationSceneProxy 
//

#ifndef __vtkSMAnimationCueProxy_h
#define __vtkSMAnimationCueProxy_h

#include "vtkSMProxy.h"

class vtkSMAnimationCueProxyObserver;
class vtkAnimationCue;
class vtkSMProperty;
class vtkSMDomain;
class vtkSMAnimationCueManipulatorProxy;

class VTK_EXPORT vtkSMAnimationCueProxy : public vtkSMProxy
{
public:
  static vtkSMAnimationCueProxy* New();
  vtkTypeRevisionMacro(vtkSMAnimationCueProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Saves the proxy to batch script.
  virtual void SaveInBatchScript(ofstream*);
  
  // Description:
  // Pointer to the proxy whose property is being animated by this cue.
  void SetAnimatedProxy(vtkSMProxy* proxy);
  vtkGetObjectMacro(AnimatedProxy, vtkSMProxy);

  // Description:
  // the XMLName of the property of the AnimatedProxy that is being
  // animated by this cue.
  vtkGetStringMacro(AnimatedPropertyName);
  vtkSetStringMacro(AnimatedPropertyName);

  // Description:
  // The domain name for the domain of the property to be used
  // to change the property value when animating. 
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
  // Set's the vtkAnimationCue time mode.
  void SetTimeMode(int mode);

  // Description:
  // Set's the vtkAnimationCue start time.
  void SetStartTime(double time);
  double GetStartTime();
  
  // Description:
  // Set's the vtkAnimationCue's end time.
  void SetEndTime(double time);
  double GetEndTime();

  vtkSMProperty* GetAnimatedProperty();
  vtkSMDomain* GetAnimatedDomain();

  vtkGetObjectMacro(AnimationCue, vtkAnimationCue);

  // Description:
  // Get/Set if caching is enabled.
  virtual void SetCaching(int enable);
  vtkGetMacro(Caching, int);
  
//BTX
  vtkClientServerID GetID() { return this->SelfID; }
//ETX
protected:
  vtkSMAnimationCueProxy();
  ~vtkSMAnimationCueProxy();

  virtual void CreateVTKObjects(int numeObjects);

  virtual void InitializeObservers(vtkAnimationCue* cue); 

  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  // Description:
  // Callbacks for corresponding Cue events. The argument must be 
  // casted to vtkAnimationCue::AnimationCueInfo.
  virtual void StartCueInternal(void* info);
  virtual void TickInternal(void* info);
  virtual void EndCueInternal(void* info);
  
//BTX
  vtkSMAnimationCueProxyObserver* Observer;
  friend class vtkSMAnimationCueProxyObserver;
//ETX

  int Caching; // flag indicating if the animation is to use Cache.
    // The SMAnimationScene synchrinized this flag for all cues it maintains.
 
  vtkSMProxy* AnimatedProxy;
  int AnimatedElement;
  char *AnimatedPropertyName;
  char *AnimatedDomainName;

  vtkAnimationCue *AnimationCue;
  vtkSMAnimationCueManipulatorProxy* Manipulator;
private:
  vtkSMAnimationCueProxy(const vtkSMAnimationCueProxy&); // Not implemented
  void operator=(const vtkSMAnimationCueProxy&); // Not implemented
};

#endif

