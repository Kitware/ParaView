/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTranslateCameraInteractor.h
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
// .NAME vtkKWTranslateCameraInteractor
// .SECTION Description
// This is not much of a widget, but it works with a panel
// to enable the user to xy translate the camera as well as zoom.
// It zooms when the top third or bottom third
// of the screen is first selected.  The middle third pans xy.

#ifndef __vtkKWTranslateCameraInteractor_h
#define __vtkKWTranslateCameraInteractor_h

#include "vtkKWInteractor.h"
#include "vtkCameraInteractor.h"

class vtkPVRenderView;

class VTK_EXPORT vtkKWTranslateCameraInteractor : public vtkKWInteractor
{
public:
  static vtkKWTranslateCameraInteractor* New();
  vtkTypeMacro(vtkKWTranslateCameraInteractor,vtkKWInteractor);

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
  void Button1Motion(int x, int y);
  void Button3Motion(int x, int y);

  // Changes the cursor based on mouse position.
  void MotionCallback(int x, int y);

protected: 
  vtkKWTranslateCameraInteractor();
  ~vtkKWTranslateCameraInteractor();
  vtkKWTranslateCameraInteractor(const vtkKWTranslateCameraInteractor&) {};
  void operator=(const vtkKWTranslateCameraInteractor&) {};

  vtkKWWidget *Label;

  // The vtk object which manipulates the camera.
  vtkCameraInteractor *Helper;

  void InitializeCursors();
  char *PanCursorName;
  char *ZoomCursorName;
  vtkSetStringMacro(PanCursorName);
  vtkSetStringMacro(ZoomCursorName);
  int CursorState;
};


#endif


