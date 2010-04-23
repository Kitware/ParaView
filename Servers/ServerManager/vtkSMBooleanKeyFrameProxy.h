/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBooleanKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBooleanKeyFrameProxy
// .SECTION Description
// Empty key frame. Can be used to toggle boolean properties.

#ifndef __vtkSMBooleanKeyFrameProxy_h
#define __vtkSMBooleanKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class VTK_EXPORT vtkSMBooleanKeyFrameProxy: public vtkSMKeyFrameProxy
{
public:
  vtkTypeMacro(vtkSMBooleanKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMBooleanKeyFrameProxy* New();

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);

protected:
  vtkSMBooleanKeyFrameProxy();
  ~vtkSMBooleanKeyFrameProxy();
  
private:
  vtkSMBooleanKeyFrameProxy(const vtkSMBooleanKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMBooleanKeyFrameProxy&); // Not implemented.

};




#endif

