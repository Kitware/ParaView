/*=========================================================================

  Module:    vtkKWPopupButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPopupButton - a button that triggers a popup
// .SECTION Description
// The vtkKWPopupButton class creates a push button that
// will popup a window. User widgets should be inserted inside
// the PopupFrame ivar.

#ifndef __vtkKWPopupButton_h
#define __vtkKWPopupButton_h

#include "vtkKWPushButton.h"

class vtkKWFrame;

class VTK_EXPORT vtkKWPopupButton : public vtkKWPushButton
{
public:
  static vtkKWPopupButton* New();
  vtkTypeRevisionMacro(vtkKWPopupButton, vtkKWPushButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Access to sub-widgets.
  // The PopupFrame widget is the place to put your own sub-widgets.
  vtkGetObjectMacro(PopupTopLevel, vtkKWWidget);
  vtkGetObjectMacro(PopupFrame, vtkKWFrame);
  vtkGetObjectMacro(PopupCloseButton, vtkKWPushButton);

  // Description:
  // Popup callbacks;
  virtual void DisplayPopupCallback();
  virtual void WithdrawPopupCallback();

  // Description:
  // Set/Get popup title
  virtual void SetPopupTitle(const char* title);
  vtkGetStringMacro(PopupTitle);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWPopupButton();
  ~vtkKWPopupButton();

  vtkKWWidget     *PopupTopLevel;
  vtkKWFrame     *PopupFrame;
  vtkKWPushButton *PopupCloseButton;

  char* PopupTitle;

  virtual void Bind();
  virtual void UnBind();

private:
  vtkKWPopupButton(const vtkKWPopupButton&); // Not implemented
  void operator=(const vtkKWPopupButton&); // Not implemented
};

#endif

