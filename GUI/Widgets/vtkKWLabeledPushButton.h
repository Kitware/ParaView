/*=========================================================================

  Module:    vtkKWLabeledPushButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledPushButton - a push button with a label
// .SECTION Description
// The vtkKWLabeledPushButton class creates a push button with a label in 
// front of it

#ifndef __vtkKWLabeledPushButton_h
#define __vtkKWLabeledPushButton_h

#include "vtkKWLabeledWidget.h"

class vtkKWPushButton;

class VTK_EXPORT vtkKWLabeledPushButton : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledPushButton* New();
  vtkTypeRevisionMacro(vtkKWLabeledPushButton, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Convenience method to set the pushbutton label.
  void SetPushButtonLabel(const char *);
  
  // Description:
  // Get the internal object
  vtkGetObjectMacro(PushButton, vtkKWPushButton);
  
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
  vtkKWLabeledPushButton();
  ~vtkKWLabeledPushButton();

  vtkKWPushButton *PushButton;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledPushButton(const vtkKWLabeledPushButton&); // Not implemented
  void operator=(const vtkKWLabeledPushButton&); // Not implemented
};

#endif

