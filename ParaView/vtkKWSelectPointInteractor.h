/*=========================================================================

  Program:   ParaView
  Module:    vtkKWSelectPointInteractor.h
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
// .NAME vtkKWSelectPointInteractor
// .SECTION Description
// This is the interactor used by vtkPVProbeFilter to select points.

#ifndef __vtkKWSelectPointInteractor_h
#define __vtkKWSelectPointInteractor_h

#include "vtkKWInteractor.h"
#include "vtkCursor3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

class vtkPVProbe;

class VTK_EXPORT vtkKWSelectPointInteractor : public vtkKWInteractor
{
public:
  static vtkKWSelectPointInteractor* New();
  vtkTypeMacro(vtkKWSelectPointInteractor,vtkKWInteractor);

  // Description:
  // Set/Get the selected point.
  vtkGetVector3Macro(SelectedPoint, float);
  void SetSelectedPoint(float point[3]);
  void SetSelectedPoint(float X, float Y, float Z);
  void SetSelectedPointX(float X);
  void SetSelectedPointY(float Y);
  void SetSelectedPointZ(float Z);  
  
  void SetBounds(float bounds[6]);

  // mouse callbacks
  void MotionCallback(int x, int y);
  void Button1Motion(int x, int y);
  
  void SetPVProbe(vtkPVProbe *probe);
  void SetCursorVisibility(int value);
  
protected:
  vtkKWSelectPointInteractor();
  ~vtkKWSelectPointInteractor();
  vtkKWSelectPointInteractor(const vtkKWSelectPointInteractor&) {};
  void operator=(const vtkKWSelectPointInteractor&) {};

  void GetSphereCoordinates(int i, float coords[3]);
  void ColorSphere(int i, float rgb[3]);
  
  float SelectedPoint[3];
  vtkCursor3D *Cursor;
  vtkPolyDataMapper *CursorMapper;
  vtkActor *CursorActor;
  vtkActor *XSphere1Actor; // id = 0
  vtkActor *XSphere2Actor; // id = 1
  vtkActor *YSphere1Actor; // id = 2
  vtkActor *YSphere2Actor; // id = 3
  vtkActor *ZSphere1Actor; // id = 4
  vtkActor *ZSphere2Actor; // id = 5
  int CurrentSphereId;
  float Bounds[6];
  vtkPVProbe *PVProbe;
};

#endif
