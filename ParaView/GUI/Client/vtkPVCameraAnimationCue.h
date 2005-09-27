/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraAnimationCue.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraAnimationCue - GUI for Camera.
// .SECTION Description
// Animation Cues are designed to animate single properties for a proxy.
// However, in case of Camera, we are animating the entire proxy. Hence, 
// we have a special cue.
// We have a special animation cue manipulator for camera interpolation.

#ifndef __vtkPVCameraAnimationCue_h
#define __vtkPVCameraAnimationCue_h

#include "vtkPVAnimationCue.h"

class vtkPVKeyFrame;
class vtkSMProxy;

class VTK_EXPORT vtkPVCameraAnimationCue : public vtkPVAnimationCue
{
public:
  static vtkPVCameraAnimationCue* New();
  vtkTypeRevisionMacro(vtkPVCameraAnimationCue, vtkPVAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Replace key frame is called when the type of the key frame is changed.
  // CameraAnimationCue supports only one type of keyframe, hence
  // we override this method. We may allow this if we start supporting
  // different types of camera interpolations.
  virtual void ReplaceKeyFrame(vtkPVKeyFrame*, vtkPVKeyFrame*);

  // Description:
  // Method to query if the animation cue supports the given type of
  // key frame. Rturns true for all Camera keyframes.
  virtual int IsKeyFrameTypeSupported(int type);

  // Description:
  // Sets the animated proxy (must be a camera proxy or a proxy
  // that has camera proxy as subproxy with its properties exposed).
  // Overridden to set up the vtkSMPropertyStatusManager
  // to monitor the status of all the concerned properties.
  virtual void SetAnimatedProxy(vtkSMProxy* proxy);

  // Description:
  // Overridden to avoid initialization of vtkSMPropertyStatusManager
  // which this class initializes on SetAnimatedProxy.
  virtual void SetAnimatedPropertyName(const char*) { }

  // Description:
  // Recording methods.
  virtual void StartRecording();
  virtual void RecordState(double ntime, double offset, int onlyfocus);
  virtual void RecordState(double ntime, double offset);
protected:
  vtkPVCameraAnimationCue();
  ~vtkPVCameraAnimationCue();

private:
  vtkPVCameraAnimationCue(const vtkPVCameraAnimationCue&); // Not implemented.
  void operator=(const vtkPVCameraAnimationCue&); // Not implemented.
};

#endif

