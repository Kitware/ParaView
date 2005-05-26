/*=========================================================================

  Module:    vtkKWCheckButtonWithChangeColor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWCheckButtonWithChangeColor - a check button and color change button
// .SECTION Description
// This packs a checkbutton and a color change button inside a frame

#ifndef __vtkKWCheckButtonWithChangeColor_h
#define __vtkKWCheckButtonWithChangeColor_h

#include "vtkKWWidget.h"

class vtkKWChangeColorButton;
class vtkKWCheckButton;

class KWWIDGETS_EXPORT vtkKWCheckButtonWithChangeColor : public vtkKWWidget
{
public:
  static vtkKWCheckButtonWithChangeColor* New();
  vtkTypeRevisionMacro(vtkKWCheckButtonWithChangeColor, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Get the internal objects
  vtkGetObjectMacro(CheckButton, vtkKWCheckButton);
  vtkGetObjectMacro(ChangeColorButton, vtkKWChangeColorButton);
  
  // Description:
  // Refresh the interface given the current value of the widgets and Ivars
  virtual void Update();

  // Description:
  // Disable the color button when the checkbutton is not checked.
  // You will have to call the Update() method manually though, to reflect
  // that state.
  virtual void SetDisableChangeColorButtonWhenNotChecked(int);
  vtkBooleanMacro(DisableChangeColorButtonWhenNotChecked, int);
  vtkGetMacro(DisableChangeColorButtonWhenNotChecked, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWCheckButtonWithChangeColor();
  ~vtkKWCheckButtonWithChangeColor();

  vtkKWCheckButton       *CheckButton;
  vtkKWChangeColorButton *ChangeColorButton;

  int DisableChangeColorButtonWhenNotChecked;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWCheckButtonWithChangeColor(const vtkKWCheckButtonWithChangeColor&); // Not implemented
  void operator=(const vtkKWCheckButtonWithChangeColor&); // Not implemented
};

#endif

