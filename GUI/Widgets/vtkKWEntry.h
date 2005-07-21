/*=========================================================================

  Module:    vtkKWEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWEntry - a single line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for single line input.

#ifndef __vtkKWEntry_h
#define __vtkKWEntry_h

#include "vtkKWCoreWidget.h"

class vtkKWApplication;

class KWWIDGETS_EXPORT vtkKWEntry : public vtkKWCoreWidget
{
public:
  static vtkKWEntry* New();
  vtkTypeRevisionMacro(vtkKWEntry,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set/Get the value of the entry in a few different formats.
  // In the SetValue method with double, values are printed in printf's f or e
  // format, whichever is more compact for the given value and precision. 
  // The e format is used only when the exponent of the value is less than
  // -4 or greater than or equal to the precision argument (which can be
  // controlled using the the second parameter of SetValue). Trailing zeros
  // are truncated, and the decimal point appears only if one or more digits
  // follow it.
  // IMPORTANT: whenever possible, use any of the GetValueAs...() methods
  // to retrieve the value if it is meant to be a number. This is faster
  // than calling GetValue() and converting the resulting string to a number.
  virtual void SetValue(const char *);
  virtual const char* GetValue();
  virtual void SetValueAsInt(int a);
  virtual int GetValueAsInt();
  virtual void SetValueAsFormattedDouble(double f, int size);
  virtual void SetValueAsDouble(double f);
  virtual double GetValueAsDouble();
  
  // Description:
  // The width is the number of charaters wide the entry box can fit.
  // To keep from changing behavior of the entry, the default
  // value is -1 wich means the width is not explicitly set and will default
  // to whatever value Tk is using (at this point, 20). Set it to 0
  // and the widget should pick a size just large enough to hold its text.
  virtual void SetWidth(int width);
  vtkGetMacro(Width, int);

  // Description:
  // Set or get readonly flag. This flags makes entry read only.
  virtual void SetReadOnly(int);
  vtkBooleanMacro(ReadOnly, int);
  vtkGetMacro(ReadOnly, int);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the return key is pressed, or the focus is lost.
  // The first argument is the object that will have the method called on it.
  // The second argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method
  // is evaluated as a simple command.
  virtual void SetCommand(vtkObject *object, const char *method);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWEntry();
  ~vtkKWEntry();
  
  int Width;
  int ReadOnly;

private:

  char *InternalValueString;
  vtkGetStringMacro(InternalValueString);
  vtkSetStringMacro(InternalValueString);

  vtkKWEntry(const vtkKWEntry&); // Not implemented
  void operator=(const vtkKWEntry&); // Not Implemented
};

#endif
