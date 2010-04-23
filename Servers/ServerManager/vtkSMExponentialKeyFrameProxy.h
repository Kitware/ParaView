/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExponentialKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMExponentialKeyFrameProxy
// .SECTION Description
// Interplates lineraly between consequtive key frames.

#ifndef __vtkSMExponentialKeyFrameProxy_h
#define __vtkSMExponentialKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class VTK_EXPORT vtkSMExponentialKeyFrameProxy : public vtkSMKeyFrameProxy
{
public:
  vtkTypeMacro(vtkSMExponentialKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMExponentialKeyFrameProxy* New();

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);

  // Description:
  // Base to be used for exponential function.
  vtkSetMacro(Base, double);
  vtkGetMacro(Base, double);

  vtkSetMacro(StartPower, double);
  vtkGetMacro(StartPower, double);

  vtkSetMacro(EndPower, double);
  vtkGetMacro(EndPower, double);

protected:
  vtkSMExponentialKeyFrameProxy();
  ~vtkSMExponentialKeyFrameProxy();
  
  double Base;
  double StartPower;
  double EndPower;
  
private:
  vtkSMExponentialKeyFrameProxy(const vtkSMExponentialKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMExponentialKeyFrameProxy&); // Not implemented.

};


#endif


