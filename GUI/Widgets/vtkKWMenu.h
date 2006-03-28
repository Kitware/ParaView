/*=========================================================================

  Module:    vtkKWMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMenu - a menu widget
// .SECTION Description
// This class is the Menu abstraction for the
// Kitware toolkit. It provides a c++ interface to
// the TK menu widgets used by the Kitware toolkit.

#ifndef __vtkKWMenu_h
#define __vtkKWMenu_h

#include "vtkKWCoreWidget.h"

class KWWidgets_EXPORT vtkKWMenu : public vtkKWCoreWidget
{
public:
  static vtkKWMenu* New();
  vtkTypeRevisionMacro(vtkKWMenu,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description: 
  // Create the widget.
  virtual void Create();
  
  // Description: 
  // Append/Insert a separator to the menu.
  void AddSeparator();
  void InsertSeparator(int position);
  
  // Description: 
  // Append/Insert a sub menu to the current menu.
  void AddCascade(const char* label, vtkKWMenu*, 
                  int underline, const char* help = 0);
  void InsertCascade(int position, const char* label,  vtkKWMenu*, 
                     int underline, const char* help = 0  );
  int GetCascadeIndex(vtkKWMenu *);

  // Description:
  // Set cascade menu for menu entry.
  void SetCascade(int index, vtkKWMenu*);
  void SetCascade(const char *label, vtkKWMenu*);
  void SetCascade(int index, const char *menu_name);
  void SetCascade(const char *label, const char *menu_name);

  // Description:
  // Copy the radio button variable logic.

  // Description: 
  // Append/Insert a CheckButton menu item to the current menu.
  char* CreateCheckButtonVariable(vtkKWObject* Object, const char* name);
  int   GetCheckButtonValue(vtkKWObject* Object, const char* name);
  void  CheckCheckButton(vtkKWObject *Object, const char *name, int val);
  void AddCheckButton(const char* label, const char* ButtonVar, 
                      vtkObject *object, 
                      const char *method , const char* help = 0);
  void AddCheckButton(const char* label, const char* ButtonVar, 
                      vtkObject *object, 
                      const char *method , int underline,
                      const char* help = 0);
  void InsertCheckButton(int position, 
                         const char* label, const char* ButtonVar, 
                         vtkObject *object, 
                         const char *method , const char* help = 0);
  void InsertCheckButton(int position, 
                         const char* label, const char* ButtonVar, 
                         vtkObject *object, 
                         const char *method , 
                         int underline, const char* help = 0);

  // Description: 
  // Append/Insert a standard menu item and command to the current menu.
  void AddCommand(const char* label, vtkObject *object,
                  const char *method , const char* help = 0);
  void AddCommand(const char* label, vtkObject *object,
                  const char *method , int underline, 
                  const char* help = 0);
  void InsertCommand(int position, const char* label, vtkObject *object,
                     const char *method , const char* help = 0);
  void InsertCommand(int position, const char* label, vtkObject *object,
                     const char *method , 
                     int underline, const char* help = 0);

  // Description:
  // Set command of the menu entry with a given index.
  void SetEntryCommand(int index, vtkObject *object, 
                       const char *method);
  void SetEntryCommand(const char* item, vtkObject *object, 
                       const char *method);
  void SetEntryCommand(int item, const char *method);
  void SetEntryCommand(const char* item, const char *method);

  // Description: 
  // Append a radio menu item and command to the current menu.
  // The radio group is specified by the buttonVar value.
  char* CreateRadioButtonVariable(vtkKWObject* Object, const char* varname);
  int   GetRadioButtonValue(vtkKWObject* Object, const char* varname);
  void  CheckRadioButton(vtkKWObject *Object, const char *varname, int id);
  int   GetCheckedRadioButtonItem(vtkKWObject *Object, const char *varname);
  void AddRadioButton(int value, const char* label, const char* buttonVar, 
                      vtkObject *object, 
                      const char *method, const char* help = 0);
  void AddRadioButton(int value, const char* label, const char* buttonVar, 
                      vtkObject *object, 
                      const char *method, int underline,  
                      const char* help = 0);
  void AddRadioButtonImage(int value, const char* imgname, 
                           const char* buttonVar, vtkObject *object, 
                          const char *method, const char* help = 0);
  void  InsertRadioButton(int position, int value, const char* label, 
                          const char* buttonVar, vtkObject *object, 
                          const char *method, const char* help = 0);
  void  InsertRadioButton(int position, int value, const char* label, 
                          const char* buttonVar, vtkObject *object, 
                          const char *method, 
                          int underline, const char* help = 0);

  // Description: 
  // Add a generic menu item (defined by addtype)
  void AddGeneric(const char* addtype, const char* label, vtkObject *object,
                  const char *method, const char* extra, 
                  const char* help);
  void InsertGeneric(int position, const char* addtype, const char* label, 
                     vtkObject *object,
                     const char *method, const char* extra, 
                     const char* help);

  // Description:
  // Call the menu item callback at the given index
  void Invoke(int position);
  void Invoke(const char *label);

  // Description:
  // Delete the menu item at the given position.
  // Be careful, there is a bug in tk, that will break other items
  // in the menu below the one being deleted, unless a new item is added.
  void DeleteMenuItem(int position);
  void DeleteMenuItem(const char *label);
  void DeleteAllMenuItems();
  
  // Description:
  // Returns the integer index of the menu item by string, or by the
  // command (object/method) pair associated to it.
  int GetIndexOfItem(const char *label);
  int GetIndexOfCommand(vtkObject *object, const char *method);

  // Description:
  // Get the command for the entry at index. This is what is returned by
  // Script, so you should make a copy if you want to use it in Tcl.
  const char* GetItemCommand(int idx);

  // Description:
  // Copies the label of the item at the given position
  // to the given string ( with the given length ). Returns VTK_OK
  // if there is label, VTK_ERROR otherwise.
  // The second version returns a pointer to the result of the
  // Tcl interpreter last evaluation (be careful).
  int GetItemLabel(int position, char* label, int maxlen);
  const char* GetItemLabel(int position);
  int SetItemLabel(int position, const char* label);

  // Description:
  // Get the option of an entry
  int HasItemOption(int position, const char *option);
  const char* GetItemOption(int position, const char *option);
  const char* GetItemOption(const char *label, const char *option);

  // Description:
  // Set the image and select image of an entry.
  // Check the SetItemCompoundMode if you want to display both the
  // image and the text.
  void SetItemImage(int position, const char *imagename);
  void SetItemImage(const char *label, const char *imagename);
  void SetItemImageToPredefinedIcon(int position, int icon_index);
  void SetItemImageToPredefinedIcon(const char *label, int icon_index);

  // Description:
  // Set the select image of an entry.
  // The select image is available only for checkbutton and radiobutton 
  // entries. Specifies an image to display in the entry (in place of the 
  // regular image) when it is selected. 
  void SetItemSelectImage(int position, const char *imagename);
  void SetItemSelectImage(const char *label, const char *imagename);
  void SetItemSelectImageToPredefinedIcon(int position, int icon_index);
  void SetItemSelectImageToPredefinedIcon(const char *label, int icon_index);

  // Description:
  // Set the compound mode of an entry. Set it to 'true' to display
  // both the image and the text.
  // Check the SetItemMarginVisibility method too.
  void SetItemCompoundMode(int position, int flag);
  void SetItemCompoundMode(const char *label, int flag);

  // Description:
  // Set the visibility of the standard margin of an entry.
  // Hiding the margin is useful when creating palette with images in them,
  // i.e., color palettes, pattern palettes, etc.
  void SetItemMarginVisibility(int position, int flag);
  void SetItemMarginVisibility(const char *label, int flag);

  // Description:
  // Set the visibility of the indicator of an entry.
  // Available only for checkbutton and radiobutton entries.
  void SetItemIndicatorVisibility(int position, int flag);
  void SetItemIndicatorVisibility(const char *label, int flag);

  // Description:
  // Set/Get the accelerator for a given item.
  void SetItemAccelerator(int position, const char *accelerator);
  void SetItemAccelerator(const char *label, const char *accelerator);

  // Description:
  // Checks if an item is in the menu
  int HasItem(const char* label);

  // Description:
  // Returns the number of items
  int GetNumberOfItems();
  
  // Description:
  // Call back for active menu item doc line help
  void DisplayHelp(const char*);
  
  // Description:
  // Option to make this menu a tearoff menu.  By dafault this value is off.
  void SetTearOff(int val);
  vtkGetMacro(TearOff, int);
  vtkBooleanMacro(TearOff, int);

  // Description:
  // Set/Get state of the menu entry with a given index or name.
  // Valid constants can be found in vtkKWTkOptions::StateType.
  virtual void SetItemState(int index, int state);
  virtual void SetItemState(const char *label, int state);
  virtual int  GetItemState(int index);
  virtual int  GetItemState(const char *label);

  // Description:
  // Convenience method to set the state of all entries.
  // Valid constants can be found in vtkKWTkOptions::StateType.
  // This should not be used directly, this is done by 
  // SetEnabled()/UpdateEnableState(). 
  // Overriden to pass to all menu entries
  virtual void SetState(int state);

  // Description:
  // Configure the item at given index.
  void ConfigureItem(int index, const char*);

  // Description:
  // Pop-up the menu at screen coordinates x, y
  virtual void PopUp(int x, int y);

  // Description:
  // Set or get enabled state.
  // This method has been overriden to propagate the state to all its
  // menu entries by calling UpdateEnableState(), *even* if the
  // state (this->Enabled) is actually unchanged by the function. This
  // make sure all the menu entries have been enabled/disabled properly.
  virtual void SetEnabled(int);

  // Description:
  // This method has been overriden to propagate the state to all its
  // menu entries by calling SetState().
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  
  vtkKWMenu();
  ~vtkKWMenu();

  int TearOff;
  
private:
  vtkKWMenu(const vtkKWMenu&); // Not implemented
  void operator=(const vtkKWMenu&); // Not implemented
};


#endif



