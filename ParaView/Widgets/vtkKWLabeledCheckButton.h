/*=========================================================================

  Module:    vtkKWLabeledCheckButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledCheckButton - a check button with a label
// .SECTION Description
// The vtkKWLabeledCheckButton class creates a check button with a label in 
// front of it

#ifndef __vtkKWLabeledCheckButton_h
#define __vtkKWLabeledCheckButton_h

#include "vtkKWLabeledWidget.h"

class vtkKWCheckButton;

class VTK_EXPORT vtkKWLabeledCheckButton : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledCheckButton* New();
  vtkTypeRevisionMacro(vtkKWLabeledCheckButton, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal object
  vtkGetObjectMacro(CheckButton, vtkKWCheckButton);
  
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
  vtkKWLabeledCheckButton();
  ~vtkKWLabeledCheckButton();

  vtkKWCheckButton *CheckButton;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledCheckButton(const vtkKWLabeledCheckButton&); // Not implemented
  void operator=(const vtkKWLabeledCheckButton&); // Not implemented
};

#endif

