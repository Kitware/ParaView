/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVArrayMenu - Menu for selecting arrays from the input.
//
// .SECTION Description
// This menu is pretty general, but expects a certain pattern to the 
// set get method.  It may be better to have explicit set/get method strings.
// I have been trying to get rid of the reference to PVSource, but this 
// reference is the only way I have to track when the input changes.
// I could make a subclass to do this, but I do not know how the more 
// general widget will be used.

#ifndef __vtkPVArrayMenu_h
#define __vtkPVArrayMenu_h

#include "vtkPVWidget.h"

class vtkPVInputMenu;
class vtkPVFieldMenu;
class vtkCollection;
class vtkDataArray;
class vtkPVDataSetAttributesInformation;
class vtkPVArrayInformation;
class vtkKWOptionMenu;
class vtkKWLabel;

class VTK_EXPORT vtkPVArrayMenu : public vtkPVWidget
{
public:
  static vtkPVArrayMenu* New();
  vtkTypeRevisionMacro(vtkPVArrayMenu, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  virtual void Accept();

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel();

  // Description:
  // This input menu supplies the data set.
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);
  
  // Description:
  // This menu can alternatively supply the data set.
  virtual void SetFieldMenu(vtkPVFieldMenu*);
  vtkGetObjectMacro(FieldMenu, vtkPVFieldMenu);

  // Description:
  // Set the menus value as a string.
  // Used by the Accept and Reset callbacks.
  // Can also be used from a script.
  void SetValue(const char* name);
  const char* GetValue() { return this->ArrayName;}

  //BTX
  // Description:
  // A convenience method that reutrns the VTK array selected.
  vtkPVArrayInformation *GetArrayInformation();
  //ETX

  // Description:
  // Direct access to the ArrayName is used internally by the Reset method. 
  // The methods "SetValue" should be used instead of this method.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // These are internal methods that are called when a menu is changed.
  void ArrayMenuEntryCallback(const char* name);

  // Description:
  // This is called to update the menus if something (InputMenu) changes.
  virtual void Update();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  vtkPVArrayMenu* ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *file);

  // Description:
  // Initialize the widget after creation
  virtual void Initialize();
 
protected:
  vtkPVArrayMenu();
  ~vtkPVArrayMenu();

  vtkPVDataSetAttributesInformation *GetFieldInformation();

  // Gets called when the reset button is pressed.
  virtual void ResetInternal();

  // The selected array name in the menu.  Current value of the widget.
  char *ArrayName;

  // This is where we get the data object arrays to populate our menu.
  vtkPVInputMenu *InputMenu;
  vtkPVFieldMenu *FieldMenu;

  // Subwidgets.
  vtkKWLabel *Label;
  vtkKWOptionMenu *ArrayMenu;

  // Resets the values based on the array.
  void UpdateArrayMenu();

  void UpdateProperty();

//BTX
  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

private:
  vtkPVArrayMenu(const vtkPVArrayMenu&); // Not implemented
  void operator=(const vtkPVArrayMenu&); // Not implemented
};

#endif
