/*=========================================================================

  Module:    vtkKWPopupFrameCheckButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPopupFrameCheckButton - a popup frame + checkbutton
// .SECTION Description
// A class that provides a checkbutton and a (popup) frame. In popup mode
// the checkbutton is visible on the left of the popup button that will
// display the frame. In normal mode, the checkbutton is the first item
// packed in the frame.

#ifndef __vtkKWPopupFrameCheckButton_h
#define __vtkKWPopupFrameCheckButton_h

#include "vtkKWPopupFrame.h"

class vtkKWCheckButton;

class VTK_EXPORT vtkKWPopupFrameCheckButton : public vtkKWPopupFrame
{
public:
  static vtkKWPopupFrameCheckButton* New();
  vtkTypeRevisionMacro(vtkKWPopupFrameCheckButton,vtkKWPopupFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Automatically disable the popup button when the checkbutton is not 
  // checked.
  virtual void SetLinkPopupButtonStateToCheckButton(int);
  vtkBooleanMacro(LinkPopupButtonStateToCheckButton, int);
  vtkGetMacro(LinkPopupButtonStateToCheckButton, int);

  // Description:
  // Callbacks
  virtual void CheckButtonCallback();

  // Description:
  // Access to sub-widgets
  vtkGetObjectMacro(CheckButton, vtkKWCheckButton);

  // Description:
  // Update the GUI according to the value of the ivars
  virtual void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWPopupFrameCheckButton();
  ~vtkKWPopupFrameCheckButton();

  // GUI

  int                     LinkPopupButtonStateToCheckButton;

  vtkKWCheckButton        *CheckButton;

  // Get the value that should be used to set the checkbutton state
  // (i.e. depending on the value this checkbutton is supposed to reflect,
  // for example, an annotation visibility).
  // This does *not* return the state of the widget.
  virtual int GetCheckButtonState() { return 0; };

private:
  vtkKWPopupFrameCheckButton(const vtkKWPopupFrameCheckButton&); // Not implemented
  void operator=(const vtkKWPopupFrameCheckButton&); // Not Implemented
};

#endif
