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
// the button label. Note that if the dialog is cancelled, the button
// will not be reset to an empty string, therefore reflecting the
// previously selected file, if any (which is the more logical behavior).
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
  // Access to sub-widgets.
  vtkGetObjectMacro(LoadSaveDialog, vtkKWLoadSaveDialog);

  // Description:
  // Retrieve the filename. This method only query the GetFileName method
  // on the LoadSaveDialog member.
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
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Add all the default observers needed by that object, or remove
  // all the observers that were added through AddCallbackCommandObserver.
  // Subclasses can override these methods to add/remove their own default
  // observers, but should call the superclass too.
  virtual void AddCallbackCommandObservers();
  virtual void RemoveCallbackCommandObservers();

protected:
  vtkKWLoadSaveButton();
  ~vtkKWLoadSaveButton();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWLoadSaveDialog *LoadSaveDialog;

  int TrimPathFromFileName;
  int MaximumFileNameLength;
  virtual void UpdateTextFromFileName();

  virtual void InvokeCommand();

  // Description:
  // Processes the events that are passed through CallbackCommand (or others).
  // Subclasses can oberride this method to process their own events, but
  // should call the superclass too.
  virtual void ProcessCallbackCommandEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  
private:
  vtkKWLoadSaveButton(const vtkKWLoadSaveButton&); // Not implemented
  void operator=(const vtkKWLoadSaveButton&); // Not implemented
};

#endif

