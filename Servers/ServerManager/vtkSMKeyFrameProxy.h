/*=========================================================================

  Program:   ParaView
  Module:    vtkSMKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMKeyFrameProxy
// .SECTION Description
// Base class for key frames.
// A key frame is responsible to interpolate the curve
// between it self and a consequent key frame. A new subclass is
// needed for each type of interpolation available between two 
// key frames. This class can be instantiated to create a no-action key 
// frame. 

#ifndef __vtkSMKeyFrameProxy_h
#define __vtkSMKeyFrameProxy_h

#include "vtkSMProxy.h"
class vtkSMAnimationCueProxy;
class vtkClientServerID;

class VTK_EXPORT vtkSMKeyFrameProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMKeyFrameProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMKeyFrameProxy* New();
  
  virtual void SaveInBatchScript(ofstream* file);

  // Description:;
  // Key time is the time at which this key frame is
  // associated. KeyTime ranges from [0,1], where 0 is the
  // start time of the cue for which this is a key frame and
  // 1 is that cue's end time.
  vtkSetMacro(KeyTime, double);
  vtkGetMacro(KeyTime, double);

  // Description:
  // Get/Set the animated value at this key frame.
  vtkSetMacro(KeyValue, double);
  vtkGetMacro(KeyValue, double);

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);

  vtkClientServerID GetID() { return this->SelfID; }
protected:
  vtkSMKeyFrameProxy();
  ~vtkSMKeyFrameProxy();

  double KeyTime;
  double KeyValue;

private:
  vtkSMKeyFrameProxy(const vtkSMKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMKeyFrameProxy&); // Not implemented.
};

#endif

