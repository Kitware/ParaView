/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeKeyFrame - composite keyframe.
// .SECTION Description
// There are many different types of keyframes such as
// vtkPVSinusoidKeyFrame, vtkPVRampKeyFrame etc.
// This is keyframe that has all different types of keyframes
// as internal objects and provides API to choose one of them as the
// active type. This is helpful in GUIs that allow for switching the
// type of keyframe on the fly without much effort from the GUI.

#ifndef __vtkPVCompositeKeyFrame_h
#define __vtkPVCompositeKeyFrame_h

#include "vtkPVKeyFrame.h"

class vtkPVBooleanKeyFrame;
class vtkPVSinusoidKeyFrame;
class vtkPVExponentialKeyFrame;
class vtkPVRampKeyFrame;

class VTKPVANIMATION_EXPORT vtkPVCompositeKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVCompositeKeyFrame* New();
  vtkTypeMacro(vtkPVCompositeKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);
  //BTX
  enum
    {
    NONE =0,
    BOOLEAN=1,
    RAMP=2,
    EXPONENTIAL=3,
    SINUSOID=4
    };
  //ETX

  // Description:
  // Overridden to pass on to the internal keyframe proxies.
  virtual void RemoveAllKeyValues();
  virtual void SetKeyTime(double time);
  virtual void SetKeyValue(double val)
    { this->Superclass::SetKeyValue(val); }
  virtual void SetKeyValue(unsigned int index, double val);
  virtual void SetNumberOfKeyValues(unsigned int num);

  // Description:
  // Passed on to the ExponentialKeyFrame.
  void SetBase(double val);
  void SetStartPower(double val);
  void SetEndPower(double val);

  // Description:
  // Passed on to the SinusoidKeyFrame.
  void SetPhase(double val);
  void SetFrequency(double val);
  void SetOffset(double val);

  // Description:
  // Get/Set the type of keyframe to be used as the active type.
  // Default is RAMP.
  vtkSetClampMacro(Type, int, NONE, SINUSOID);
  vtkGetMacro(Type, int);
  const char* GetTypeAsString() { return this->GetTypeAsString(this->Type); }
  static const char* GetTypeAsString(int);
  static int GetTypeFromString(const char* string);

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue( double currenttime,
                            vtkPVAnimationCue* cue, vtkPVKeyFrame* next);

protected:
  vtkPVCompositeKeyFrame();
  ~vtkPVCompositeKeyFrame();

  int Type;

  vtkPVBooleanKeyFrame* BooleanKeyFrame;
  vtkPVRampKeyFrame* RampKeyFrame;
  vtkPVExponentialKeyFrame* ExponentialKeyFrame;
  vtkPVSinusoidKeyFrame* SinusoidKeyFrame;
private:
  vtkPVCompositeKeyFrame(const vtkPVCompositeKeyFrame&); // Not implemented.
  void operator=(const vtkPVCompositeKeyFrame&); // Not implemented.
};

#endif
