/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFieldMenu.h
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
class vtkPVIndexWidgetProperty;
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

  // Description:
  // A convenience method that returns information 
  // of the data attribute selected.
  vtkPVDataSetAttributesInformation* GetFieldInformation();

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

  // Description:
  // Called when the accept or reset button is pressed.
  // This internal version is passed VTK source name,
  virtual void AcceptInternal(vtkClientServerID);
  virtual void ResetInternal();

  // Description:
  // Set/get the property to use with this widget.
  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* GetProperty();
  
  // Description:
  // Create the right property for use with this widget.
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
protected:
  vtkPVFieldMenu();
  ~vtkPVFieldMenu();

  // For saving batch scripts.
  void SaveInBatchScriptForPart(ofstream *file,
                                vtkClientServerID);

  int ReadXMLAttributes(vtkPVXMLElement* element,
                        vtkPVXMLPackageParser* parser);

  vtkKWLabel* Label;
  vtkKWOptionMenu* FieldMenu;
  vtkPVInputMenu* InputMenu;

  // Description:
  // The property filters the allowable values of this menu..
  vtkPVInputProperty* GetInputProperty();

  int Value;  

  vtkPVIndexWidgetProperty *Property;
  int PropertyInitialized;
  
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
