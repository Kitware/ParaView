/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWComposite.h
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
// .NAME vtkKWComposite
// .SECTION Description
// A composite represents an element in the view. It is very similar to
// the notion of an actor in a renderer. The composite is different in 
// that it combines the Actor (vtkProp actually) with some user interface
// code called the properties, and it may also contain more complex
// elements such as filters etc.

#ifndef __vtkKWComposite_h
#define __vtkKWComposite_h

#include "vtkKWNotebook.h"
class vtkKWView;
class vtkKWApplication;
class vtkProp;
class vtkKWWidget;

class VTK_EXPORT vtkKWComposite : public vtkKWObject
{
public:
  vtkKWComposite();
  ~vtkKWComposite();
  const char *GetClassName() {return "vtkKWComposite";};

  // Description:
  // Get the View for this class.
  vtkGetObjectMacro(View,vtkKWView);
  virtual void SetView(vtkKWView *view);

  // Description:
  // Get the Prop for this class.
  virtual vtkProp *GetProp() = 0;

  // Description:
  // Initialize properties should be called by any methods
  // that rely on the proerties being created first.
  void InitializeProperties();

  // Description:
  // Create the properties object, called by InitializeProperties.
  virtual void CreateProperties();

  // Description:
  // Methods to indicate when this composite is the selected composite.
  // These methods are used by subclasses to modify the menu bar
  // for example. When a volume composite is selected it might 
  // add an option to the menu bar to view the 2D slices.
  virtual void Select(vtkKWView *);
  virtual void Deselect(vtkKWView *) {};
  virtual void MakeSelected();

  // Description:
  // Allow access to the notebook objects.
  vtkGetObjectMacro(Notebook,vtkKWNotebook);
  vtkGetObjectMacro(Notebook2,vtkKWNotebook);

  // Description:
  // Give a composite a chance to close. Called from the vtkKWView when
  // it is closed. The default behavior is to do nothing - can be
  // overridden in a subclass for example to close dialogs.
  virtual void Close() {};

protected:
  vtkKWNotebook *Notebook;
  vtkKWNotebook *Notebook2;
  int PropertiesCreated;
  vtkKWWidget *TopLevel;
  vtkKWView *View;
};


#endif


