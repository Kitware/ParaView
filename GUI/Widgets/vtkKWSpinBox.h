/*=========================================================================

  Module:    vtkKWSpinBox.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSpinBox - SpinBox
// .SECTION Description
// A widget with up and down arrow controls and direct text editing.
// Typically used with integer fields that users increment by 1 (or
// decrement) by clicking on the arrows.

#ifndef __vtkKWSpinBox_h
#define __vtkKWSpinBox_h

#include "vtkKWCoreWidget.h"

class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWSpinBox : public vtkKWCoreWidget
{
public:
  static vtkKWSpinBox* New();
  vtkTypeRevisionMacro(vtkKWSpinBox,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the range.
  virtual void SetRange(double from, double to);

  // Description:
  // Set the increment value.
  virtual void SetIncrement(double increment);

  // Description:
  // Set/Get the current value.
  virtual void SetValue(double value);
  virtual double GetValue();

  // Description:
  // Set/Get the string used to format the value.
  virtual void SetValueFormat(const char *format);

  // Description:
  // Set/Get the wrap. If on, values at edges of range wrap around to the
  // other side of the range when clicking on the up/down arrows.
  virtual void SetWrap(int wrap);
  virtual int GetWrap();
  vtkBooleanMacro(Wrap, int);

  // Description:
  // Prevent the user from typing in non-integer values.
  virtual void SetRestrictValuesToIntegers(int restrict);
  vtkBooleanMacro(RestrictValuesToIntegers, int);

  // Description:
  // Set/Get the width of the spinbox in number of characters.
  virtual void SetWidth(int);
  virtual int GetWidth();

  // Description:
  // Specifies whether or not a selection in the widget should also be the X
  // selection. If the selection is exported, then selecting in the widget
  // deselects the current X selection, selecting outside the widget deselects
  // any widget selection, and the widget will respond to selection retrieval
  // requests when it has a selection.  
  virtual void SetExportSelection(int);
  virtual int GetExportSelection();
  vtkBooleanMacro(ExportSelection, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWSpinBox();
  ~vtkKWSpinBox();

private:
  vtkKWSpinBox(const vtkKWSpinBox&); // Not implemented
  void operator=(const vtkKWSpinBox&); // Not implemented
};

#endif
