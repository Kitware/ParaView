/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeAnimationCueProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTimeAnimationCueProxy - an animation cue that can be used to 
// animate pipeline time.
// .SECTION Description
// This is the animation cue that can be used to animate pipeline time. It's a
// typical animation cue, except that it adds a new attribute 
// "UseAnimationTime" which when enabled, updates the animated property directly
// by using the current animation clock time (disregarding any manipulator i.e.
// keyframes).

#ifndef __vtkSMTimeAnimationCueProxy_h
#define __vtkSMTimeAnimationCueProxy_h

#include "vtkSMAnimationCueProxy.h"

class VTK_EXPORT vtkSMTimeAnimationCueProxy : public vtkSMAnimationCueProxy
{
public:
  static vtkSMTimeAnimationCueProxy* New();
  vtkTypeMacro(vtkSMTimeAnimationCueProxy, vtkSMAnimationCueProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When enabled, the animated property is directly updated with the value of
  // the Animation clock. Default value it true.
  vtkSetMacro(UseAnimationTime, int);
  vtkGetMacro(UseAnimationTime, int);

//BTX
protected:
  vtkSMTimeAnimationCueProxy();
  ~vtkSMTimeAnimationCueProxy();

  // Description:
  // Callbacks for corresponding Cue events. The argument must be 
  // casted to vtkAnimationCue::AnimationCueInfo.
  // Overridden to directly use the animation clock time when UseAnimationTime
  // is on.
  virtual void TickInternal(void* info);

  int UseAnimationTime;
private:
  vtkSMTimeAnimationCueProxy(const vtkSMTimeAnimationCueProxy&); // Not implemented
  void operator=(const vtkSMTimeAnimationCueProxy&); // Not implemented
//ETX
};

#endif

