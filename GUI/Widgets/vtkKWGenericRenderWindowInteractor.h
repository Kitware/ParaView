/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWGenericRenderWindowInteractor - Subclass of vtkGenericRenderWindowInteractor specific to KWWidgets
// .SECTION Description
// vtkKWGenericRenderWindowInteractor provides a way to translate native
// mouse and keyboard events into vtk Events. By calling the methods on
// this class, vtk events will be invoked. This will allow scripting
// languages to use vtkInteractorStyles and 3D widgets.


#ifndef __vtkKWGenericRenderWindowInteractor_h
#define __vtkKWGenericRenderWindowInteractor_h

#include "vtkGenericRenderWindowInteractor.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkKWRenderWidget;

class KWWidgets_EXPORT vtkKWGenericRenderWindowInteractor : public vtkGenericRenderWindowInteractor
{
public:
  static vtkKWGenericRenderWindowInteractor *New();
  vtkTypeRevisionMacro(vtkKWGenericRenderWindowInteractor, vtkGenericRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the renderwidget associated to this interactor.
  // It is used to override the Render() method and allow the interactor styles
  // to communicate with the vtkKWRenderWidget (and subclasses) instance.
  // It is not ref-counted.
  virtual void SetRenderWidget(vtkKWRenderWidget *widget);
  vtkGetObjectMacro(RenderWidget, vtkKWRenderWidget);
  
  // Description:
  // Override Render to render through the widget. 
  // The superclass would call vtkRenderWindow::Render(). We want the
  // vtkKWRenderWidget::Render() method to be called instead. Depending
  // on its RenderMode (interactive, still, print) and various flag it
  // will perform some tests and ultimately called vtkRenderWindow::Render()
  // if needed.
  virtual void Render();
  
protected:
  vtkKWGenericRenderWindowInteractor();
  ~vtkKWGenericRenderWindowInteractor();
  
  vtkKWRenderWidget *RenderWidget;
  
private:
  vtkKWGenericRenderWindowInteractor(const vtkKWGenericRenderWindowInteractor&); // Not implemented
  void operator=(const vtkKWGenericRenderWindowInteractor&); // Not implemented
};

#endif
