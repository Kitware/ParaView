/*=========================================================================

  Module:    vtkKWOptionMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWOptionMenu - an option menu widget
// .SECTION Description
// A widget that looks like a button but when pressed provides a list
// of options that the user can select.

#ifndef __vtkKWOptionMenu_h
#define __vtkKWOptionMenu_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWMenu;

class VTK_EXPORT vtkKWOptionMenu : public vtkKWWidget
{
public:
  static vtkKWOptionMenu* New();
  vtkTypeRevisionMacro(vtkKWOptionMenu,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the current entry of this optionmenu
  const char *GetValue();
  void SetValue(const char *);
  void SetCurrentEntry(const char *name);
  void SetCurrentImageEntry(const char *image_name);

  // Description:
  // Get the menu.
  vtkGetObjectMacro(Menu, vtkKWMenu);

  // Description:
  // Add/Insert entries to an option menu, with or without a command.
  void AddEntry(const char *name);
  void AddEntryWithCommand(const char *name, vtkKWObject *obj,
                           const char *method, const char *options = 0);
  void AddImageEntryWithCommand(const char *image_name, vtkKWObject *obj,
                                const char *method, const char *options = 0);
  void AddSeparator();

  // Description:
  // Remove entry from an option menu.
  void DeleteEntry(const char *name);
  void DeleteEntry(int index);
  
  // Description:
  // Has entry ?
  int HasEntry(const char *name);
  int GetNumberOfEntries();

  // Description:
  // Get entry label
  const char *GetEntryLabel(int index);
  
  // Description:
  // Remove all entries from the option menu.
  void ClearEntries();
  
  // Description
  // Set the indicator On/Off. To be called after creation.
  void IndicatorOn();
  void IndicatorOff();

  // Description:
  // Convenience method to set the button width (in chars if text, 
  // in pixels if image).
  void SetWidth(int width);
  
protected:
  vtkKWOptionMenu();
  ~vtkKWOptionMenu();

  char *CurrentValue;  
  vtkKWMenu *Menu;

private:
  vtkKWOptionMenu(const vtkKWOptionMenu&); // Not implemented
  void operator=(const vtkKWOptionMenu&); // Not implemented
};


#endif



