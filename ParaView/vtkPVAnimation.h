/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkPVAnimation - An object that controlls an animation.
// .SECTION Description
// This object will have a link to a vtkPVSource and a method.
// It will create an animation byu changing that method and rendering 
// in a loop.  vtkPVAnimtion may have a polydata timeline to display 
// to make it like other sources.  I am going to have to find a way to 
// choose the vtkPVSource to control.  It is different than the input 
// selection list I have been considering (source vs data object)?
// I imagine it will default to the current source when the animation
// object was created.  Selecting a method will be harder.  Mainly
// because the method can be for the PVSource or the VTK source.
// I am going to have to make the parameter widget list accessible.
// The object may also be able to save images or an AVI of the animation.

#ifndef __vtkPVAnimation_h
#define __vtkPVAnimation_h

#include "vtkPVSource.h"

class VTK_EXPORT vtkPVAnimation : public vtkPVSource
{
public:
  static vtkPVAnimation* New();
  vtkTypeMacro(vtkPVAnimation, vtkPVSource);

  // Description:
  // You have to clone this object before you can create it.
  void CreateProperties();

  // Description:
  // Access to the animation parmeters
  vtkSetMacro(Start, float);
  vtkGetMacro(Start, float);
  vtkSetMacro(End, float);
  vtkGetMacro(End, float);
  vtkSetMacro(Step, float);
  vtkGetMacro(Step, float);
  void SetCurrent(float time);
  vtkGetMacro(Current, float);

  // Description:
  // Choosing a method.  We should come up with a better way.
  vtkSetStringMacro(Method);
  vtkGetStringMacro(Method);

  // Description:
  // The object which is bieng manipulated.
  vtkSetObjectMacro(Object,vtkPVSource);
  vtkGetObjectMacro(Object,vtkPVSource);

  // Description:
  // We need to changed the slider when the Min/max animation time changes.
  void AcceptCallback();

  // Description:
  // Callback that starts an animation.
  void Play();

protected:
  vtkPVAnimation();
  ~vtkPVAnimation();
  vtkPVAnimation(const vtkPVAnimation&) {};
  void operator=(const vtkPVAnimation&) {};

  vtkPVSource *Object;
  char *Method;

  // Animation parameters
  float Start;
  float End;
  float Step;
  float Current;

  // UI elements
  vtkKWEntry *TimeMin;
  vtkKWEntry *TimeMax;
  vtkKWEntry *TimeStep;
  vtkKWScale *TimeScale;
};

#endif
