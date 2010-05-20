/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRampKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRampKeyFrameProxy
// .SECTION Description
// Interplates lineraly between consequtive key frames.

#ifndef __vtkSMRampKeyFrameProxy_h
#define __vtkSMRampKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class VTK_EXPORT vtkSMRampKeyFrameProxy : public vtkSMKeyFrameProxy
{
public:
  vtkTypeMacro(vtkSMRampKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMRampKeyFrameProxy* New();

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);

protected:
  vtkSMRampKeyFrameProxy();
  ~vtkSMRampKeyFrameProxy();
  
private:
  vtkSMRampKeyFrameProxy(const vtkSMRampKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMRampKeyFrameProxy&); // Not implemented.

};


#endif


