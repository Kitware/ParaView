/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationInterfaceEntry.h
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
// .NAME vtkPVAnimationInterfaceEntry -
// .SECTION Description

#ifndef __vtkPVAnimationInterfaceEntry_h
#define __vtkPVAnimationInterfaceEntry_h

#include "vtkKWObject.h"

class vtkPVApplication;
class vtkKWWidget;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWMenuButton;
class vtkKWLabeledEntry;
class vtkPVSource;
class vtkPVAnimationInterface;
class vtkKWRange;
class vtkPVAnimationInterfaceEntryObserver;
class vtkKWText;
class vtkPVWidgetProperty;

class VTK_EXPORT vtkPVAnimationInterfaceEntry : public vtkKWObject
{
public:
  static vtkPVAnimationInterfaceEntry* New();
  vtkTypeRevisionMacro(vtkPVAnimationInterfaceEntry, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetParent(vtkKWWidget* widget);

  const void CreateLabel(int idx);

  void Create(vtkPVApplication* pvApp, const char*);

  const char* GetWidgetName();

  vtkGetObjectMacro(SourceMenuButton, vtkKWMenuButton);
  vtkGetObjectMacro(MethodMenuButton, vtkKWMenuButton);

  vtkGetObjectMacro(PVSource, vtkPVSource);
  void SetPVSource(vtkPVSource* src);

  vtkGetStringMacro(Script);
  void SetScript(const char* scr);
  void ScriptEditorCallback();

  vtkSetStringMacro(CurrentMethod);
  vtkGetStringMacro(CurrentMethod);

  void SetCurrentProperty(vtkPVWidgetProperty *prop);
  vtkGetObjectMacro(CurrentProperty, vtkPVWidgetProperty);
  
  void SetTimeStart(float f);

  void SetTimeEnd(float f);

  void UpdateStartEndValueToEntry();
  void UpdateStartEndValueFromEntry();

  float GetTimeStartValue();

  float GetTimeEndValue();

  vtkGetMacro(TimeStart, float);
  vtkGetMacro(TimeEnd, float);

  const char* GetTimeEquation(float vtkNotUsed(tmax));

  void SetupBinds();
  void RemoveBinds();

  void SetTypeToFloat();

  void SetTypeToInt();

  vtkGetStringMacro(TimeEquation);
  vtkSetStringMacro(TimeEquation);
  vtkGetStringMacro(Label);
  vtkSetStringMacro(Label);

  void SetParent(vtkPVAnimationInterface* ai);

  void SetCurrentIndex(int idx);

  void SetLabelAndScript(const char* label, const char* script);

  // Description:
  // This method has to be called in PV widget after all modifications of this
  // object are made.
  void Update();

  void UpdateMethodMenu(int samesource=1);

  void ExecuteEvent(vtkObject *o, unsigned long event, void* calldata);

  vtkSetClampMacro(Dirty, int, 0, 1);

  int GetDirty();

  // Description:
  // This is a method call when None menu entry is selected.
  void NoMethodCallback();

  // Description:
  // This is a method call when Script menu entry is selected.
  void ScriptMethodCallback();

  // Description:
  // This method saves state of animation entry.
  void SaveState(ofstream *file);

  // Description:
  // Set the save state script.
  vtkSetStringMacro(SaveStateScript);
  void SetSaveStateObject(vtkKWObject* o)
    {
    this->SaveStateObject = o;
    }

  // Description:
  // Switch between script and time
  // i = 0 - script
  // i > 0 - time
  // i < 0 - none
  void SwitchScriptTime(int i);

  // Description:
  // Set custom script
  void SetCustomScript(const char* script);

  // Description:
  // Prepare to be played or whatever.
  void Prepare();

  // Description:
  // Callback when key is pressed in the script editor.
  void MarkScriptEditorDirty();

protected:
  vtkPVAnimationInterfaceEntry();
  ~vtkPVAnimationInterfaceEntry();
  
  vtkPVAnimationInterface* Parent;
  vtkKWFrame *SourceMethodFrame;
  vtkKWLabel *SourceLabel;
  vtkKWMenuButton *SourceMenuButton;

  vtkKWFrame *TimeScriptEntryFrame;
  vtkKWLabeledEntry *StartTimeEntry;
  vtkKWLabeledEntry *EndTimeEntry;

  // Menu Showing all of the possible methods of the selected source.
  vtkKWLabel*        MethodLabel;
  vtkKWMenuButton*   MethodMenuButton;
  vtkPVSource*       PVSource;

  vtkKWRange*        TimeRange;

  vtkKWFrame*        DummyFrame;

  vtkKWText*         ScriptEditor;
  vtkKWFrame*        ScriptEditorFrame;
  vtkKWWidget*       ScriptEditorScroll;

  vtkKWObject*       SaveStateObject;
  char*              SaveStateScript;
  char*              Script;
  char*              CurrentMethod;
  char*              TimeEquation;
  char*              Label;

  vtkPVWidgetProperty *CurrentProperty;
  
  float TimeStart;
  float TimeEnd;

  int TypeIsInt;
  int CurrentIndex;
  int UpdatingEntries;

  int Dirty;

  int ScriptEditorDirty;

  int CustomScript;

  unsigned long DeleteEventTag;
  vtkPVAnimationInterfaceEntryObserver* Observer;

  vtkPVAnimationInterfaceEntry(const vtkPVAnimationInterfaceEntry&); // Not implemented
  void operator=(const vtkPVAnimationInterfaceEntry&); // Not implemented
};

#endif
