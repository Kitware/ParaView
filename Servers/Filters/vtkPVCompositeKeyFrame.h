/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeKeyFrame - composite keyframe proxy.
// .SECTION Description
// There are many different types of keyframes such as
// vtkSMSinusoidKeyFrameProxy, vtkSMRampKeyFrameProxy etc.
// This is keyframe proxy that has all different types of keyframes
// as subproxies and provides API to choose one of them as the
// active type. This is helpful in GUIs that allow for switching the
// type of keyframe on the fly without much effort from the GUI.

#ifndef __vtkPVCompositeKeyFrame_h
#define __vtkPVCompositeKeyFrame_h

#include "vtkPVKeyFrame.h"

class vtkSMPropertyLink;

class VTK_EXPORT vtkPVCompositeKeyFrame : public vtkPVKeyFrame
{
public:
  static vtkPVCompositeKeyFrame* New();
  vtkTypeMacro(vtkPVCompositeKeyFrame, vtkPVKeyFrame);
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
  virtual void UpdateValue( double currenttime,
                            vtkPVAnimationCue* cue, vtkPVKeyFrame* next);

protected:
  vtkPVCompositeKeyFrame();
  ~vtkPVCompositeKeyFrame();

  void InvokeModified()
    {
    this->Modified();
    }


  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects();

  // FIXME ++++++++++++++++++++++++++++++++++
  //vtkSMPropertyLink* TimeLink;
  //vtkSMPropertyLink* ValueLink;
  int Type;

  vtkPVBooleanKeyFrame* BooleanKeyFrame;
  vtkPVRampKeyFrame* RampKeyFrame;
  vtkPVExponentialKeyFrame* ExponentialKeyFrame;
  vtkPVSinusoidKeyFrame* SinusoidKeyFrame;


private:
  vtkPVCompositeKeyFrame(const vtkPVCompositeKeyFrame&); // Not implemented.
  void operator=(const vtkPVCompositeKeyFrame&); // Not implemented.
};

#endif
