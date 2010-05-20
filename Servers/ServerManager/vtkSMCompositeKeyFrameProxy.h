/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeKeyFrameProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompositeKeyFrameProxy - composite keyframe proxy.
// .SECTION Description
// There are many different types of keyframes such as
// vtkSMSinusoidKeyFrameProxy, vtkSMRampKeyFrameProxy etc. 
// This is keyframe proxy that has all different types of keyframes
// as subproxies and provides API to choose one of them as the
// active type. This is helpful in GUIs that allow for switching the 
// type of keyframe on the fly without much effort from the GUI.

#ifndef __vtkSMCompositeKeyFrameProxy_h
#define __vtkSMCompositeKeyFrameProxy_h

#include "vtkSMKeyFrameProxy.h"

class vtkSMPropertyLink;

class VTK_EXPORT vtkSMCompositeKeyFrameProxy : public vtkSMKeyFrameProxy
{
public:
  static vtkSMCompositeKeyFrameProxy* New();
  vtkTypeMacro(vtkSMCompositeKeyFrameProxy, vtkSMKeyFrameProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  //BTX
  enum
    {
    NONE =0,
    BOOLEAN=1,
    RAMP=2,
    EXPONENTIAL=3,
    SINUSOID=4
    };
  //ETX

  // Description:
  // Get/Set the type of keyframe to be used as the active type.
  // Default is RAMP.
  vtkSetClampMacro(Type, int, NONE, SINUSOID);
  vtkGetMacro(Type, int);
  const char* GetTypeAsString() { return this->GetTypeAsString(this->Type); }
  static const char* GetTypeAsString(int);
  static int GetTypeFromString(const char* string);

  // Description:
  // This method will do the actual interpolation.
  // currenttime is normalized to the time range between
  // this key frame and the next key frame.
  virtual void UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next);

protected:
  vtkSMCompositeKeyFrameProxy();
  ~vtkSMCompositeKeyFrameProxy();

  void InvokeModified()
    {
    this->Modified();
    }


  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects();

  vtkSMPropertyLink* TimeLink;
  vtkSMPropertyLink* ValueLink;
  int Type;
private:
  vtkSMCompositeKeyFrameProxy(const vtkSMCompositeKeyFrameProxy&); // Not implemented.
  void operator=(const vtkSMCompositeKeyFrameProxy&); // Not implemented.
};

#endif

