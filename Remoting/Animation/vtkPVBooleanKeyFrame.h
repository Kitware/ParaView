/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBooleanKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVBooleanKeyFrame
 *
 * Empty key frame. Can be used to toggle boolean properties.
*/

#ifndef vtkPVBooleanKeyFrame_h
#define vtkPVBooleanKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKREMOTINGANIMATION_EXPORT vtkPVBooleanKeyFrame : public vtkPVKeyFrame
{
public:
  vtkTypeMacro(vtkPVBooleanKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVBooleanKeyFrame* New();

  /**
   * This method will do the actual interpolation.
   * currenttime is normalized to the time range between
   * this key frame and the next key frame.
   */
  void UpdateValue(double currenttime, vtkPVAnimationCue* cueProxy, vtkPVKeyFrame* next) override;

protected:
  vtkPVBooleanKeyFrame();
  ~vtkPVBooleanKeyFrame() override;

private:
  vtkPVBooleanKeyFrame(const vtkPVBooleanKeyFrame&) = delete;
  void operator=(const vtkPVBooleanKeyFrame&) = delete;
};
#endif
