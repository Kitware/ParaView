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
/**
 * @class   vtkPVAnimationCue
 * @brief   proxy for vtkAnimationCue.
 *
 * This is a proxy for vtkAnimationCue. All animation proxies are client
 * side proxies.
 * This class needs a vtkPVCueManipulator. The \b Manipulator
 * performs the actual interpolation.
 * @sa
 * vtkAnimationCue vtkSMAnimationSceneProxy
 *
*/

#ifndef vtkPVAnimationCue_h
#define vtkPVAnimationCue_h

#include "vtkAnimationCue.h"
#include "vtkPVAnimationModule.h" // needed for export macro

class vtkAnimationCue;
class vtkCommand;
class vtkPVCueManipulator;

class VTKPVANIMATION_EXPORT vtkPVAnimationCue : public vtkAnimationCue
{
public:
  vtkTypeMacro(vtkPVAnimationCue, vtkAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The index of the element of the property this cue animates.
   * If the index is -1, the cue will animate all the elements
   * of the animated property.
   */
  vtkSetMacro(AnimatedElement, int);
  vtkGetMacro(AnimatedElement, int);
  //@}

  //@{
  /**
   * Get/Set the manipulator used to compute values
   * for each instance in the animation.
   * Note that the time passed to the Manipulator is normalized [0,1]
   * to the extents of this cue.
   */
  void SetManipulator(vtkPVCueManipulator*);
  vtkGetObjectMacro(Manipulator, vtkPVCueManipulator);
  //@}

  //@{
  /**
   * Enable/Disable this cue.
   */
  vtkSetMacro(Enabled, int);
  vtkGetMacro(Enabled, int);
  vtkBooleanMacro(Enabled, int);
  //@}

  //@{
  /**
   * Used to update the animated item. This API makes it possible for vtk-level
   * classes to update properties without actually linking with the
   * ServerManager library. This only works since they object are created only
   * on the client.
   */
  virtual void BeginUpdateAnimationValues() = 0;
  virtual void SetAnimationValue(int index, double value) = 0;
  virtual void EndUpdateAnimationValues() = 0;
  //@}

  //@{
  /**
   * When set to true, the manipulator is skipped and the key frame value is set
   * by using the ClockTime directly. false by default.
   */
  vtkSetMacro(UseAnimationTime, bool);
  vtkGetMacro(UseAnimationTime, bool);
  //@}

  //@{
  /**
   * Overridden to ignore the calls when this->Enabled == false.
   */
  void Initialize() override;
  void Tick(double currenttime, double deltatime, double clocktime) override;
  void Finalize() override;
  //@}

protected:
  vtkPVAnimationCue();
  ~vtkPVAnimationCue() override;

  //@{
  void StartCueInternal() override;
  void TickInternal(double currenttime, double deltatime, double clocktime) override;
  void EndCueInternal() override;
  //@}

  friend class vtkSMAnimationSceneProxy;

  unsigned long ObserverID;
  bool UseAnimationTime;
  int AnimatedElement;
  int Enabled;

  vtkAnimationCue* AnimationCue;
  vtkPVCueManipulator* Manipulator;

private:
  vtkPVAnimationCue(const vtkPVAnimationCue&) = delete;
  void operator=(const vtkPVAnimationCue&) = delete;
};

#endif
