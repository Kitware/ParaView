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

class vtkSMKeyFrameProxyInternals;
class vtkSMAnimationCueProxy;
struct vtkClientServerID;

class VTK_EXPORT vtkSMKeyFrameProxy : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMKeyFrameProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSMKeyFrameProxy* New();
  
  // Description:;
  // Key time is the time at which this key frame is
  // associated. KeyTime ranges from [0,1], where 0 is the
  // start time of the cue for which this is a key frame and
  // 1 is that cue's end time.
  vtkSetMacro(KeyTime, double);
  vtkGetMacro(KeyTime, double);

  // Description:
  // Get/Set the animated value at this key frame.
  // Note that is the number of values is adjusted to fit the index
  // specified in SetKeyValue.
  void SetKeyValue(double val) { this->SetKeyValue(0, val); }
  void SetKeyValue(unsigned int index, double val);
  double GetKeyValue() { return this->GetKeyValue(0); }
  double GetKeyValue(unsigned int index);

  // Description:
  // Removes all key values.
  void RemoveAllKeyValues();

  // Description:
  // Set/Get the number of key values this key frame currently stores.
  unsigned int GetNumberOfKeyValues();
  void SetNumberOfKeyValues(unsigned int num);
  
  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);
  
  // Description:
  // Overridden to call MarkAllPropertiesAsModified().
  virtual void Copy(vtkSMProxy* src, const char* exceptionClass, 
    int proxyPropertyCopyFlag);
  void Copy(vtkSMProxy* src, const char* exceptionClass)
    { this->Superclass::Copy(src, exceptionClass); }
protected:
  vtkSMKeyFrameProxy();
  ~vtkSMKeyFrameProxy();

  // Description:
  // This method is complementary to CreateVTKObjects() when we want the
  // proxy to reuse the VTK object ID are defined. When reviving a proxy
  // on the client, this method creates new client-side objects and reuses
  // the server side objects.  When reviving a proxy on the server, it
  // reuses the server objects while creating new "client-only" objects on
  // the server itself.
  virtual void ReviveVTKObjects()
    { this->CreateVTKObjects(); }

  double KeyTime;
  vtkSMKeyFrameProxyInternals* Internals;

private:
  vtkSMKeyFrameProxy(const vtkSMKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMKeyFrameProxy&); // Not implemented.
};

#endif

