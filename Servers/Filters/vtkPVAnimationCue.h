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
// .NAME vtkPVAnimationCue - proxy for vtkAnimationCue.
// .SECTION Description
// This is a proxy for vtkAnimationCue. All animation proxies are client
// side proxies.
// This class needs a vtkPVCueManipulator. The \b Manipulator
// performs the actual interpolation.
// .SECTION See Also
// vtkAnimationCue vtkSMAnimationSceneProxy
//

#ifndef __vtkPVAnimationCue_h
#define __vtkPVAnimationCue_h

#include "vtkObject.h"

class vtkAnimationCue;
class vtkCommand;
class vtkPVCueManipulator;
class vtkPVAnimationCueObserver;

class VTK_EXPORT vtkPVAnimationCue : public vtkObject
{
public:
  vtkTypeMacro(vtkPVAnimationCue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Forwarded to the internal vtkAnimationCue
  void SetTimeMode(int mode);
  void SetStartTime(double val);
  void SetEndTime(double val);

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
  void SetManipulator(vtkPVCueManipulator*);
  vtkGetObjectMacro(Manipulator, vtkPVCueManipulator);

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

  // Description:
  // Used to update the animated item. This API makes it possible for vtk-level
  // classes to update properties without actually linking with the
  // ServerManager library. This only works since they object are created only
  // on the client.
  virtual void BeginUpdateAnimationValues() = 0;
  virtual void SetAnimationValue(int index, double value) =0;
  virtual void EndUpdateAnimationValues() = 0;

  // Description:
  // When set to true, the manipulator is skipped and the key frame value is set
  // by using the ClockTime directly. false by default.
  vtkSetMacro(UseAnimationTime, bool);
  vtkGetMacro(UseAnimationTime, bool);

protected:
  vtkPVAnimationCue();
  ~vtkPVAnimationCue();

  virtual void InitializeObservers(vtkAnimationCue* cue);

  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  // Description:
  // Callbacks for corresponding Cue events. The argument must be
  // casted to vtkAnimationCue::AnimationCueInfo.
  virtual void StartCueInternal(void* info);
  virtual void TickInternal(void* info);
  virtual void EndCueInternal(void* info);

//BTX
  vtkCommand* Observer;
  friend class vtkPVAnimationCueObserver;
  friend class vtkSMAnimationSceneProxy;
//ETX

  bool UseAnimationTime;
  int AnimatedElement;
  int Enabled;

  vtkAnimationCue *AnimationCue;
  vtkPVCueManipulator* Manipulator;
private:
  vtkPVAnimationCue(const vtkPVAnimationCue&); // Not implemented
  void operator=(const vtkPVAnimationCue&); // Not implemented
};

#endif
