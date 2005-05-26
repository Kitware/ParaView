/*=========================================================================

  Module:    vtkKWMenuButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkKWMenuButton_h
#define __vtkKWMenuButton_h

#include "vtkKWWidget.h"

class vtkKWMenu;

class KWWIDGETS_EXPORT vtkKWMenuButton : public vtkKWWidget
{
public:
  static vtkKWMenuButton* New();
  vtkTypeRevisionMacro(vtkKWMenuButton, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Add text to the button
  void SetButtonText(const char *text);
  const char* GetButtonText();
  
  // Description: 
  // Append a standard menu item and command to the current menu.
  void AddCommand(const char* label, vtkKWObject* Object,
                  const char* MethodAndArgString , const char* help = 0);
  
  // Description: 
  // Access to the menu
  vtkKWMenu* GetMenu();
  
  // Description
  // Set the indicator On/Off. To be called after creation.
  void IndicatorOn();
  void IndicatorOff();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWMenuButton();
  ~vtkKWMenuButton();
  
  vtkKWMenu *Menu;

private:
  vtkKWMenuButton(const vtkKWMenuButton&); // Not implemented
  void operator=(const vtkKWMenuButton&); // Not implemented
};

#endif

