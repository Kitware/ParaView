/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPickSphereWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPickSphereWidget - A widget to manipulate an implicit plane.
// .SECTION Description
// vtkPVPickSphereWidget can be considered as equivalent to the combination of
// vtkPVLineWidget and vtkPVLineSourceWidget.
// Unlike vtkPVLineWidget, vtkPVPickSphereWidget is never used without the 
// implicit function, hence there was no need to have the distinction here.
// 

#ifndef __vtkPVPickSphereWidget_h
#define __vtkPVPickSphereWidget_h

#include "vtkPVSphereWidget.h"

class vtkKWLabel;
//ATTRIBUTE EDITOR
class vtkKWCheckButton;

class VTK_EXPORT vtkPVPickSphereWidget : public vtkPVSphereWidget
{
public:
  static vtkPVPickSphereWidget* New();
  vtkTypeRevisionMacro(vtkPVPickSphereWidget, vtkPVSphereWidget);

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

protected:
  vtkPVPickSphereWidget();
  ~vtkPVPickSphereWidget();

  // Description:
  // Call creation on the child.
  virtual void ChildCreate();

// ATTRIBUTE EDITOR
  vtkKWLabel* InstructionsLabel;
  vtkKWCheckButton*  MouseControlToggle;
  int MouseControlFlag;

private:
  vtkPVPickSphereWidget(const vtkPVPickSphereWidget&); // Not implemented
  void operator=(const vtkPVPickSphereWidget&); // Not implemented
};

#endif
