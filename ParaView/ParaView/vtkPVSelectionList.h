/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.h
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

  // Description:
  // Called when accept button is pushed.
  // Sets the objects variable from UI.
  virtual void AcceptInternal(vtkClientServerID);

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  virtual void SetProperty(vtkPVWidgetProperty *prop);
  virtual vtkPVWidgetProperty* CreateAppropriateProperty();
  
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
  int AcceptCalled;
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
