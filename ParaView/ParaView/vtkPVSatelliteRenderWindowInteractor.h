/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSatelliteRenderWindowInteractor.h
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
// .NAME vtkPVSatelliteRenderWindowInteractor
// .SECTION Description
// This interactor is going to be on the satellite processes.
// The main interactor will forward events to this interactor.
// It drives the 3D widgets, but not the camera interactors.
// It also does not render.

#ifndef __vtkPVSatelliteRenderWindowInteractor_h
#define __vtkPVSatelliteRenderWindowInteractor_h

#include "vtkGenericRenderWindowInteractor.h"

class vtkPVRenderView;

class VTK_EXPORT vtkPVSatelliteRenderWindowInteractor : public vtkGenericRenderWindowInteractor
{
public:
  static vtkPVSatelliteRenderWindowInteractor *New();
  vtkTypeRevisionMacro(vtkPVSatelliteRenderWindowInteractor, vtkGenericRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  void LeftPress(int x, int y, int control, int shift);
  void MiddlePress(int x, int y, int control, int shift);
  void RightPress(int x, int y, int control, int shift);
  void LeftRelease(int x, int y, int control, int shift);
  void MiddleRelease(int x, int y, int control, int shift);
  void RightRelease(int x, int y, int control, int shift);
  void Move(int x, int y);

  virtual void Render();
  
protected:
  vtkPVSatelliteRenderWindowInteractor();
  ~vtkPVSatelliteRenderWindowInteractor();
  
  vtkPVRenderView *PVRenderView;

private:
  vtkPVSatelliteRenderWindowInteractor(const vtkPVSatelliteRenderWindowInteractor&); // Not implemented
  void operator=(const vtkPVSatelliteRenderWindowInteractor&); // Not implemented
};

#endif
