/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWXtEmbeddedWidget.h
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
// .NAME vtkKWXtEmbeddedWidget - dialog box superclass
// .SECTION Description
// A class that can be used to embed KW widgets in Motif or Xt windows.
// If the WindowId is not set, a new top level window is created.

#ifndef __vtkKWXtEmbeddedWidget_h
#define __vtkKWXtEmbeddedWidget_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWXtEmbeddedWidget : public vtkKWWidget
{
public:
  static vtkKWXtEmbeddedWidget* New();
  vtkTypeMacro(vtkKWXtEmbeddedWidget,vtkKWWidget);

  // Description:
  // Set the window ID to embed the widget in.
  // If this is not set, a toplevel window is created
  void SetWindowId(void* w) { this->WindowId = w;}
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // display the widget
  virtual void Display();

protected:
  vtkKWXtEmbeddedWidget();
  ~vtkKWXtEmbeddedWidget();
  vtkKWXtEmbeddedWidget(const vtkKWXtEmbeddedWidget&) {};
  void operator=(const vtkKWXtEmbeddedWidget&) {};

  void* WindowId;
};


#endif


