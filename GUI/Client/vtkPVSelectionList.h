/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSelectionList - Another form of radio buttons.
// .SECTION Description
// This widget produces a button that displays a selection from a list.
// When the button is pressed, the list is displayed in the form of a menu.
// The user can select a new value from the menu.

// This class is not named correctly.  It should be selection menu.


#ifndef __vtkPVSelectionList_h
#define __vtkPVSelectionList_h

#include "vtkPVObjectWidget.h"

class vtkStringList;
class vtkKWOptionMenu;
class vtkKWLabel;
class vtkPVIndexWidgetProperty;

class VTK_EXPORT vtkPVSelectionList : public vtkPVObjectWidget
{
public:
  static vtkPVSelectionList* New();
  vtkTypeRevisionMacro(vtkPVSelectionList, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Add items to the possible selection.
  // The string name is displayed in the list, and the integer value
  // is used to set and get the current selection programmatically.
  void AddItem(const char *name, int value);
  
  // Description:
  // Set the label of the menu.
  void SetLabel(const char *label);
  const char *GetLabel();

  // Description:
  // This is how the user can query the state of the selection.
  // Warning:  Setting the current value will not change vtk ivar.
  vtkGetMacro(CurrentValue, int);
  void SetCurrentValue(int val);
  vtkGetStringMacro(CurrentName);

  // Description:
  // Sets the width (in units of character) of the option menu
  // (should be specified before create)
  vtkSetMacro(OptionWidth, int);
  
  // Description:
  // This method gets called when the user selects an entry.
  // Use this method if you want to programmatically change the selection.
  void SelectCallback(const char *name, int value);

  // Description:
  // This class redefines SetBalloonHelpString since it
  // has to forward the call to the widgets (label and selection) it contains.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Disables the widget.
  void Disable();
  
//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVSelectionList* ClonePrototype(vtkPVSource* pvSource,
                                     vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  //BTX
  // Description:
  // Called when accept button is pushed.
  // Sets the objects variable from UI.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVSelectionList();
  ~vtkPVSelectionList();

  vtkKWLabel *Label;
  vtkKWOptionMenu *Menu;
  char *Command;

  int OptionWidth;

  int CurrentValue;
  char *CurrentName;
  // Using this list as an array of strings.
  vtkStringList *Names;

  vtkSetStringMacro(CurrentName);

  int DefaultValue;
  vtkSetMacro(DefaultValue, int);

  vtkPVIndexWidgetProperty *Property;
  
//BTX
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX
  
  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVSelectionList(const vtkPVSelectionList&); // Not implemented
  void operator=(const vtkPVSelectionList&); // Not implemented
};

#endif
