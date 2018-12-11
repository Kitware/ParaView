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
/**
 * @class   vtkPVExponentialKeyFrame
 *
 * Interplates lineraly between consecutive key frames.
*/

#ifndef vtkPVExponentialKeyFrame_h
#define vtkPVExponentialKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKPVANIMATION_EXPORT vtkPVExponentialKeyFrame : public vtkPVKeyFrame
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

  //@{
  /**
   * Base to be used for exponential function.
   */
  vtkSetMacro(Base, double);
  vtkGetMacro(Base, double);
  //@}

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
