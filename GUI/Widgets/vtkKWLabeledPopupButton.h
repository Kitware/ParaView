/*=========================================================================

  Module:    vtkKWLabeledPopupButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledPopupButton - a popup button with a label
// .SECTION Description
// The vtkKWLabeledPopupButton class creates a popup button with a label in 
// front of it

#ifndef __vtkKWLabeledPopupButton_h
#define __vtkKWLabeledPopupButton_h

#include "vtkKWLabeledWidget.h"

class vtkKWPopupButton;

class VTK_EXPORT vtkKWLabeledPopupButton : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledPopupButton* New();
  vtkTypeRevisionMacro(vtkKWLabeledPopupButton, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Convenience method to set the pushbutton label.
  void SetPopupButtonLabel(const char *);
  
  // Description:
  // Get the internal object
  vtkGetObjectMacro(PopupButton, vtkKWPopupButton);
  
  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Pack the label last
  virtual void SetPackLabelLast(int);
  vtkGetMacro(PackLabelLast, int);
  vtkBooleanMacro(PackLabelLast, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledPopupButton();
  ~vtkKWLabeledPopupButton();

  int PackLabelLast;

  vtkKWPopupButton *PopupButton;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledPopupButton(const vtkKWLabeledPopupButton&); // Not implemented
  void operator=(const vtkKWLabeledPopupButton&); // Not implemented
};

#endif

