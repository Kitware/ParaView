/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSelectPointInteractor.h
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
  
  void SetCursorVisibility(int value);

  void SetPVProbe(vtkPVProbe *probe);
  
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
