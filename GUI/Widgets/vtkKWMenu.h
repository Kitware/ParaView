/*=========================================================================

  Module:    vtkKWMenu.h,v

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMenu - a menu widget
// .SECTION Description
// This class is the Menu abstraction.

#ifndef __vtkKWMenu_h
#define __vtkKWMenu_h

#include "vtkKWCoreWidget.h"

class vtkKWMenuInternals;

class KWWidgets_EXPORT vtkKWMenu : public vtkKWCoreWidget
{
public:
  static vtkKWMenu* New();
  vtkTypeRevisionMacro(vtkKWMenu,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  // Description: 
  // Append/Insert a standard command menu item to the menu.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // A vtkKWMenu::CommandItemAddedEvent event is generated as well.
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddCommand(const char *label);
  virtual int AddCommand(const char *label, 
                         vtkObject *object, const char *method);
  virtual int InsertCommand(int index, const char *label);
  virtual int InsertCommand(int index, const char *label, 
                            vtkObject *object, const char *method);

  // Description:
  // Set/Get the command for an existing menu item. This can also be used
  // on checkbutton and radiobutton entries (i.e. any menu item that was
  // created with a command).
  // Note that the output of GetItemCommand is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  virtual void SetItemCommand(
    int index, vtkObject *object, const char *method);
  virtual const char* GetItemCommand(int index);

  // Description: 
  // Get the index of the first menu item that is using a specific command.
  // You can also use GetIndexOfItem to retrieve the menu item using a 
  // specific label.
  virtual int GetIndexOfCommandItem(vtkObject *object, const char *method);

  // Description: 
  // Append/Insert a checkbutton menu item to the menu.
  // A vtkKWMenu::CheckButtonItemAddedEvent event is generated as well.
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddCheckButton(const char *label);
  virtual int AddCheckButton(const char *label,
                             vtkObject *object, const char *method);
  virtual int InsertCheckButton(int index, const char *label);
  virtual int InsertCheckButton(int index, const char *label,
                                vtkObject *object, const char *method);

  // Description: 
  // Set/Get an checkbutton or radiobutton selected state (where 1
  // means selected, 0 means deselected). 
  // Note that you can *not* set the selected state of a radiobutton to 0;
  // a radiobutton is part of a group, to deselect it one needs to select
  // another radiobutton in that group (therefore, unselecting a 
  // radiobutton has no meaning).
  // Items can be referred to by index or label.
  virtual void SetItemSelectedState(int index, int state);
  virtual void SetItemSelectedState(const char *label, int state);
  virtual int GetItemSelectedState(int index);
  virtual int GetItemSelectedState(const char *label);

  // Description: 
  // Select/Deselect a checkbutton or radiobutton.
  // These methods are just front-end to SetItemSelectedState. The same
  // constraints apply, i.e. you can *not* deselect a radiobutton (but
  // should select another radiobutton in the same group).
  // Items can be referred to by index or label.
  virtual void SelectItem(int index);
  virtual void SelectItem(const char *label);
  virtual void DeselectItem(int index);
  virtual void DeselectItem(const char *label);

  // Description: 
  // Append/Insert a radiobutton menu item to the menu.
  // Radiobuttons can be grouped together using either the 
  // PutItemInGroup method or the SetItemGroupName method.
  // By default, each newly created radiobutton share the same group already.
  // A vtkKWMenu::RadioButtonItemAddedEvent event is generated as well.
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddRadioButton(const char *label);
  virtual int AddRadioButton(const char *label, 
                             vtkObject *object, const char *method);
  virtual int InsertRadioButton(int index, const char *label);
  virtual int InsertRadioButton(int index, const char *label, 
                                vtkObject *object, const char *method);

  // Description: 
  // Append/Insert a radiobutton menu item to the menu using an image
  // instead of a label. The image name should be a valid Tk image name.
  // Radiobuttons can be grouped together using either the 
  // PutItemInGroup method or the SetItemGroupName method.
  // By default, each newly created radiobutton share the same group already.
  // A vtkKWMenu::RadioButtonItemAddedEvent event is generated as well.
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddRadioButtonImage(const char *imgname);
  virtual int AddRadioButtonImage(const char *imgname,
                                  vtkObject *object, const char *method);
  virtual int InsertRadioButtonImage(int index, const char *imgname);
  virtual int InsertRadioButtonImage(int index, const char *imgname,
                                     vtkObject *object, const char *method);

  // Description: 
  // Group items together so that when one is selected, the other
  // ones are automatically deselected. This makes sense for radiobutton
  // items where each item should be assigned a different 'selected value'
  // using the SetItemSelectedValue method.
  // This method puts the item specified by 'index' in the 
  // group the item specificied by 'index_g' is already a member of.
  // Note that put(index_a, index_g) and put(index_b, index_g)
  // will put index_a and index_b in the same group as index_g, but
  // put(index_a, index_g) and put(index_g, index_b) will put
  // index_a in the same group as index_g, then put index_g in the same
  // group as index_b, thus resulting as index_a being in a different group
  // as index_g/index_b.
  // Another way to group items together is to make sure they all share the
  // same group name (see SetItemGroupName), which is what is really done by
  // PutItemInGroup anyway.
  // By default, each newly created radiobutton share the same group already,
  // new checkbuttons are assigned to their own group, and other items type
  // have no group.
  virtual void PutItemInGroup(int index, int index_g);

  // Description:
  // Set/Get the *local* group name to use to group a set of items together
  // (i.e. selecting one item in the group will deselect the others).
  // This makes sense for radiobutton items where each item should
  // be assigned a different 'selected value' using the SetItemSelectedValue
  // method. Another way to group items together is to use the PutItemInGroup
  // method, which at the end of the day will make sure items share the same
  // group name.
  // Note that the group name should be unique within *this* menu instance (if
  // you need a global name that can be shared between menus, check the more
  // advanced SetItemVariableName method). The group name should stick to
  // the usual variable naming convention (no spaces, start with a letter,
  // etc); note that it will be cleaned automatically anyway and
  // stored that way.
  // By default, each newly created radiobutton share the same group already,
  // new checkbuttons are assigned to their own group, and other items type
  // have no group.
  virtual const char* GetItemGroupName(int index);
  virtual void SetItemGroupName(int index, const char *group_name);

  // Description:
  // Set/Get the value associated to the selected state of a checkbutton
  // or radiobutton menu item. Convenience methods are provided to Set/Get
  // that value as an integer.
  // By default, checkbutton are assigned a selected value of 1. Radiobuttons
  // are assigned a value corresponding to their own label (or image name).
  // Note that the output of GetItemSelectedValue is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  // Returns the value, or -1 if called on the wrong type of menu item
  virtual void SetItemSelectedValue(int index, const char *value);
  virtual const char* GetItemSelectedValue(int index);
  virtual void SetItemSelectedValueAsInt(int index, int value);
  virtual int GetItemSelectedValueAsInt(int index);

  // Description: 
  // Select the item in a group which selected value (as it was set per item
  // using SetItemSelectedValue) matches a given value. Convenience method is
  // provided to select the item using a value as an integer.
  // Returns the item that was selected, -1 otherwise.
  virtual int SelectItemInGroupWithSelectedValue(
    const char *group_name, const char *selected_value);
  virtual int SelectItemInGroupWithSelectedValueAsInt(
    const char *group_name, int selected_value);

  // Description: 
  // Get the index of the selected item in a group.
  // Return -1 on error.
  virtual int GetIndexOfSelectedItemInGroup(const char *group_name);

  // Description:
  // Set/Get the value associated to the de-selected state of a checkbutton
  // menu item (no allowed for radiobuttons). Can be safely left as is
  // most of the time, unless you want to store very specific selected
  // and deselected values for a checkbutton and retrieve them later on.
  // By default, checkbutton are assigned a selected value of 0. Radiobuttons
  // do not have a deselected value: a radiobutton is part of a group, to
  // deselect it one needs to select another radiobutton in that group
  // (therefore, unselecting a radiobutton has no meaning).
  // Note that the output of GetItemDeselectedValue is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  // Returns the value, or -1 if called on the wrong type of menu item
  virtual void SetItemDeselectedValue(int index, const char *value);
  virtual void SetItemDeselectedValueAsInt(int index, int value);
  virtual int GetItemDeselectedValueAsInt(int index);
  virtual const char* GetItemDeselectedValue(int index);

  // Description: 
  // Append/Insert a separator to the menu.
  // A vtkKWMenu::SeparatorItemAddedEvent event is generated as well.
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddSeparator();
  virtual int InsertSeparator(int index);
  
  // Description: 
  // Append/Insert a sub-menu (cascade) to the menu.
  // A vtkKWMenu::CascadeItemAddedEvent event is generated as well.
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddCascade(const char *label, vtkKWMenu *menu);
  virtual int InsertCascade(int index, const char *label, vtkKWMenu *menu);

  // Description: 
  // Get the index of the first sub-menu (cascade) item that is using a
  // specific menu object.
  virtual int GetIndexOfCascadeItem(vtkKWMenu *menu);

  // Description:
  // Set the sub-menu for an existing sub-menu (cascade) item.
  virtual void SetItemCascade(int index, vtkKWMenu*);
  virtual void SetItemCascade(int index, const char *menu_name);
  virtual vtkKWMenu* GetItemCascade(int index);

  // Description:
  // Returns the integer index of the menu item using a specific label.
  // You can also use GetIndexOfCommandItem to retrieve the menu item using a
  // specific command (if that menu item supports a command).
  virtual int GetIndexOfItem(const char *label);

  // Description:
  // Checks if an item with a given label is in the menu.
  virtual int HasItem(const char *label);

  // Description:
  // Set/Get the label of a specific menu item at a given index.
  // Note that the output of GetItemLabel is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  virtual int SetItemLabel(int index, const char *label);
  virtual const char* GetItemLabel(int index);

  // Description:
  // Call the callback/command of a specific menu item specified by its
  // index.
  virtual void InvokeItem(int index);

  // Description:
  // Delete the menu item specified by its given index.
  // Be careful, there is a bug in Tk, that will break other items
  // in the menu below the one being deleted, unless a new item is added.
  virtual void DeleteItem(int index);
  virtual void DeleteAllItems();
  
  // Description:
  // Returns the number of items.
  virtual int GetNumberOfItems();
  
  // Description:
  // Set/Get state of the menu item with a given index or label.
  // Valid constants can be found in vtkKWOptions::StateType.
  virtual void SetItemState(int index, int state);
  virtual void SetItemStateToDisabled(int index);
  virtual void SetItemStateToNormal(int index);
  virtual void SetItemState(const char *label, int state);
  virtual void SetItemStateToDisabled(const char *label);
  virtual void SetItemStateToNormal(const char *label);
  virtual int GetItemState(int index);
  virtual int GetItemState(const char *label);

  // Description:
  // Set the state of all entries.
  // Valid constants can be found in vtkKWOptions::StateType.
  // This should not be used directly, this is done by 
  // SetEnabled()/UpdateEnableState(). 
  // Overriden to pass to all menu entries.
  virtual void SetState(int state);

  // Description:
  // Set the image of a menu item. A valid Tk image name or the index of
  // a predefined icon (as found in vtkKWIcon) should be passed.
  // Check the SetItemCompoundMode method if you want to display both the
  // image and the label at the same time.
  virtual void SetItemImage(int index, const char *imgname);
  virtual void SetItemImageToPredefinedIcon(int index, int icon_index);

  // Description:
  // Set the select image of a menu item.
  // The select image is available only for checkbutton and radiobutton 
  // entries. This method can be used to specify the image to display in
  // the menu item when it is selected (instead of the regular image). 
  virtual void SetItemSelectImage(int index, const char *imgname);
  virtual void SetItemSelectImageToPredefinedIcon(int index, int icon_index);

  // Description:
  // Specifies if the menu item should display text and bitmaps/images at the
  // same time, and if so, where the bitmap/image should be placed relative 
  // to the text. 
  // Valid constants can be found in vtkKWOptions::CompoundModeType.
  // The (default) value CompoundModeNone specifies that the bitmap or image 
  // should (if defined) be displayed instead of the text. 
  // Check the SetItemMarginVisibility method too.
  virtual void SetItemCompoundMode(int index, int mode);
  virtual int GetItemCompoundMode(int index);
  virtual void SetItemCompoundModeToNone(int index);
  virtual void SetItemCompoundModeToLeft(int index);
  virtual void SetItemCompoundModeToCenter(int index);
  virtual void SetItemCompoundModeToRight(int index);
  virtual void SetItemCompoundModeToTop(int index);
  virtual void SetItemCompoundModeToBottom(int index);

  // Description:
  // Set the visibility of the standard margin of a menu item.
  // Hiding the margin is useful when creating palette with images in them,
  // i.e., color palettes, pattern palettes, etc.
  virtual void SetItemMarginVisibility(int index, int flag);

  // Description:
  // Set the visibility of the indicator of a menu item.
  // Available only for checkbutton and radiobutton entries.
  virtual void SetItemIndicatorVisibility(int index, int flag);

  // Description:
  // Specifies a string to display at the right side of the menu entry. 
  // Normally describes an accelerator keystroke sequence that may be typed
  // to invoke the same function as the menu entry. This is an arbitrary
  // string, not a key binding per say; yet, as a convenience, the 
  // accelerator will also be interpreted and added as a key binding to
  // the toplevel this menu is a child (or sub-child) of. It can recognizes
  // keyword like Ctrl+0.
  virtual void SetItemAccelerator(int index, const char *accelerator);

  // Description:
  // Set/Get the help string for a given item specified by its index.
  // The help string will be displayed automatically in the status bar
  // of the window containing the menu, if any.
  // Note that the output of GetItemHelpString is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  virtual void SetItemHelpString(int index, const char *help);
  virtual const char* GetItemHelpString(int index);

  // Description:
  // Set the index of a character to underline in the menu item specified by 
  // its index. Note that you can also specify that character
  // by using the special '&' marker in the label of the menu item 
  // (ex: "&File", or "Sa&ve File").
  virtual void SetItemUnderline(int index, int underline_index);

  // Description:
  // Set/Get a column break flag at a specific index.
  virtual void SetItemColumnBreak(int index, int flag);

  // Description:
  // Set/Get the name of a *global* variable to set when a checkbutton or a 
  // radiobutton menu item is selected. Whenever selected, a checkbutton
  // or radiobutton will set its variable to the value that was associated
  // to it using SetItemSelectedValue. When deselected a checkbutton set
  // its variable to the value that was associated to it using 
  // SetItemDeselectedValue.
  // Setting the same variable names for different radiobuttons is the
  // actual way to put them in the same *group* (i.e. selecting one 
  // radiobutton in the group will deselect the others). Is is recommended
  // to use either the PutItemInGroup method to group buttons 
  // together, or the SetItemGroupName method, which are just  
  // front-ends to the SetItemVariable method, but build a global variable
  // name out of the current menu instance name and a local group name.
  // Note that the variable name should be unique within *this* menu 
  // instance as well as *all* other menu instances so that it can be
  // shared between menus if needed. Most of the time, using a global name
  // will not be needed and we therefore recommend you pick a local global
  // name by using the SetItemGroupName. This variable name should
  // stick to the usual variable naming convention (no spaces, start with
  // a letter, etc). One way to create such a group name is, for example, to
  // concatenate the Tcl name (GetTclName()) or the widget name
  // (GetWidgetName()) of the menu instance with some string suffix 
  // describing the variable; a different signature is available to create
  // such composite name automatically, but the CreateItemVariableName can 
  // be used as well. 
  // By default, each newly created radiobutton share the same variable
  // name already. Each newly created checkbutton is assigned a unique 
  // variable name.
  // Note that the output of GetItemVariable is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  virtual const char* GetItemVariable(int index);
  virtual void SetItemVariable(int index, const char *varname);
  virtual void SetItemVariable(
    int index, vtkKWObject *object, const char *suffix);

  // Description: 
  // Convenience method to create a variable name out of an object
  // (say, this instance) and an arbitrary suffix (say, the menu item label). 
  // Such a variable name can be used to call SetItemVariable on a 
  // checkbutton or radiobutton menu item.
  // Note that spaces and unusual characters are automatically removed
  // from the 'suffix'. 
  // This method allocates enough memory (using 'new') for that name and
  // returns a pointer to that location. This pointer should be deleted
  // by the user using 'delete []'.
  virtual char* CreateItemVariableName(
    vtkKWObject *object, const char *suffix);

  // Description: 
  // Set/Get the value for a variable name given a variable name (as retrieved
  // using GetItemVariable() for example).
  // This will affect the menu entries accordingly (i.e. setting the 
  // variable will select/deselect the entries).
  // Note that the output of GetItemVariableValue is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  virtual const char* GetItemVariableValue(const char *varname);
  virtual void SetItemVariableValue(const char *varname, const char *value);
  virtual int GetItemVariableValueAsInt(const char *varname);
  virtual void SetItemVariableValueAsInt(const char *varname, int value);

  // Description:
  // Set/Get if this menu is a tearoff menu.  By dafault this value is off.
  virtual void SetTearOff(int val);
  vtkGetMacro(TearOff, int);
  vtkBooleanMacro(TearOff, int);

  // Description:
  // Pop-up the menu at screen coordinates x, y
  virtual void PopUp(int x, int y);

  // Description:
  // Get the Tk option of a specific menu item. Internal use.
  // Note that the output of GetItemOption is a pointer to a
  // temporary buffer that should be copied *immediately* to your own storage.
  virtual int HasItemOption(int index, const char *option);
  virtual const char* GetItemOption(int index, const char *option);

  // Description:
  // Events. The ItemAddedEvent events are generated when menu entries are
  // addded or inserted (say, RadioButtonItemAddedEvent).
  // The MenuItemInvokedEvent is sent when a menu entry is invoked (i.e.,
  // a mouse button is released over the entry).
  // The following parameters are also passed as client data:
  // - the index of the new menu item: int
  //BTX
  enum
  {
    RadioButtonItemAddedEvent = 10000,
    CheckButtonItemAddedEvent,
    CommandItemAddedEvent,
    SeparatorItemAddedEvent,
    CascadeItemAddedEvent,
    MenuItemInvokedEvent
  };
  //ETX

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget.
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the active background color of the widget. An element
  // (a widget or portion of a widget) is active if the mouse cursor is
  // positioned over the element and pressing a mouse button will cause some
  // action to occur.
  virtual void GetActiveBackgroundColor(double *r, double *g, double *b);
  virtual double* GetActiveBackgroundColor();
  virtual void SetActiveBackgroundColor(double r, double g, double b);
  virtual void SetActiveBackgroundColor(double rgb[3])
    { this->SetActiveBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the active foreground color of the widget. An element
  // (a widget or portion of a widget) is active if the mouse cursor is
  // positioned over the element and pressing a mouse button will cause some
  // action to occur.
  virtual void GetActiveForegroundColor(double *r, double *g, double *b);
  virtual double* GetActiveForegroundColor();
  virtual void SetActiveForegroundColor(double r, double g, double b);
  virtual void SetActiveForegroundColor(double rgb[3])
    { this->SetActiveForegroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget when it is disabled.
  virtual void GetDisabledForegroundColor(double *r, double *g, double *b);
  virtual double* GetDisabledForegroundColor();
  virtual void SetDisabledForegroundColor(double r, double g, double b);
  virtual void SetDisabledForegroundColor(double rgb[3])
    { this->SetDisabledForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the background color to use when the widget is selected.
  virtual void GetSelectColor(double *r, double *g, double *b);
  virtual double* GetSelectColor();
  virtual void SetSelectColor(double r, double g, double b);
  virtual void SetSelectColor(double rgb[3])
    { this->SetSelectColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the border width, a non-negative value indicating the width of
  // the 3-D border to draw around the outside of the widget (if such a border
  // is being drawn; the Relief option typically determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised();
  virtual void SetReliefToSunken();
  virtual void SetReliefToFlat();
  virtual void SetReliefToRidge();
  virtual void SetReliefToSolid();
  virtual void SetReliefToGroove();

  // Description:
  // Specifies the font to use when drawing text inside the widget. 
  // You can use predefined font names (e.g. 'system'), or you can specify
  // a set of font attributes with a platform-independent name, for example,
  // 'times 12 bold'. In this example, the font is specified with a three
  // element list: the first element is the font family, the second is the
  // size, the third is a list of style parameters (normal, bold, roman, 
  // italic, underline, overstrike). Example: 'times 12 {bold italic}'.
  // The Times, Courier and Helvetica font families are guaranteed to exist
  // and will be matched to the corresponding (closest) font on your system.
  // If you are familiar with the X font names specification, you can also
  // describe the font that way (say, '*times-medium-r-*-*-12*').
  virtual void SetFont(const char *font);
  virtual const char* GetFont();

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

  // Description:
  // Callbacks: for active menu item doc line help
  virtual void DisplayHelpCallback(const char *widget_name);
  virtual void CommandInvokedCallback(const char *command);
  
protected:
  vtkKWMenu();
  ~vtkKWMenu();
  
  // Description: 
  // Create the widget.
  virtual void CreateWidget();
  
  // Description: 
  // Add a generic menu item (defined by type)
  // Return index of the menu item that was created/inserted, -1 on error
  virtual int AddGeneric(const char *type, const char *label, 
                         const char* extra);
  virtual int InsertGeneric(int index, const char *type, const char *label, 
                            const char* extra);

  // Description: 
  // Given a label, allocate and create a clean label that is stripped
  // out of its underline marker (ex: "&Open File" is cleaned as "Open File", 
  // "Sa&ve File" is cleaned as "Save File"). 
  // Return 1 if a clean label was created: the 'clean_label' parameter 
  // should be freed later on by the user by calling 'delete []'. The 
  // 'underline' parameter is set to the position of the underline marker.
  // Return 0 if the label was already cleaned: the 'clean_label' parameter 
  // is set to the value of 'label' (set, *not* allocated). The 
  // 'underline' parameter is set to -1.
  virtual int GetLabelWithoutUnderline(
    const char *label, char **clean_label, int *underline_index);

  // Description: 
  // Get the index of the radiobutton using a specific variable name and
  // selected value.
  virtual int GetIndexOfItemUsingVariableAndSelectedValue(
    const char *varname, const char *selected_value);

  int TearOff;

  // Description:
  // Returns the integer index of the active menu item of a given menu.
  // Be extra careful with this method: most menus seems to be clone of
  // themselves, according to the Tk doc:
  //   When a menu is set as a menubar for a toplevel window, or when a menu
  //   is torn off, a clone of the menu is made. This clone is a menu widget
  //   in its own right, but it is a child of the original. Changes in the
  //   configuration of the original are reflected in the clone. Additionally, 
  //   any cascades that are pointed to are also cloned so that menu traversal
  //   will work right. 
  // Sadly, the active item of a menu is not something that is synchronized
  // between a menu and its clone. Querying the active item on the current
  // instance might therefore not work, because a clone is in fact being
  // interacted upon. This kind of situation is mostly encountered when
  // Tk events like <<MenuSelect>> are processed, and can therefore be solved
  // by making sure the callbacks are passing the %W parameter around: this
  // gets resolved to the widget name of the clone.
  virtual int GetIndexOfActiveItem(const char *widget_name);

  // Description:
  // Get the suffix out of variable name that was created using 
  // CreateItemVariableName
  const char* GetSuffixOutOfCreatedItemVariableName(const char *varname);

  // Description:
  // Install accelerator binding to toplevel
  virtual void InstallItemAcceleratorBindingOnToplevel(int index);
  virtual void ConvertItemAcceleratorToKeyBinding(
    const char *accelerator, char **keybinding);

  // Description:
  // PIMPL Encapsulation for STL containers
  vtkKWMenuInternals *Internals;
  
private:

  vtkKWMenu(const vtkKWMenu&); // Not implemented
  void operator=(const vtkKWMenu&); // Not implemented
};

#endif
