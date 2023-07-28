// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVSinusoidKeyFrame
 *
 * Interplates a sinusoid. At any given time \c t, the resultant
 * value obtained from this keyframe is given by :
 * value = this->Offset + (Key Value) * Sin (2*pi*theta);
 * where theta = this->Frequency*t + (this->Phase/360).
 * As is clear from  the equation, the amplitude of the wave
 * is obtained from the value of the keyframe.
 */

#ifndef vtkPVSinusoidKeyFrame_h
#define vtkPVSinusoidKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKREMOTINGANIMATION_EXPORT vtkPVSinusoidKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVSinusoidKeyFrame* New();
  vtkTypeMacro(vtkPVSinusoidKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method will do the actual interpolation.
   * currenttime is normalized to the time range between
   * this key frame and the next key frame.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next) override;

  ///@{
  /**
   * Get/Set the phase for the sine wave.
   */
  vtkSetMacro(Phase, double);
  vtkGetMacro(Phase, double);
  ///@}

  ///@{
  /**
   * Get/Set the frequency for the sine wave in number of cycles
   * for the entire length of this keyframe i.e. until the next key frame.
   */
  vtkSetMacro(Frequency, double);
  vtkGetMacro(Frequency, double);
  ///@}

  ///@{
  /**
   * Get/Set the Wave offset.
   */
  vtkSetMacro(Offset, double);
  vtkGetMacro(Offset, double);
  ///@}

protected:
  vtkPVSinusoidKeyFrame();
  ~vtkPVSinusoidKeyFrame() override;

  double Phase;
  double Frequency;
  double Offset;

private:
  vtkPVSinusoidKeyFrame(const vtkPVSinusoidKeyFrame&) = delete;
  void operator=(const vtkPVSinusoidKeyFrame&) = delete;
};

#endif
