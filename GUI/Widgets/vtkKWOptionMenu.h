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
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the current entry of this optionmenu
  virtual const char *GetValue();
  virtual void SetValue(const char *);
  virtual void SetCurrentEntry(const char *name);
  virtual void SetCurrentImageEntry(const char *image_name);

  // Description:
  // Add/Insert entries to an option menu, with or without a command.
  virtual void AddEntry(const char *name);
  virtual void AddEntryWithCommand(
    const char *name, vtkKWObject *obj, const char *method, 
    const char *options=0);
  virtual void AddImageEntryWithCommand(
    const char *image_name, vtkKWObject *obj, const char *method, 
    const char *options = 0);
  virtual void AddSeparator();

  // Description:
  // Remove entry from an option menu (given its name or position in the menu).
  // Or remove all entries with DeleteAllEntries.
  virtual void DeleteEntry(const char *name);
  virtual void DeleteEntry(int index);
  virtual void DeleteAllEntries();
  
  // Description:
  // Has entry ? Number of Entries
  virtual int HasEntry(const char *name);
  virtual int GetNumberOfEntries();

  // Description:
  // Get the n-th entry label
  virtual const char *GetEntryLabel(int index);
  
  // Description
  // Set the indicator On/Off. To be called after creation.
  void IndicatorOn();
  void IndicatorOff();

  // Description:
  // Convenience method to set the button width (in chars if text, 
  // in pixels if image).
  void SetWidth(int width);
  
  // Description:
  // Set/Get the maximum width of the option menu label
  // This does not modify the internal value, this is just for display
  // purposes: the option menu button can therefore be automatically
  // shrinked, while the menu associated to it will display all entries
  // correctly.
  // Set width to 0 (default) to prevent auto-cropping.
  virtual void SetMaximumLabelWidth(int);
  vtkGetMacro(MaximumLabelWidth, int);

  // Description:
  // Get the menu object
  vtkGetObjectMacro(Menu, vtkKWMenu);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks (don't call)
  virtual void TracedVariableChangedCallback(
    const char *, const char *, const char *);

protected:
  vtkKWOptionMenu();
  ~vtkKWOptionMenu();

  vtkGetStringMacro(CurrentValue);
  vtkSetStringMacro(CurrentValue);

  char      *CurrentValue;  
  vtkKWMenu *Menu;
  int       MaximumLabelWidth;

  virtual void UpdateOptionMenuLabel();

private:
  vtkKWOptionMenu(const vtkKWOptionMenu&); // Not implemented
  void operator=(const vtkKWOptionMenu&); // Not implemented
};


#endif



