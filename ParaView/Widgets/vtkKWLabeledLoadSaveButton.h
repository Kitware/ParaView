/*=========================================================================

  Module:    vtkKWLabeledLoadSaveButton.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLabeledLoadSaveButton - a load/save button with a label
// .SECTION Description
// The vtkKWLabeledLoadSaveButton class creates a load/save button with a label
// in front of it

#ifndef __vtkKWLabeledLoadSaveButton_h
#define __vtkKWLabeledLoadSaveButton_h

#include "vtkKWLabeledWidget.h"

class vtkKWLoadSaveButton;

class VTK_EXPORT vtkKWLabeledLoadSaveButton : public vtkKWLabeledWidget
{
public:
  static vtkKWLabeledLoadSaveButton* New();
  vtkTypeRevisionMacro(vtkKWLabeledLoadSaveButton, vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Convenience method to set the button label.
  void SetLoadSaveButtonLabel(const char *);
  
  // Description:
  // Convenience method to get the button load/save filename.
  char* GetLoadSaveButtonFileName();
  
  // Description:
  // Get the internal object
  vtkGetObjectMacro(LoadSaveButton, vtkKWLoadSaveButton);
  
  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Pack the label last
  virtual void SetPackLabelLast(int);
  vtkGetMacro(PackLabelLast, int);
  vtkBooleanMacro(PackLabelLast, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWLabeledLoadSaveButton();
  ~vtkKWLabeledLoadSaveButton();

  int PackLabelLast;

  vtkKWLoadSaveButton *LoadSaveButton;

  // Pack or repack the widget

  virtual void Pack();

private:
  vtkKWLabeledLoadSaveButton(const vtkKWLabeledLoadSaveButton&); // Not implemented
  void operator=(const vtkKWLabeledLoadSaveButton&); // Not implemented
};

#endif

