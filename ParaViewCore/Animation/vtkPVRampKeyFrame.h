/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRampKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRampKeyFrame
// .SECTION Description
// Interplates lineraly between consequtive key frames.

#ifndef __vtkPVRampKeyFrame_h
#define __vtkPVRampKeyFrame_h

#include "vtkPVKeyFrame.h"

class VTKPVANIMATION_EXPORT vtkPVRampKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVRampKeyFrame* New();
  vtkTypeMacro(vtkPVRampKeyFrame, vtkPVKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
                           vtkPVAnimationCue* cue, vtkPVKeyFrame* next);

protected:
  vtkPVRampKeyFrame();
  ~vtkPVRampKeyFrame();

private:
  vtkPVRampKeyFrame(const vtkPVRampKeyFrame&); // Not implemented.
  void operator=(const vtkPVRampKeyFrame&); // Not implemented.

};

#endif
