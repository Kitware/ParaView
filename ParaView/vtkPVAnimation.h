/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimation.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  void SetObject(vtkPVSource *object);
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
