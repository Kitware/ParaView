/*=========================================================================

  Module:    vtkKWLabeledOptionMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledOptionMenu - an option menu with a label
// .SECTION Description
// This class creates an option menu with another label in front of it; both are
// contained in a frame
// .SECTION See Also
// vtkKWOptionMenu

#ifndef __vtkKWLabeledOptionMenu_h
#define __vtkKWLabeledOptionMenu_h

#include "vtkKWLabeledWidget.h"

class vtkKWOptionMenu;

class VTK_EXPORT vtkKWLabeledOptionMenu : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledOptionMenu* New();
  vtkTypeRevisionMacro(vtkKWLabeledOptionMenu, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(OptionMenu, vtkKWOptionMenu);

  // Description:
  // Set the widget packing order to be horizontal (default).
  virtual void SetPackHorizontally(int);
  vtkBooleanMacro(PackHorizontally, int);
  vtkGetMacro(PackHorizontally, int);

  // Description:
  // Set the option menu to auto-expand (does not by default).
  virtual void SetExpandOptionMenu(int);
  vtkBooleanMacro(ExpandOptionMenu, int);
  vtkGetMacro(ExpandOptionMenu, int);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledOptionMenu();
  ~vtkKWLabeledOptionMenu();

  vtkKWOptionMenu *OptionMenu;

  int PackHorizontally;
  int ExpandOptionMenu;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledOptionMenu(const vtkKWLabeledOptionMenu&); // Not implemented
  void operator=(const vtkKWLabeledOptionMenu&); // Not implemented
};


#endif

