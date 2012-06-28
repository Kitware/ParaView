/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSinusoidKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSinusoidKeyFrame
// .SECTION Description
// Interplates a sinusoid. At any given time \c t, the resultant
// value obtained from this keyframe is given by :
// value = this->Offset + (Key Value) * Sin (2*pi*theta);
// where theta = this->Frequency*t + (this->Phase/360).
// As is clear from  the equation, the amplitude of the wave
// is obtained from the value of the keyframe.

#ifndef __vtkPVSinusoidKeyFrame_h
#define __vtkPVSinusoidKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTK_EXPORT vtkPVSinusoidKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVSinusoidKeyFrame* New();
  vtkTypeMacro(vtkPVSinusoidKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue( double currenttime, vtkPVAnimationCue* cue,
                            vtkPVKeyFrame* next);

  // Description:
  // Get/Set the phase for the sine wave.
  vtkSetMacro(Phase, double);
  vtkGetMacro(Phase, double);

  // Description:
  // Get/Set the frequency for the sine wave in number of cycles
  // for the entire length of this keyframe i.e. until the next key frame.
  vtkSetMacro(Frequency, double);
  vtkGetMacro(Frequency, double);

  // Description:
  // Get/Set the Wave offset.
  vtkSetMacro(Offset, double);
  vtkGetMacro(Offset, double);

protected:
  vtkPVSinusoidKeyFrame();
  ~vtkPVSinusoidKeyFrame();

  double Phase;
  double Frequency;
  double Offset;

private:
  vtkPVSinusoidKeyFrame(const vtkPVSinusoidKeyFrame&); // Not implemented.
  void operator=(const vtkPVSinusoidKeyFrame&); // Not implemented.
};

#endif
