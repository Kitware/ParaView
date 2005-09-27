/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSinusoidKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSinusoidKeyFrameProxy
// .SECTION Description
// Interplates a sinusoid. 

#ifndef __vtkSMSinusoidKeyFrameProxy_h
#define __vtkSMSinusoidKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class VTK_EXPORT vtkSMSinusoidKeyFrameProxy : public vtkSMKeyFrameProxy
{
public:
  vtkTypeRevisionMacro(vtkSMSinusoidKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMSinusoidKeyFrameProxy* New();

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);

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

  virtual void SaveInBatchScript(ofstream* file);
protected:
  vtkSMSinusoidKeyFrameProxy();
  ~vtkSMSinusoidKeyFrameProxy();
  
  double Phase;
  double Frequency;
  double Offset;

private:
  vtkSMSinusoidKeyFrameProxy(const vtkSMSinusoidKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMSinusoidKeyFrameProxy&); // Not implemented.
};

#endif


