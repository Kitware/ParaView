/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationInterfaceEntry.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVAnimationInterfaceEntry -
// .SECTION Description

#ifndef __vtkPVAnimationInterfaceEntry_h
#define __vtkPVAnimationInterfaceEntry_h

#include "vtkKWWidget.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWEntryLabeled;
class vtkKWOptionMenuLabeled;
class vtkKWMenuButton;
class vtkKWPushButton;
class vtkKWRange;
class vtkKWScale;
class vtkKWText;
class vtkKWThumbWheel;
class vtkKWWidget;
class vtkPVAnimationInterface;
class vtkPVAnimationInterfaceEntryObserver;
class vtkPVApplication;
class vtkPVSource;
class vtkSMDomain;
class vtkSMProperty;

class VTK_EXPORT vtkPVAnimationInterfaceEntry : public vtkKWWidget
{
public:
  static vtkPVAnimationInterfaceEntry* New();
  vtkTypeRevisionMacro(vtkPVAnimationInterfaceEntry, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetParent(vtkKWWidget* widget);

  void CreateLabel(int idx);

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

  // Description:
  // Set the current method using the index in the Methods menu.
  // It invokes the command of the menu item at the given idx,
  // this simulating the effect that the user choose the given index
  // method from the methods menu.
  void SetCurrentMethodIndex(int idx);
  
  void SetCurrentSMProperty(vtkSMProperty *prop);
  vtkGetObjectMacro(CurrentSMProperty, vtkSMProperty);
  
  void SetCurrentSMDomain(vtkSMDomain *domain);
  vtkGetObjectMacro(CurrentSMDomain, vtkSMDomain);

  void SetTimeStart(float f);

  void SetTimeEnd(float f);

  void UpdateStartEndValueToEntry();
  void UpdateStartEndValueFromEntry();

  float GetTimeStartValue();

  float GetTimeEndValue();

  vtkGetMacro(TimeStart, float);
  vtkGetMacro(TimeEnd, float);

  void SetAnimationElement(int elem);
  vtkGetMacro(AnimationElement, int);
  
  void SetTimeEquationStyle( int s );
  void SetTimeEquationPhase( float p );
  void SetTimeEquationFrequency( float f );

  void UpdateTimeEquationValuesToEntry();
  void UpdateTimeEquationValuesFromEntry();

  int GetTimeEquationStyleValue();
  float GetTimeEquationPhaseValue();
  float GetTimeEquationFrequencyValue();

  vtkGetMacro(TimeEquationStyle,int);
  vtkGetMacro(TimeEquationPhase,float);
  vtkGetMacro(TimeEquationFrequency,float);

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

  void SetLabelAndScript(const char* label, 
                         const char* script, 
                         const char* traceName);

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
  // Is the entry doing a custom script?
  vtkGetMacro(CustomScript, int);

  // Description:
  // Prepare to be played or whatever.
  void Prepare();

  // Description:
  // Callback when key is pressed in the script editor.
  void MarkScriptEditorDirty();

  // Description:
  // Return 1 if the action is valid, so if there is script or there is some
  // not none action. If the argument has_source is 1, then the action will be
  // valiad if the source is present.
  int IsActionValid(int has_source = 0);

  // Description:
  // Convenience method.
  vtkPVApplication* GetPVApplication();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Widget who support resetting range should set this to true
  // and bind the right method to ResetRangeButton in their
  // AnimationMenuCallback.
  vtkGetMacro(ResetRangeButtonState, int);
  vtkSetMacro(ResetRangeButtonState, int);

  vtkGetObjectMacro(ResetRangeButton, vtkKWPushButton);

protected:
  vtkPVAnimationInterfaceEntry();
  ~vtkPVAnimationInterfaceEntry();
  
  vtkPVAnimationInterface* Parent;
  vtkKWFrame *SourceMethodFrame;
  vtkKWLabel *SourceLabel;
  vtkKWMenuButton *SourceMenuButton;

  vtkKWFrame *TimeScriptEntryFrame;
  vtkKWEntryLabeled *StartTimeEntry;
  vtkKWEntryLabeled *EndTimeEntry;
  vtkKWOptionMenuLabeled *TimeEquationStyleEntry;
  vtkKWScale *TimeEquationPhaseEntry;
  vtkKWThumbWheel *TimeEquationFrequencyEntry;
  vtkKWWidget *TimeEquationFrame;

  // Menu Showing all of the possible methods of the selected source.
  vtkKWLabel*        MethodLabel;
  vtkKWMenuButton*   MethodMenuButton;
  vtkPVSource*       PVSource;

  vtkKWRange*        TimeRange;

  vtkKWFrame*        DummyFrame;

  vtkKWText*         ScriptEditor;

  vtkKWPushButton*   ResetRangeButton;
  int ResetRangeButtonState;

  vtkKWObject*       SaveStateObject;
  char*              SaveStateScript;
  char*              Script;
  char*              CurrentMethod;
  char*              TraceName;
  char*              TimeEquation;
  char*              Label;

  vtkSetStringMacro(TraceName);
  vtkGetStringMacro(TraceName);

  vtkSMProperty *CurrentSMProperty;
  vtkSMDomain *CurrentSMDomain;
  
  float TimeStart;
  float TimeEnd;

  int AnimationElement;
  
  int TimeEquationStyle;
  float TimeEquationPhase;
  float TimeEquationFrequency;

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
