/*=========================================================================

  Module:    vtkKWPushButtonWithMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPushButtonWithMenu - PushButton with left mouse bound to menu.
// .SECTION Description
// vtkKWPushButtonWithMenu was created for the reset view.  The menu
// will change the behavior of the button.

#ifndef __vtkKWPushButtonWithMenu_h
#define __vtkKWPushButtonWithMenu_h

#include "vtkKWPushButton.h"

class vtkKWMenu;

class KWWidgets_EXPORT vtkKWPushButtonWithMenu : public vtkKWPushButton
{
public:
  static vtkKWPushButtonWithMenu* New();
  vtkTypeRevisionMacro(vtkKWPushButtonWithMenu, vtkKWPushButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();
    
  // Description: 
  // Access to the menu
  virtual vtkKWMenu* GetMenu();
  
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
  virtual void PopupCallback(int x, int y);

protected:
  vtkKWPushButtonWithMenu();
  ~vtkKWPushButtonWithMenu();
  
  vtkKWMenu *Menu;

private:
  vtkKWPushButtonWithMenu(const vtkKWPushButtonWithMenu&); // Not implemented
  void operator=(const vtkKWPushButtonWithMenu&); // Not implemented
};

#endif

