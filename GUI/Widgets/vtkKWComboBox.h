/*=========================================================================

  Module:    vtkKWComboBox.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWComboBox - a text entry widget with a pull-down menu of values
// .SECTION Description
// A simple subclass of entry that adds a pull-down menu where a predefined
// set of values can be chosed to set the entry field.
// .SECTION See Also
// vtkKWEntry

#ifndef __vtkKWComboBox_h
#define __vtkKWComboBox_h

#include "vtkKWEntry.h"

class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWComboBox : public vtkKWEntry
{
public:
  static vtkKWComboBox* New();
  vtkTypeRevisionMacro(vtkKWComboBox,vtkKWEntry);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Add and delete values to put in the list.
  virtual void AddValue(const char* value);
  virtual void DeleteValue(int idx);
  virtual int GetValueIndex(const char* value);
  virtual const char* GetValueFromIndex(int idx);
  virtual int GetNumberOfValues();
  virtual void DeleteAllValues();

  // Description:
  // Set/Get the value of the entry in a few different formats.
  // Overriden to comply with the Tk type
  virtual void SetValue(const char *);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWComboBox() {};
  ~vtkKWComboBox() {};
  
private:
  vtkKWComboBox(const vtkKWComboBox&); // Not implemented
  void operator=(const vtkKWComboBox&); // Not Implemented
};


#endif



