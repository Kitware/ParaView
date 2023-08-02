// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVKeyFrameAnimationCue
 *
 * vtkPVKeyFrameAnimationCue is a specialization of vtkPVAnimationCue that uses
 * the vtkPVKeyFrameCueManipulator as the manipulator.
 */

#ifndef vtkPVKeyFrameAnimationCue_h
#define vtkPVKeyFrameAnimationCue_h

#include "vtkPVAnimationCue.h"

class vtkPVKeyFrame;
class vtkPVKeyFrameCueManipulator;

class VTKREMOTINGANIMATION_EXPORT vtkPVKeyFrameAnimationCue : public vtkPVAnimationCue
{
public:
  vtkTypeMacro(vtkPVKeyFrameAnimationCue, vtkPVAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Forwarded to the internal vtkPVKeyFrameCueManipulator.
   */
  int AddKeyFrame(vtkPVKeyFrame* keyframe);
  int GetLastAddedKeyFrameIndex();
  void RemoveKeyFrame(vtkPVKeyFrame*);
  void RemoveAllKeyFrames();
  ///@}

protected:
  vtkPVKeyFrameAnimationCue();
  ~vtkPVKeyFrameAnimationCue() override;

  vtkPVKeyFrameCueManipulator* GetKeyFrameManipulator();

private:
  vtkPVKeyFrameAnimationCue(const vtkPVKeyFrameAnimationCue&) = delete;
  void operator=(const vtkPVKeyFrameAnimationCue&) = delete;
};

#endif
