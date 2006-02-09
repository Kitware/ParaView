/*=========================================================================

  Module:    vtkKWRadioButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRadioButton - a radio button widget
// .SECTION Description
// A simple widget representing a radio button. The state can be set or
// queried.

#ifndef __vtkKWRadioButton_h
#define __vtkKWRadioButton_h

#include "vtkKWCheckButton.h"

class KWWidgets_EXPORT vtkKWRadioButton : public vtkKWCheckButton
{
public:
  static vtkKWRadioButton* New();
  vtkTypeRevisionMacro(vtkKWRadioButton,vtkKWCheckButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the state of the Radio button 0 = off 1 = on
  vtkBooleanMacro(SelectedState,int);
  virtual int GetSelectedState();

  // Description:
  // Set/Get the value to store in the button's associated variable 
  // whenever this button is selected.
  virtual void SetValue(const char *v);
  virtual void SetValueAsInt(int v);
  virtual const char* GetValue();
  virtual int GetValueAsInt();

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the button is selected or deselected.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // Note that the selected state is *not* passed as parameter, which is
  // the case for vtkKWCheckButton for example. In most cases, since the 
  // selected state is shared among many radiobuttons, this command will
  // likely to perform a task related to the meaning of the button itself.
  virtual void SetCommand(vtkObject *object, const char *method);

  // Description:
  // Events. The SelectedStateChangedEvent is triggered when the button
  // is selected or deselected.
  // The following parameters are also passed as client data:
  // - the current selected state: int
  // Yes, this is duplicated from vtkKWCheckButton, so that code does not
  // break when vtkKWRadioButton is not a subclass of vtkKWCheckButton anymore.
  //BTX
  enum
  {
    SelectedStateChangedEvent = 10000
  };
  //ETX

  // Description:
  // Convenience method to set/get the button's associated variable directly
  // to a specific value.
  virtual void SetVariableValue(const char *v);
  virtual void SetVariableValueAsInt(int v);
  virtual const char* GetVariableValue();
  virtual int GetVariableValueAsInt();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void CommandCallback();

protected:
  vtkKWRadioButton() {};
  ~vtkKWRadioButton() {};

  // Override the superclass (state is ignored)
  virtual void InvokeCommand(int state);

private:
  vtkKWRadioButton(const vtkKWRadioButton&); // Not implemented
  void operator=(const vtkKWRadioButton&); // Not implemented
};


#endif



