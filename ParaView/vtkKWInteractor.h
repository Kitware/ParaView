/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWInteractor.h
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
// .NAME vtkKWInteractor
// .SECTION Description
// This is the superclass for interactors used by vtkCadInteractorPanel

#ifndef __vtkKWInteractor_h
#define __vtkKWInteractor_h

#include "vtkKWWidget.h"
#include "vtkKWRadioButton.h"
class vtkKWToolbar;
class vtkPVRenderView;

class VTK_EXPORT vtkKWInteractor : public vtkKWWidget
{
public:
  static vtkKWInteractor* New() {return new vtkKWInteractor;};
  vtkTypeMacro(vtkKWInteractor,vtkKWWidget);

  // Description:
  // This does nothing but create the widgets frame.
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // These methods allow the Composite to be turned on and off.
  // It is up to the subclasses to determine what that means.
  // It could be adding special actors to the renderer, changing
  // cursors, or adding and enabling UI features.
  virtual void Select();
  virtual void Deselect();

  // Description:
  // Composites may want to add actors to the renderer.
  virtual void SetRenderView(vtkPVRenderView *view);

  // Description:
  // Setting this reference causes this superclass to manage
  // the buttons state with the selected state of the composite.
  vtkSetObjectMacro(ToolbarButton, vtkKWRadioButton);
  vtkGetObjectMacro(ToolbarButton, vtkKWRadioButton);

  // Description:
  // The render view forards these messages.
  virtual void AButtonPress(int num, int x, int y) {};
  virtual void AButtonRelease(int num, int x, int y) {};
  virtual void Button1Motion(int x, int y) {};
  virtual void Button2Motion(int x, int y) {};
  virtual void Button3Motion(int x, int y) {};
  virtual void AKeyPress(char key, int x, int y) {};

  // Description:
  // Get rid of all references.  A quick and dirty way
  // of dealing with reference loops.
  virtual void PrepareForDelete() {};

protected:
  vtkKWInteractor();
  ~vtkKWInteractor();
  vtkKWInteractor(const vtkKWInteractor&) {};
  void operator=(const vtkKWInteractor&) {};

  int SelectedState;
  vtkPVRenderView *RenderView;

  // If set, select and deselect will set its state.
  // The button does not belong to the toolbar below.
  // The button should be used for selecting the composite.
  vtkKWRadioButton *ToolbarButton;

  // If the composite has a toolbar, the this super class
  // will manage packing the toolbar with our selection status.
  vtkKWToolbar *Toolbar;
};


#endif


