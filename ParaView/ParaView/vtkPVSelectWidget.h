/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkPVSelectWidget - Select different subwidgets.
// .SECTION Description
// This widget has a selection menu which will pack different
// pvWidgets associated with selection values.  There is also an object
// varible assumed to have different string values for each of the entries.
// This widget was made for selecting clip functions or clip by scalar values.


#ifndef __vtkPVSelectWidget_h
#define __vtkPVSelectWidget_h

#include "vtkPVObjectWidget.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWLabeledFrame.h"

class vtkStringList;
class vtkKWOptionMenu;
class vtkKWLabel;

class VTK_EXPORT vtkPVSelectWidget : public vtkPVObjectWidget
{
public:
  static vtkPVSelectWidget* New();
  vtkTypeMacro(vtkPVSelectWidget, vtkPVObjectWidget);
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  int Create(vtkKWApplication *app);

  // Description:
  // Add widgets to the possible selection.  The vtkValue
  // is value used to set the vtk object variable.
  void AddItem(const char* labelVal, vtkPVWidget *pvw, const char* vtkVal);
  
  // Description:
  // Access to the widgets for tracing.
  vtkPVWidget *GetPVWidget(const char* label);

  // Description:
  // Set the label of the menu.
  void SetLabel(const char *label);

  // Description:
  // Called when accept button is pushed.
  // Adds to the trace file and sets the objects variable from UI.
  virtual void Accept();

  // Description:
  // Called when reset button is pushed.
  // Sets UI current value from objects variable.
  virtual void Reset();

  // Description:
  // This is how the user can query the state of the selection.
  // The value is the label of the widget item.
  const char* GetCurrentValue();
  void SetCurrentValue(const char* val);

  // Description:
  // This method gets called when the menu changes.
  void MenuCallback();

  // Description:
  // All sub widgets should have this frame as their parent.
  vtkKWWidget *GetFrame() {return this->LabeledFrame->GetFrame();}

  // Description:
  // Methods used internally by accept and reset to 
  // Set and Get the widget selection.
  const char* GetCurrentVTKValue();
  void SetCurrentVTKValue(const char* val);

  // Description:
  // For saving the widget into a VTK tcl script.
  void SaveInTclScript(ofstream *file);
    
protected:
  vtkPVSelectWidget();
  ~vtkPVSelectWidget();
  vtkPVSelectWidget(const vtkPVSelectWidget&) {};
  void operator=(const vtkPVSelectWidget&) {};

  int FindIndex(const char* str, vtkStringList *list);
  void SetCurrentIndex(int idx);

  vtkKWLabeledFrame *LabeledFrame;
  vtkKWOptionMenu *Menu;

  // Using this list as an array of strings.
  vtkStringList *Labels;
  vtkStringList *Values;
  vtkCollection *Widgets;

  int CurrentIndex;
};

#endif
