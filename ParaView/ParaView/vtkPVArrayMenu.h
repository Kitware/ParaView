/*=========================================================================

  Program:   ParaView
  Module:    vtkPVArrayMenu.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVArrayMenu menu for selecting arrays from the input.
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
class vtkPVStringAndScalarListWidgetProperty;
class vtkKWOptionMenu;
class vtkKWLabel;

class VTK_EXPORT vtkPVArrayMenu : public vtkPVWidget
{
public:
  static vtkPVArrayMenu* New();
  vtkTypeRevisionMacro(vtkPVArrayMenu, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the label.  The label can be used to get this widget
  // from a script.
  void SetLabel (const char* label);
  const char* GetLabel();

  // Description:
  // This is a method that the field menu calls.  The user can call
  // it also.  It determines whether the array menu has point or cell arrays
  // to select.
  void SetFieldSelection(int field);
  int GetFieldSelection() {return this->FieldSelection;}

  // Description:
  // When NumberOfComponents is 1, this option allows components to
  // be selected from multidimensional arrays.  
  // A second component menu is displayed.
  void SetShowComponentMenu(int flag);
  vtkGetMacro(ShowComponentMenu, int);
  vtkBooleanMacro(ShowComponentMenu, int);

  // Description:
  // Only arrays with this number of components are added to the menu.
  // If this value is 0 or less, then all arrays are added to the menu.
  // The default value is 1.  If "ShowComponentMenu" is on, then all
  // arrays are added to the menu.
  void SetNumberOfComponents(int num);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // This is one value that lets this widget interact with its associated 
  // object.  This specifies the name of the input (i.e Input, Source ...).
  // Most of the time this value should be set to "Input".
  // It defaults to NULL.
  vtkSetStringMacro(InputName);
  vtkGetStringMacro(InputName);
  
  // Description:
  // This is one value that lets this widget interact with its associated 
  // object.  This specifies the type of the attribute (i.e Scalars, Vectors ...).
  // It defaults to NULL.
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);  

  // Description:
  // This is the filter/object that will be modified by the widgtet when the 
  // selected array gets changed in the menu.  It should have methods like:
  // SelectInputScalars and GetInputScalarsSelection.
  // Description:
  vtkSetStringMacro(ObjectTclName);
  vtkGetStringMacro(ObjectTclName);

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

  // Description:
  // A convenience method that reutrns the VTK array selected.
  vtkPVArrayInformation *GetArrayInformation();

  // Description:
  // This is the number of components the selected array has.
  vtkGetMacro(ArrayNumberOfComponents, int);

  // Description:
  // Set the selected component.  This is only aplicable when 
  // "ShowComponentMenu" is on. It can be used in scripts.
  void SetSelectedComponent(int comp);
  vtkGetMacro(SelectedComponent, int);


  // Description:
  // Direct access to the ArrayName is used internally by the Reset method. 
  // The methods "SetValue" should be used instead of this method.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // These are internal methods that are called when a menu is changed.
  void ArrayMenuEntryCallback(const char* name);
  void ComponentMenuEntryCallback(int comp);

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

  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVArrayMenu();
  ~vtkPVArrayMenu();

  vtkPVDataSetAttributesInformation *GetFieldInformation();

  // Gets called when the accept button is pressed.
  virtual void AcceptInternal(const char* sourceTclName);

  // Gets called when the reset button is pressed.
  virtual void ResetInternal();


  // The selected array name in the menu.  Current value of the widget.
  char *ArrayName;
  int ArrayNumberOfComponents;
  int SelectedComponent;

  int NumberOfComponents;
  int ShowComponentMenu;

  // Selection only used if no field menu.
  int FieldSelection;

  // This is where we get the data object arrays to populate our menu.
  vtkPVInputMenu *InputMenu;
  vtkPVFieldMenu *FieldMenu;

  // These are options that allow the widget to interact with its associated object.
  char*       InputName;
  int         AttributeType;
  char*       ObjectTclName;

  // Subwidgets.
  vtkKWLabel *Label;
  vtkKWOptionMenu *ArrayMenu;
  vtkKWOptionMenu *ComponentMenu;

  // Resets the values based on the array.
  void UpdateArrayMenu();

  // Resets the values based on the array.
  void UpdateComponentMenu();
  
  int AcceptCalled;

  vtkPVStringAndScalarListWidgetProperty *Property;
  
//BTX
  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  // Description:
  // The widget saves it state/command in the vtk tcl script.
  void SaveInBatchScriptForPart(ofstream *file, const char* sourceTclName);

private:
  vtkPVArrayMenu(const vtkPVArrayMenu&); // Not implemented
  void operator=(const vtkPVArrayMenu&); // Not implemented
};

#endif
