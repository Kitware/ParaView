/*=========================================================================

  Module:    vtkKWScalarComponentSelectionWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWScalarComponentSelectionWidget - a scalar component selection widget
// .SECTION Description
// This class contains the UI for scalar component selection.

#ifndef __vtkKWScalarComponentSelectionWidget_h
#define __vtkKWScalarComponentSelectionWidget_h

#include "vtkKWWidget.h"

class vtkKWOptionMenuLabeled;

class KWWIDGETS_EXPORT vtkKWScalarComponentSelectionWidget : public vtkKWWidget
{
public:
  static vtkKWScalarComponentSelectionWidget* New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkKWScalarComponentSelectionWidget,vtkKWWidget);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Are the components independent of each other?
  virtual void SetIndependentComponents(int);
  vtkGetMacro(IndependentComponents, int);
  vtkBooleanMacro(IndependentComponents, int);
  
  // Description:
  // Set/get the number of components controlled by the widget
  virtual void SetNumberOfComponents(int);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // Set/get the current component controlled by the widget (if controllable)
  virtual void SetSelectedComponent(int);
  vtkGetMacro(SelectedComponent, int);

  // Description:
  // Allow component selection (a quick way to hide the UI)
  virtual void SetAllowComponentSelection(int);
  vtkBooleanMacro(AllowComponentSelection, int);
  vtkGetMacro(AllowComponentSelection, int);

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Set the command called when the selected component is changed.
  // Note that the selected component is passed as a parameter.
  virtual void SetSelectedComponentChangedCommand(
    vtkKWObject* object, const char *method);
  virtual void InvokeSelectedComponentChangedCommand();

  // Description:
  // Callbacks
  virtual void SelectedComponentCallback(int);

  // Description:
  // Access to objects
  vtkGetObjectMacro(SelectedComponentOptionMenu, vtkKWOptionMenuLabeled);
 
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWScalarComponentSelectionWidget();
  ~vtkKWScalarComponentSelectionWidget();

  int IndependentComponents;
  int NumberOfComponents;
  int SelectedComponent;
  int AllowComponentSelection;

  // Commands

  char  *SelectedComponentChangedCommand;

  // GUI

  vtkKWOptionMenuLabeled *SelectedComponentOptionMenu;

  // Pack
  virtual void Pack();

private:
  vtkKWScalarComponentSelectionWidget(const vtkKWScalarComponentSelectionWidget&); // Not implemented
  void operator=(const vtkKWScalarComponentSelectionWidget&); // Not implemented
};

#endif
