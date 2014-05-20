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
// .NAME vtkPVBooleanKeyFrame
// .SECTION Description
// Empty key frame. Can be used to toggle boolean properties.

#ifndef __vtkSMBooleanKeyFrameProxy_h
#define __vtkSMBooleanKeyFrameProxy_h

#include "vtkPVKeyFrame.h"

class VTKPVANIMATION_EXPORT vtkPVBooleanKeyFrame: public vtkPVKeyFrame
{
public:
  vtkTypeMacro(vtkPVBooleanKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVBooleanKeyFrame* New();

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkPVAnimationCue* cueProxy, vtkPVKeyFrame* next);

protected:
  vtkPVBooleanKeyFrame();
  ~vtkPVBooleanKeyFrame();

private:
  vtkPVBooleanKeyFrame(const vtkPVBooleanKeyFrame&); // Not implemented.
  void operator=(const vtkPVBooleanKeyFrame&); // Not implemented.

};
#endif
