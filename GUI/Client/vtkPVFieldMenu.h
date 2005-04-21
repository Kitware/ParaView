/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFieldMenu.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVFieldMenu - A menu to select point or cell fields.
// .SECTION Description
// This menu works with vtkPVInputMenu and vtkPVArrayMenu.
// It was developed for the Threshold filter which can threshold
// based on a cell or point array.
// Input menu supplies the DataSet information of the current
// input.  It also calls update when the value of the input menu changes.
// The array menu can use this field menu as a source of its arrays.


#ifndef __vtkPVFieldMenu_h
#define __vtkPVFieldMenu_h

#include "vtkPVWidget.h"

class vtkKWOptionMenu;
class vtkKWWidget;
class vtkKWLabel;
class vtkKWOptionMenu;
class vtkPVInputMenu;
class vtkPVInputProperty;
class vtkPVDataSetAttributesInformation;

class VTK_EXPORT vtkPVFieldMenu : public vtkPVWidget
{
public:
  static vtkPVFieldMenu* New();
  vtkTypeRevisionMacro(vtkPVFieldMenu, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // This input menu supplies the data information.
  // You must set this (XML).
  virtual void SetInputMenu(vtkPVInputMenu*);
  vtkGetObjectMacro(InputMenu, vtkPVInputMenu);
  
  // Description:
  // Set the menus value as a string.
  // Used by the Accept and Reset callbacks.
  // Can also be used from a script.
  void SetValue(int field);
  vtkGetMacro(Value,int);

  //BTX
  // Description:
  // A convenience method that returns information 
  // of the data attribute selected.
  vtkPVDataSetAttributesInformation* GetFieldInformation();
  //ETX

  // Description:
  // This is called to update the menus if something (InputMenu) changes.
  virtual void Update();

//BTX
  // Description:
  // Creates and returns a copy of this widget. It will create
  // a new instance of the same type as the current object
  // using NewInstance() and then copy some necessary state 
  // parameters.
  virtual vtkPVFieldMenu* ClonePrototype(vtkPVSource* pvSource,
                             vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  //BTX
  // Description:
  // Called when the accept or reset button is pressed.
  virtual void Accept();
  virtual void ResetInternal();

  // Description:
  // Called after the widget is created. Initializes the gui from
  // server manager
  virtual void Initialize();
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVFieldMenu();
  ~vtkPVFieldMenu();

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  vtkKWLabel* Label;
  vtkKWOptionMenu* FieldMenu;
  vtkPVInputMenu* InputMenu;

  void UpdateProperty();

  // Description:
  // The property filters the allowable values of this menu..
  vtkPVInputProperty* GetInputProperty();

  int Value;  

//BTX
  virtual vtkPVWidget* ClonePrototypeInternal(vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
  virtual void CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map);
//ETX

  vtkPVFieldMenu(const vtkPVFieldMenu&); // Not implemented
  void operator=(const vtkPVFieldMenu&); // Not implemented
};

#endif
