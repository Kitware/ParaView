// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVRampKeyFrame
 *
 * Interplates lineraly between consecutive key frames.
 */

#ifndef vtkPVRampKeyFrame_h
#define vtkPVRampKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKREMOTINGANIMATION_EXPORT vtkPVRampKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVRampKeyFrame* New();
  vtkTypeMacro(vtkPVRampKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method will do the actual interpolation.
   * currenttime is normalized to the time range between
   * this key frame and the next key frame.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next) override;

protected:
  vtkPVRampKeyFrame();
  ~vtkPVRampKeyFrame() override;

private:
  vtkPVRampKeyFrame(const vtkPVRampKeyFrame&) = delete;
  void operator=(const vtkPVRampKeyFrame&) = delete;
};

#endif
