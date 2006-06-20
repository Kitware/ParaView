/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimestepKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTimestepKeyFrameProxy
// .SECTION Description
// This is an interpolator which can be used for \c Timestep property of a 
// reader. This uses the actual timestep values provided by the reader, 
// scales them over the play time and changes the time accordingly. 

#ifndef __vtkSMTimestepKeyFrameProxy_h
#define __vtkSMTimestepKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class VTK_EXPORT vtkSMTimestepKeyFrameProxy : public vtkSMKeyFrameProxy
{
public:
  static vtkSMTimestepKeyFrameProxy* New();
  vtkTypeRevisionMacro(vtkSMTimestepKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);
protected:
  vtkSMTimestepKeyFrameProxy();
  ~vtkSMTimestepKeyFrameProxy();

private:
  vtkSMTimestepKeyFrameProxy(const vtkSMTimestepKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMTimestepKeyFrameProxy&); // Not implemented.
};

#endif

