/*=========================================================================

  Module:    vtkKWLoadSaveButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLoadSaveButton - a button that triggers a load/save dialog
// .SECTION Description
// The vtkKWLoadSaveButton class creates a push button that
// will popup a vtkKWLoadSaveDialog and display the chosen filename as
// the button label.
// .SECTION See Also
// vtkKWLoadSaveButtonWithLabel

#ifndef __vtkKWLoadSaveButton_h
#define __vtkKWLoadSaveButton_h

#include "vtkKWPushButton.h"

class vtkKWLoadSaveDialog;

class KWWidgets_EXPORT vtkKWLoadSaveButton : public vtkKWPushButton
{
public:
  static vtkKWLoadSaveButton* New();
  vtkTypeRevisionMacro(vtkKWLoadSaveButton, vtkKWPushButton);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Access to sub-widgets.
  vtkGetObjectMacro(LoadSaveDialog, vtkKWLoadSaveDialog);

  // Description:
  // Retrieve the filename.
  virtual char* GetFileName();

  // Description:
  // Set/Get the length of the filename when displayed in the button.
  // If set to 0, do not shorten the filename.
  virtual void SetMaximumFileNameLength(int);
  vtkGetMacro(MaximumFileNameLength, int);

  // Description:
  // Set/Get if the path of the filename should be trimmed when displayed in
  // the button.
  virtual void SetTrimPathFromFileName(int);
  vtkBooleanMacro(TrimPathFromFileName, int);
  vtkGetMacro(TrimPathFromFileName, int);
  
  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when button is pressed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // Override the parent SetCommand method.
  virtual void SetCommand(vtkObject *object, const char *method);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void InvokeLoadSaveDialogCallback();

protected:
  vtkKWLoadSaveButton();
  ~vtkKWLoadSaveButton();

  vtkKWLoadSaveDialog *LoadSaveDialog;

  int TrimPathFromFileName;
  int MaximumFileNameLength;
  virtual void UpdateFileName();

  char *UserCommand;
  vtkSetStringMacro(UserCommand);
  vtkGetStringMacro(UserCommand);

private:
  vtkKWLoadSaveButton(const vtkKWLoadSaveButton&); // Not implemented
  void operator=(const vtkKWLoadSaveButton&); // Not implemented
};

#endif

