/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWScalarComponentSelectionWidget - a scalar component selection widget
// .SECTION Description
// This class contains the UI for scalar component selection.

#ifndef __vtkKWScalarComponentSelectionWidget_h
#define __vtkKWScalarComponentSelectionWidget_h

#include "vtkKWWidget.h"

class vtkKWLabeledOptionMenu;

class VTK_EXPORT vtkKWScalarComponentSelectionWidget : public vtkKWWidget
{
public:
  static vtkKWScalarComponentSelectionWidget* New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkKWScalarComponentSelectionWidget,vtkKWWidget);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, char *args);

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
  vtkGetObjectMacro(SelectedComponentOptionMenu, vtkKWLabeledOptionMenu);
 
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

  vtkKWLabeledOptionMenu *SelectedComponentOptionMenu;

  // Pack
  virtual void Pack();

private:
  vtkKWScalarComponentSelectionWidget(const vtkKWScalarComponentSelectionWidget&); // Not implemented
  void operator=(const vtkKWScalarComponentSelectionWidget&); // Not implemented
};

#endif
