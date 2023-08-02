// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVExponentialKeyFrame
 *
 * Interplates lineraly between consecutive key frames.
 */

#ifndef vtkPVExponentialKeyFrame_h
#define vtkPVExponentialKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKREMOTINGANIMATION_EXPORT vtkPVExponentialKeyFrame : public vtkPVKeyFrame
{
public:
  vtkTypeMacro(vtkPVExponentialKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVExponentialKeyFrame* New();

  /**
   * This method will do the actual interpolation.
   * currenttime is normalized to the time range between
   * this key frame and the next key frame.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next) override;

  ///@{
  /**
   * Base to be used for exponential function.
   */
  vtkSetMacro(Base, double);
  vtkGetMacro(Base, double);
  ///@}

  vtkSetMacro(StartPower, double);
  vtkGetMacro(StartPower, double);

  vtkSetMacro(EndPower, double);
  vtkGetMacro(EndPower, double);

protected:
  vtkPVExponentialKeyFrame();
  ~vtkPVExponentialKeyFrame() override;

  double Base;
  double StartPower;
  double EndPower;

private:
  vtkPVExponentialKeyFrame(const vtkPVExponentialKeyFrame&) = delete;
  void operator=(const vtkPVExponentialKeyFrame&) = delete;
};

#endif
