/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCueManipulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCueManipulator
 * @brief   abstract proxy for manipulators
 * used in animation.
 *
 * An animation cue proxy delegates the operation of modifying the property
 * on the proxy being animated to a \b Manipulator. An example of a manipulator
 * is a vtkPVKeyFrameCueManipulator. Subclasses must override
 * \c UpdateValue to perform the actual property manipulation.
 * Just like all proxies involved in Animation, this is a client side proxy,
 * with no VTK objects created on the server.
 * A manipulator fires two kinds of events:
 * \li \b vtkPVCueManipulator::StateModifiedEvent is fired when
 * the manipulator modifies the animated proxy.
 * \li \b vtkCommand::Modified is fired when properties of the manipulator
 * are changed which affects the way the animation is generated e.g in case
 * of vtkPVKeyFrameCueManipulator, this event is fired when
 * the key frames are changed i.e. added/removed/modified.
 * @sa
 * vtkPVAnimationCue vtkAnimationCue
*/

#ifndef vtkPVCueManipulator_h
#define vtkPVCueManipulator_h

#include "vtkObject.h"
#include "vtkPVAnimationModule.h" // needed for export macro

class vtkPVAnimationCue;

class VTKPVANIMATION_EXPORT vtkPVCueManipulator : public vtkObject
{
public:
  vtkTypeMacro(vtkPVCueManipulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * StateModifiedEvent - This event is fired when the manipulator modifies the animated proxy.
   * vtkCommand::Modified - is fired when the keyframes are changed i.e. added/removed/modified.
   */
  enum
  {
    StateModifiedEvent = 2000
  };

protected:
  /**
   * This method is called when the AnimationCue's StartAnimationCueEvent is
   * triggered, to let the animation manipulator know that the cue has
   * been restarted. This is here for one major reason: after the last key frame,
   * the state of the scene must be as it was left a the the last key frame. This does not
   * happened automatically, since if while animating the currentime never coincides with the
   * last key frame's key time, then it never gets a chance to update the properties value.
   * Hence, we note when the cue begins. Then, if the currentime is beyond that of the last key
   * frame we pretend that the current time coincides with that of the last key frame and let
   * it update the properties. This is done only once per Animation cycle. The Initialize method
   * is used to indicate that a new animation cycle has begun.
   */
  virtual void Initialize(vtkPVAnimationCue*) {}

  /**
   * This method is called when the AnimationCue's EndAnimationCueEvent is triggered.
   * Typically, the Manipulator will set the state of the Cue to that at the
   * end of the cue.
   */
  virtual void Finalize(vtkPVAnimationCue*) {}

  /**
   * This updates the values based on currenttime.
   * currenttime is normalized to the time range of the Cue.
   */
  virtual void UpdateValue(double currenttime, vtkPVAnimationCue* cueproxy) = 0;

  vtkPVCueManipulator();
  ~vtkPVCueManipulator() override;
  friend class vtkPVAnimationCue;

private:
  vtkPVCueManipulator(const vtkPVCueManipulator&) = delete;
  void operator=(const vtkPVCueManipulator&) = delete;
};

#endif
