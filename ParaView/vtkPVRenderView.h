/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderView.h
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
// .NAME vtkPVRenderView - For using styles
// .SECTION Description
// I am going to try to divert the events to a vtkInteractorStyle object.

#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkKWRenderView.h"
#include "vtkInteractorStyle.h"
#include "vtkRenderWindowInteractor.h"

class vtkKWApplication;

class VTK_EXPORT vtkPVRenderView : public vtkKWRenderView
{
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView,vtkKWRenderView);

  // Description:
  // Create the TK widgets associated with the view.
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // The events will be forwarded to this style object,
  void SetInteractorStyle(vtkInteractorStyle *style);
  vtkGetObjectMacro(InteractorStyle, vtkInteractorStyle);

  // Description:
  // These are the event handlers that UIs can use or override.
  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);
  void Button1Motion(int x, int y);
  void Button2Motion(int x, int y);
  void Button3Motion(int x, int y);
  void AKeyPress(char key, int x, int y);

  // Description:
  // Special binding added to this subclass.
  void MotionCallback(int x, int y);


protected:
  vtkPVRenderView();
  ~vtkPVRenderView();
  vtkPVRenderView(const vtkPVRenderView&) {};
  void operator=(const vtkPVRenderView&) {};

  vtkInteractorStyle *InteractorStyle;
  vtkRenderWindowInteractor *Interactor;
};


#endif


