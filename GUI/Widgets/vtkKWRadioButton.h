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
class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWRadioButton : public vtkKWCheckButton
{
public:
  static vtkKWRadioButton* New();
  vtkTypeRevisionMacro(vtkKWRadioButton,vtkKWCheckButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the state of the Radio button 0 = off 1 = on
  vtkBooleanMacro(State,int);
  virtual int GetState();

  // Description:
  // Specify the value to store in the button's associated variable 
  // whenever this button is selected.
  void SetValue(int v);
  void SetValue(const char *v);

protected:
  vtkKWRadioButton() {};
  ~vtkKWRadioButton() {};

private:
  vtkKWRadioButton(const vtkKWRadioButton&); // Not implemented
  void operator=(const vtkKWRadioButton&); // Not implemented
};


#endif



