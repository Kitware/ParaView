/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWFlyInteractor.h
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
// .NAME vtkKWFlyInteractor
// .SECTION Description
// This widget gets displayed when fly mode is selected.

#ifndef __vtkKWFlyInteractor_h
#define __vtkKWFlyInteractor_h

#include "vtkKWInteractor.h"
#include "vtkCameraInteractor.h"
#include "vtkKWScale.h"
#include "tk.h"

class vtkPVRenderView;

class VTK_EXPORT vtkKWFlyInteractor : public vtkKWInteractor
{
public:
  static vtkKWFlyInteractor* New();
  vtkTypeMacro(vtkKWFlyInteractor,vtkKWInteractor);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // When the active interactor is changed, these methods allow
  // it to change its state.  This may similar to a composite.
  void Select();
  void Deselect();

  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);

protected: 
  vtkKWFlyInteractor();
  ~vtkKWFlyInteractor();
  vtkKWFlyInteractor(const vtkKWFlyInteractor&) {};
  void operator=(const vtkKWFlyInteractor&) {};

  void CreateCursor();
  Tk_Window RenderTkWindow;
  Tk_Cursor PlaneCursor; 

  vtkKWWidget *Label;
  vtkKWScale *SpeedSlider;

  // The vtk object which manipulates the camera.
  vtkCameraInteractor *Helper;

  // Used to signle the fly loop to stop.
  int FlyFlag;
};


#endif


