/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSlaveProp.h
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
// .NAME vtkPVSlaveProp - A prop to represent the remote renderers.
// .SECTION Description
// vtkPVSlaveProp uses "RenderIntoImage" feature to encapsulate the
// results from remote renderers into a main UI renderer.

#ifndef __vtkPVSlaveProp_h
#define __vtkPVSlaveProp_h

#include "vtkProp.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"

class VTK_EXPORT vtkPVSlaveProp : public vtkProp
{
public:
  static vtkPVSlaveProp* New();
  vtkTypeMacro(vtkPVSlaveProp,vtkProp);

  vtkSetObjectMacro(Application, vtkPVApplication);
  vtkGetObjectMacro(Application, vtkPVApplication);  

  // Description:
  // We are going to try and manipulate the render window directly.
  // this assumes the renderer fills the whole render window.
  vtkSetObjectMacro(RenderWindow, vtkRenderWindow);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  
  int RequiresRenderingIntoImage() { return 1; };
  int RenderIntoImage(vtkViewport *viewport);
  float *GetRGBAImage() {return this->RGBAImage;};
  
  void SetupTest();
  
protected:
  vtkPVSlaveProp();
  ~vtkPVSlaveProp();
  vtkPVSlaveProp(const vtkPVSlaveProp&) {};
  void operator=(const vtkPVSlaveProp&) {};

  vtkPVApplication *Application;
  vtkRenderWindow *RenderWindow;
  
  float *RGBAImage;
};

#endif


