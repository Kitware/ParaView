/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVKeyFrameAnimationCue
// .SECTION Description
// vtkPVKeyFrameAnimationCue is a specialization of vtkPVAnimationCue that uses
// the vtkPVKeyFrameCueManipulator as the manipulator.

#ifndef __vtkPVKeyFrameAnimationCue_h
#define __vtkPVKeyFrameAnimationCue_h

#include "vtkPVAnimationCue.h"

class vtkPVKeyFrame;
class vtkPVKeyFrameCueManipulator;

class VTK_EXPORT vtkPVKeyFrameAnimationCue : public vtkPVAnimationCue
{
public:
  vtkTypeMacro(vtkPVKeyFrameAnimationCue, vtkPVAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Forwarded to the internal vtkPVKeyFrameCueManipulator.
  int AddKeyFrame (vtkPVKeyFrame* keyframe);
  int GetLastAddedKeyFrameIndex();
  void RemoveKeyFrame(vtkPVKeyFrame*);
  void RemoveAllKeyFrames();

//BTX
protected:
  vtkPVKeyFrameAnimationCue();
  ~vtkPVKeyFrameAnimationCue();

  vtkPVKeyFrameCueManipulator* GetKeyFrameManipulator();

private:
  vtkPVKeyFrameAnimationCue(const vtkPVKeyFrameAnimationCue&); // Not implemented
  void operator=(const vtkPVKeyFrameAnimationCue&); // Not implemented
//ETX
};

#endif
