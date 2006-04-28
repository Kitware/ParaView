/*=========================================================================

  Module:    vtkKWWidgetWithSpinButtons.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWidgetWithSpinButtons - an abstract class widget with spin buttons
// .SECTION Description
// This class implements an abstract superclass for composite widgets
// associating a widget to a set of spin buttons.
// The only requirement is for the widget to implement the NextValue()
// and PreviousValue() callbacks.

#ifndef __vtkKWWidgetWithSpinButtons_h
#define __vtkKWWidgetWithSpinButtons_h

#include "vtkKWCompositeWidget.h"

class vtkKWSpinButtons;

class KWWidgets_EXPORT vtkKWWidgetWithSpinButtons : public vtkKWCompositeWidget
{
public:
  static vtkKWWidgetWithSpinButtons* New();
  vtkTypeRevisionMacro(vtkKWWidgetWithSpinButtons, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Retrieve the spin buttons
  vtkGetObjectMacro(SpinButtons, vtkKWSpinButtons);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void NextValueCallback() {};
  virtual void PreviousValueCallback() {};

protected:
  vtkKWWidgetWithSpinButtons();
  ~vtkKWWidgetWithSpinButtons();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // Description:
  // Pack or repack the widget. To be implemented by subclasses.
  virtual void Pack() {};

  // Description:
  // Internal spin buttons
  vtkKWSpinButtons *SpinButtons;

private:

  vtkKWWidgetWithSpinButtons(const vtkKWWidgetWithSpinButtons&); // Not implemented
  void operator=(const vtkKWWidgetWithSpinButtons&); // Not implemented
};

#endif
