/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExponentialKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExponentialKeyFrame
// .SECTION Description
// Interplates lineraly between consequtive key frames.

#ifndef __vtkPVExponentialKeyFrame_h
#define __vtkPVExponentialKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKPVANIMATION_EXPORT vtkPVExponentialKeyFrame : public vtkPVKeyFrame
{
public:
  vtkTypeMacro(vtkPVExponentialKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVExponentialKeyFrame* New();

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue( double currenttime, vtkPVAnimationCue* cue,
                            vtkPVKeyFrame* next);

  // Description:
  // Base to be used for exponential function.
  vtkSetMacro(Base, double);
  vtkGetMacro(Base, double);

  vtkSetMacro(StartPower, double);
  vtkGetMacro(StartPower, double);

  vtkSetMacro(EndPower, double);
  vtkGetMacro(EndPower, double);

protected:
  vtkPVExponentialKeyFrame();
  ~vtkPVExponentialKeyFrame();

  double Base;
  double StartPower;
  double EndPower;

private:
  vtkPVExponentialKeyFrame(const vtkPVExponentialKeyFrame&); // Not implemented.
  void operator=(const vtkPVExponentialKeyFrame&); // Not implemented.

};

#endif
