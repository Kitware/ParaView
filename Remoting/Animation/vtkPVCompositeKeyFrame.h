// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVCompositeKeyFrame
 * @brief   composite keyframe.
 *
 * There are many different types of keyframes such as
 * vtkPVSinusoidKeyFrame, vtkPVRampKeyFrame etc.
 * This is keyframe that has all different types of keyframes
 * as internal objects and provides API to choose one of them as the
 * active type. This is helpful in GUIs that allow for switching the
 * type of keyframe on the fly without much effort from the GUI.
 */

#ifndef vtkPVCompositeKeyFrame_h
#define vtkPVCompositeKeyFrame_h

#include "vtkPVKeyFrame.h"

class vtkPVBooleanKeyFrame;
class vtkPVSinusoidKeyFrame;
class vtkPVExponentialKeyFrame;
class vtkPVRampKeyFrame;

class VTKREMOTINGANIMATION_EXPORT vtkPVCompositeKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVCompositeKeyFrame* New();
  vtkTypeMacro(vtkPVCompositeKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    NONE = 0,
    BOOLEAN = 1,
    RAMP = 2,
    EXPONENTIAL = 3,
    SINUSOID = 4
  };

  ///@{
  /**
   * Overridden to pass on to the internal keyframe proxies.
   */
  void RemoveAllKeyValues() override;
  void SetKeyTime(double time) override;
  void SetKeyValue(double val) override { this->Superclass::SetKeyValue(val); }
  void SetKeyValue(unsigned int index, double val) override;
  void SetNumberOfKeyValues(unsigned int num) override;
  ///@}

  ///@{
  /**
   * Passed on to the ExponentialKeyFrame.
   */
  void SetBase(double val);
  void SetStartPower(double val);
  void SetEndPower(double val);
  ///@}

  ///@{
  /**
   * Passed on to the SinusoidKeyFrame.
   */
  void SetPhase(double val);
  void SetFrequency(double val);
  void SetOffset(double val);
  ///@}

  ///@{
  /**
   * Get/Set the type of keyframe to be used as the active type.
   * Default is RAMP.
   */
  vtkSetClampMacro(Type, int, NONE, SINUSOID);
  vtkGetMacro(Type, int);
  const char* GetTypeAsString() { return this->GetTypeAsString(this->Type); }
  static const char* GetTypeAsString(int);
  static int GetTypeFromString(const char* string);
  ///@}

  /**
   * This method will do the actual interpolation.
   * currenttime is normalized to the time range between
   * this key frame and the next key frame.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cue, vtkPVKeyFrame* next) override;

protected:
  vtkPVCompositeKeyFrame();
  ~vtkPVCompositeKeyFrame() override;

  int Type;

  vtkPVBooleanKeyFrame* BooleanKeyFrame;
  vtkPVRampKeyFrame* RampKeyFrame;
  vtkPVExponentialKeyFrame* ExponentialKeyFrame;
  vtkPVSinusoidKeyFrame* SinusoidKeyFrame;

private:
  vtkPVCompositeKeyFrame(const vtkPVCompositeKeyFrame&) = delete;
  void operator=(const vtkPVCompositeKeyFrame&) = delete;
};

#endif
