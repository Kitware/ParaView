/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWWidget.h
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
// .NAME vtkKWWidget
// .SECTION Description
// This class is the superclass of all UI based objects in the
// Kitware toolkit. It contains common methods such as specifying
// the parent widget, generating and returning the Tcl widget name
// for an instance, and managing children. It overrides the 
// Unregister method to handle circular reference counts between
// child and parent widgets.

#ifndef __vtkKWWidget_h
#define __vtkKWWidget_h

#include "vtkKWObject.h"
#include "vtkKWWidgetCollection.h"

class VTK_EXPORT vtkKWWidget : public vtkKWObject
{
public:
  static vtkKWWidget* New();
  vtkTypeMacro(vtkKWWidget,vtkKWObject);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app,char *name, char *args);

  // Description:
  // Get the name of the underlying tk widget being used
  // the parent should be set before calling this method.
  const char *GetWidgetName();

  // Description:
  // Set/Get the parent widget for this widget
  void SetParent(vtkKWWidget *p);
  vtkGetObjectMacro(Parent,vtkKWWidget);

  // Description:
  // Add/Remove/Get a child to this Widget
  void AddChild(vtkKWWidget *w) {this->Children->AddItem(w);};
  void RemoveChild(vtkKWWidget *w) {this->Children->RemoveItem(w);};
  vtkKWWidgetCollection *GetChildren() {return this->Children;};
  
  // Description::
  // Override Unregister since widgets have loops.
  void UnRegister(vtkObject *o);

  // Description:
  // Get the net reference count of this widget. That is the
  // reference count of this widget minus its children.
  virtual int  GetNetReferenceCount();
  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand( vtkKWObject* Object, const char* MethodAndArgString);
  // Description: a method to create a callback string from a KWObject.
  // The caller is resposible for deleting the returned string.  
  char* CreateCommand(vtkKWObject* Object, const char* MethodAndArgString);

protected:
  vtkKWWidget();
  ~vtkKWWidget();
  vtkKWWidget(const vtkKWWidget&) {};
  void operator=(const vtkKWWidget&) {};

  char *WidgetName;
  vtkKWWidget *Parent;
  vtkKWWidgetCollection *Children; 
  int DeletingChildren;
};


#endif


