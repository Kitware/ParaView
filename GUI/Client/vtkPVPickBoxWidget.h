/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPickBoxWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPickBoxWidget - A widget to manipulate a box.
// .SECTION Description
// This widget creates and manages its own vtkPlanes on each process.
// I could not decide whether to include the bounds display or not. 
// (I did not.) 


#ifndef __vtkPVPickBoxWidget_h
#define __vtkPVPickBoxWidget_h

#include "vtkPVBoxWidget.h"

class vtkKWLabel;
class vtkCallbackCommand;
// ATTRIBUTE EDITOR
class vtkKWCheckButton;

class VTK_EXPORT vtkPVPickBoxWidget : public vtkPVBoxWidget
{
public:
  static vtkPVPickBoxWidget* New();
  vtkTypeRevisionMacro(vtkPVPickBoxWidget, vtkPVBoxWidget);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

// ATTRIBUTE EDITOR
  void SetMouseControlToggle(int state);
  int GetMouseControlToggleInternal();
  vtkGetObjectMacro(MouseControlToggle,vtkKWCheckButton);


  // Description:
  // Handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

  void OnChar();

protected:
  vtkPVPickBoxWidget();
  ~vtkPVPickBoxWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate();


  // Listens for keyboard and mouse events
  vtkCallbackCommand* EventCallbackCommand; 

// ATTRIBUTE EDITOR
  vtkKWCheckButton* MouseControlToggle;
  int MouseControlFlag;
  vtkKWLabel* InstructionsLabel;

private:
  vtkPVPickBoxWidget(const vtkPVPickBoxWidget&); // Not implemented
  void operator=(const vtkPVPickBoxWidget&); // Not implemented
};

#endif
