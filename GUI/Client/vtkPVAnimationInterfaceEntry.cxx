/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationInterfaceEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationInterfaceEntry.h"

#include "vtkCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledOptionMenu.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWRange.h"
#include "vtkKWText.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderModule.h"
#include "vtkPVSource.h"
#include "vtkPVProcessModule.h"
#include "vtkPVWidget.h"
#include "vtkPVWidgetCollection.h"
#include "vtkString.h"
#include "vtkKWThumbWheel.h"
#include "vtkKWScale.h"
#include "vtkKWLabeledRadioButtonSet.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkSMProperty.h"
#include "vtkSMDomain.h"

#include <vtkstd/string>

#define vtkABS(x) (((x)>0)?(x):-(x))

//===========================================================================
//***************************************************************************
class vtkPVAnimationInterfaceEntryObserver: public vtkCommand
{
public:
  static vtkPVAnimationInterfaceEntryObserver *New() 
    {return new vtkPVAnimationInterfaceEntryObserver;};

  vtkPVAnimationInterfaceEntryObserver()
    {
      this->AnimationEntry = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->AnimationEntry)
        {
        this->AnimationEntry->ExecuteEvent(wdg, event, calldata);
        }
    }

  vtkPVAnimationInterfaceEntry* AnimationEntry;
};

//***************************************************************************
//===========================================================================


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAnimationInterfaceEntry);
vtkCxxRevisionMacro(vtkPVAnimationInterfaceEntry, "1.52");

vtkCxxSetObjectMacro(vtkPVAnimationInterfaceEntry, CurrentSMDomain,
                     vtkSMDomain);

//-----------------------------------------------------------------------------
vtkPVAnimationInterfaceEntry::vtkPVAnimationInterfaceEntry()
{
  this->Parent = 0;
  this->Observer = vtkPVAnimationInterfaceEntryObserver::New();
  this->Observer->AnimationEntry = this;
  this->DeleteEventTag = 0;

  this->SourceMethodFrame = vtkKWFrame::New();
  this->SourceLabel = vtkKWLabel::New();
  this->SourceMenuButton = vtkKWMenuButton::New();
  this->MethodLabel = vtkKWLabel::New();
  this->MethodMenuButton = vtkKWMenuButton::New();
  this->StartTimeEntry = vtkKWLabeledEntry::New();
  this->EndTimeEntry = vtkKWLabeledEntry::New();
  this->ResetRangeButton = vtkKWPushButton::New();
  this->ResetRangeButtonState = 0;
  this->TimeEquationStyleEntry = vtkKWLabeledOptionMenu::New();
  this->TimeEquationPhaseEntry = vtkKWScale::New();
  this->TimeEquationFrequencyEntry = vtkKWThumbWheel::New();
  this->TimeEquationFrame = vtkKWWidget::New();
  this->TimeRange = vtkKWRange::New();
  this->ScriptEditor = vtkKWText::New();
  this->DummyFrame = vtkKWFrame::New();

  this->PVSource = 0;
  this->Script = 0;
  this->CustomScript = 0;
  this->CurrentMethod = 0;
  this->TraceName = 0;
  this->TimeStart = 0;
  this->TimeEnd = 100;
  this->AnimationElement = 0;
  this->TimeEquationStyle = 0;
  this->TimeEquationPhase = 0.;
  this->TimeEquationFrequency = 1.;
  this->TimeEquation = 0;
  this->Label = 0;

  this->TypeIsInt = 0;
  this->CurrentIndex = -1;

  this->UpdatingEntries = 0;

  this->SaveStateScript = 0;
  this->SaveStateObject = 0;

  this->TimeScriptEntryFrame = vtkKWFrame::New();

  this->Dirty = 1;

  this->ScriptEditorDirty = 0;
  
  this->CurrentSMProperty = NULL;
  this->CurrentSMDomain = NULL;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetParent(vtkKWWidget* widget)
{
  this->SourceMethodFrame->SetParent(widget);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::ExecuteEvent(vtkObject *o, 
  unsigned long event, void* calldata)
{
  (void)o;
  (void)event;
  (void)calldata;
  this->SetPVSource(0);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::CreateLabel(int idx)
{
  char index[100];
  sprintf(index, "Action %d", idx);
  vtkstd::string label;
  label = index;
  if ( this->SourceMenuButton->GetButtonText() && 
    strlen(this->SourceMenuButton->GetButtonText()) > 0 &&
    !vtkString::Equals(this->SourceMenuButton->GetButtonText(), "None") )
    {
    label += " (";
    label += this->SourceMenuButton->GetButtonText();
    label += ")";
    }
  /*
  if ( this->MethodMenuButton->GetButtonText() &&
  strlen(this->MethodMenuButton->GetButtonText()) ) 
  {
  label += "_";
  label += this->MethodMenuButton->GetButtonText();
  }
  */
  this->SetLabel(label.c_str());
}

//-----------------------------------------------------------------------------
int vtkPVAnimationInterfaceEntry::IsActionValid(int has_source)
{
  if ( has_source )
    {
    if ( this->PVSource )
      {
      return 1;
      }
    }
  return ( strcmp(this->GetMethodMenuButton()->GetButtonText(), "None") != 0 );
}

//-----------------------------------------------------------------------------
int vtkPVAnimationInterfaceEntry::GetDirty()
{
  this->UpdateStartEndValueFromEntry();
  return this->Dirty;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetCurrentIndex(int idx)
{
  if ( this->CurrentIndex == idx )
    {
    return;
    }
  this->CurrentIndex = idx;
  this->TraceInitialized = 0;
  char buffer[1024];
  sprintf(buffer, "GetSourceEntry %d", idx);
  this->SetTraceReferenceCommand(buffer);
  this->Dirty = 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::Create(vtkPVApplication* pvApp, const char*)
{
  // Call the superclass to set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SourceMethodFrame->Create(pvApp, 0);

  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(this->SourceMethodFrame->GetFrame());
  frame->Create(pvApp, 0);

  this->SourceLabel->SetParent(frame->GetFrame());
  this->SourceMenuButton->SetParent(frame->GetFrame());
  this->MethodLabel->SetParent(frame->GetFrame());
  this->MethodMenuButton->SetParent(frame->GetFrame());

  this->TimeScriptEntryFrame->SetParent(this->SourceMethodFrame->GetFrame());
  this->TimeScriptEntryFrame->Create(pvApp, 0);

  this->StartTimeEntry->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->EndTimeEntry->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->ResetRangeButton->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->TimeRange->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->DummyFrame->SetParent(this->TimeScriptEntryFrame->GetFrame());

  this->TimeEquationStyleEntry->SetLabel( "Waveform:" );
  this->TimeEquationStyleEntry->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->TimeEquationStyleEntry->Create( pvApp, 0 );
  this->TimeEquationStyleEntry->GetOptionMenu()->AddEntryWithCommand(
    "Ramp", this, "UpdateTimeEquationValuesFromEntry");
  this->TimeEquationStyleEntry->GetOptionMenu()->AddEntryWithCommand(
    "Triangle", this, "UpdateTimeEquationValuesFromEntry");
  this->TimeEquationStyleEntry->GetOptionMenu()->AddEntryWithCommand(
    "Sinusoid", this, "UpdateTimeEquationValuesFromEntry");

  this->TimeEquationFrame->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->TimeEquationFrame->Create(pvApp, "frame", "");
  
  this->TimeEquationPhaseEntry->PopupScaleOn();
  this->TimeEquationPhaseEntry->SetParent(this->TimeEquationFrame);
  this->TimeEquationPhaseEntry->Create( pvApp, 0 );
  this->TimeEquationPhaseEntry->SetRange( 0., 360. );
  this->TimeEquationPhaseEntry->SetValue( 0. );
  this->TimeEquationPhaseEntry->SetResolution( 1. );
  this->TimeEquationPhaseEntry->DisplayEntry();
  this->TimeEquationPhaseEntry->DisplayLabel( "Phase" );
  this->TimeEquationPhaseEntry->SetEndCommand( this, "UpdateTimeEquationValuesFromEntry" );
  this->TimeEquationPhaseEntry->SetEntryCommand( this, "UpdateTimeEquationValuesFromEntry" );

  this->TimeEquationFrequencyEntry->PopupModeOn();
  this->TimeEquationFrequencyEntry->SetParent(this->TimeEquationFrame);
  this->TimeEquationFrequencyEntry->Create( pvApp, 0 );
  this->TimeEquationFrequencyEntry->SetValue( 1. );
  this->TimeEquationFrequencyEntry->SetMinimumValue( 0. );
  this->TimeEquationFrequencyEntry->SetClampMinimumValue( 1 );
  this->TimeEquationFrequencyEntry->SetResolution( 0.5 );
  this->TimeEquationFrequencyEntry->DisplayEntryOn();
  this->TimeEquationFrequencyEntry->DisplayLabelOn();
  this->TimeEquationFrequencyEntry->SetLabel( "Frequency" );
  this->TimeEquationFrequencyEntry->SetEndCommand( this, "UpdateTimeEquationValuesFromEntry" );
  this->TimeEquationFrequencyEntry->SetEntryCommand( this, "UpdateTimeEquationValuesFromEntry" );

  pvApp->Script("pack %s %s -side left -fill x -expand y",
                this->TimeEquationPhaseEntry->GetWidgetName(),
                this->TimeEquationFrequencyEntry->GetWidgetName());
  
  this->ScriptEditor->SetParent(this->TimeScriptEntryFrame->GetFrame());

  this->SourceMenuButton->GetMenu()->SetTearOff(0);
  this->MethodMenuButton->GetMenu()->SetTearOff(0);

  this->TimeRange->ShowEntriesOn();

  this->SourceLabel->Create(pvApp, 0);
  this->SourceMenuButton->Create(pvApp, 0);
  this->MethodLabel->Create(pvApp, 0);
  this->MethodMenuButton->Create(pvApp, 0);

  this->StartTimeEntry->Create(pvApp, 0);
  this->EndTimeEntry->Create(pvApp, 0);
  this->ResetRangeButton->Create(pvApp, 0);
  this->TimeRange->Create(pvApp, 0);
  this->ScriptEditor->Create(pvApp, NULL);
  this->ScriptEditor->SetHeight(8);
  this->ScriptEditor->UseVerticalScrollbarOn();
  this->DummyFrame->Create(pvApp, "-height 1");

  this->StartTimeEntry->SetLabel("Start value");
  this->EndTimeEntry->SetLabel("End value");
  this->ResetRangeButton->SetLabel("Reset range");

  this->SourceMenuButton->SetBalloonHelpString(
    "Select the filter/source which will be modified by the current action.");
  this->MethodMenuButton->SetBalloonHelpString(
    "Select the property of the selected filter/source to be modified with time.");
  this->StartTimeEntry->SetBalloonHelpString(
    "This is the value of the property taken on by the waveform when the "
    "phase is zero. ");
  this->EndTimeEntry->SetBalloonHelpString(
    "This is the value of the property at the waveform peak or trough, "
    "depending on whether it is higher or lower than the value given "
    "for a phase of zero.");
  this->ResetRangeButton->SetBalloonHelpString(
    "This button resets the start and end values to appropriate "
    "default values (based on the range of the parameter)");
  this->TimeEquationStyleEntry->SetBalloonHelpString(
    "Choose the waveform of the parameter value over time.\n"
    "Ramp: Interpolate linearly between the start and end value.\n"
    "Triangle: Interpolate linearly from the start to the end value and back.\n"
    "Sinusoid: Vary the parameter value along a sine function where the start"
    " and end values are the minimum and maximum of the function. Controls are"
    " provided for specifying the frequency and phase of the function.");
  this->TimeEquationPhaseEntry->SetBalloonHelpString(
    "Specify the phase of the parameter's time waveform in degrees." );
  this->TimeEquationFrequencyEntry->SetBalloonHelpString(
    "Specify the number of waveform cycles in the animation." );

  if (this->PVSource)
    {
    char* label=this->GetPVApplication()->GetTextRepresentation(this->PVSource);
    this->SourceMenuButton->SetButtonText(label);
    delete[] label;
    }
  else
    {
    this->SourceMenuButton->SetButtonText("None");
    }

  this->SourceLabel->SetLabel("Source");
  this->MethodLabel->SetLabel("Parameter");
  pvApp->Script("grid %s %s -sticky news -pady 2 -padx 2", 
                this->SourceLabel->GetWidgetName(),
                this->SourceMenuButton->GetWidgetName());
  pvApp->Script("grid %s %s -sticky news -pady 2 -padx 2", 
                this->MethodLabel->GetWidgetName(),
                this->MethodMenuButton->GetWidgetName());

  /*
    pvApp->Script("grid %s - - - -sticky news -pady 2 -padx 2", 
    this->TimeRange->GetWidgetName());
  */

  pvApp->Script("pack %s -fill x -expand 1", 
                frame->GetWidgetName());
  pvApp->Script("pack %s -fill x -expand 1", 
                this->TimeScriptEntryFrame->GetWidgetName());

  vtkKWWidget* w = frame->GetFrame();
  pvApp->Script(
    "grid columnconfigure %s 0 -weight 0\n"
    "grid columnconfigure %s 1 -weight 1\n",
    w->GetWidgetName(),
    w->GetWidgetName(),
    w->GetWidgetName(),
    w->GetWidgetName());

  frame->Delete();
  this->UpdateStartEndValueToEntry();
  this->UpdateTimeEquationValuesToEntry();
  this->SetupBinds();

  this->SetLabelAndScript("None", 0, 0);
  this->SwitchScriptTime(-1);

  this->UpdateEnableState();
}
 
//-----------------------------------------------------------------------------
vtkPVAnimationInterfaceEntry::~vtkPVAnimationInterfaceEntry()
{
  this->SetCurrentMethod(0);
  this->SetTraceName(0);
  this->SetLabel(0);
  this->SetPVSource(0);
  this->SetSaveStateObject(0);
  this->SetSaveStateScript(0);
  this->SetScript(0);
  this->SetTimeEquation(0);

  this->DummyFrame->Delete();
  this->EndTimeEntry->Delete();
  this->ResetRangeButton->Delete();
  this->TimeEquationStyleEntry->Delete();
  this->TimeEquationPhaseEntry->Delete();
  this->TimeEquationFrequencyEntry->Delete();
  this->TimeEquationFrame->Delete();
  this->MethodLabel->Delete();
  this->MethodMenuButton->Delete();
  this->Observer->Delete();
  this->ScriptEditor->Delete();
  this->SourceLabel->Delete();
  this->SourceMenuButton->Delete();
  this->SourceMethodFrame->Delete();
  this->StartTimeEntry->Delete();
  this->TimeRange->Delete();
  this->TimeScriptEntryFrame->Delete();

  this->SetCurrentSMProperty(NULL);
  this->SetCurrentSMDomain(NULL);
}
 
//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SwitchScriptTime(int i)
{
  vtkKWApplication* pvApp = this->StartTimeEntry->GetApplication();
  pvApp->Script("catch {eval pack forget %s %s %s %s %s %s %s}",
                this->DummyFrame->GetWidgetName(),
                this->ScriptEditor->GetWidgetName(),
                this->StartTimeEntry->GetWidgetName(),
                this->ResetRangeButton->GetWidgetName(),
                this->EndTimeEntry->GetWidgetName(),
                this->TimeEquationStyleEntry->GetWidgetName(),
                this->TimeEquationFrame->GetWidgetName()
    );
  this->CustomScript = 0;
  if ( i > 0)
    {
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
                  this->StartTimeEntry->GetWidgetName());
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
                  this->EndTimeEntry->GetWidgetName());
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
                  this->ResetRangeButton->GetWidgetName());
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
                  this->TimeEquationStyleEntry->GetWidgetName());
    if (this->TimeEquationStyle == 2)
      {
      pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2",
                    this->TimeEquationFrame->GetWidgetName());
      }
    }
  else if ( ! i )
    {
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
                  this->ScriptEditor->GetWidgetName());
    this->CustomScript = 1;
    this->GetMethodMenuButton()->SetButtonText("Script");
    }
  else
    {
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
                  this->DummyFrame->GetWidgetName());
    this->GetMethodMenuButton()->SetButtonText("None");
    this->CustomScript = 1;
    }
  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
const char* vtkPVAnimationInterfaceEntry::GetWidgetName()
{
  return this->SourceMethodFrame->GetWidgetName();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetPVSource(vtkPVSource* src)
{
//cout << "SetPVSource(" << src << ")  -- replace (" << this->PVSource << ")" << endl;
//cout << "SetPVSource: " << (src?src->GetName():"<none>") << endl;
  if ( src == this->PVSource )
    {
    return;
    }
  if ( this->PVSource )
    {
    //cout << "Remove observer: " << this->DeleteEventTag << " (" << this->PVSource << ")" << endl;
    //this->PVSource->RemoveObservers(this->DeleteEventTag);
    }
  this->PVSource = src;
  vtkKWMenuButton* button = this->GetSourceMenuButton();
  if ( this->PVSource )
    {
    //this->DeleteEventTag = this->PVSource->AddObserver(vtkCommand::DeleteEvent, this->Observer);
    //cout << "Add observer: " << this->DeleteEventTag << " (" << this->PVSource << ")" << endl;
    char* label=this->GetPVApplication()->GetTextRepresentation(this->PVSource);
    button->SetButtonText(label);
    delete[] label;
    //cout << "-- PV source was set to: " << (src?src->GetName():"<none>") << endl;
    if (this->PVSource->InitializeTrace(NULL))
      {
      this->AddTraceEntry("$kw(%s) SetPVSource $kw(%s)", this->GetTclName(), 
                          this->PVSource->GetTclName());
      }
    }
  else
    {
    if ( button->IsCreated())
      {
      button->SetButtonText("None");
      }
    this->AddTraceEntry("$kw(%s) SetPVSource {}", this->GetTclName());
    }
  this->UpdateMethodMenu(0);
  this->Parent->ShowEntryInFrame(this, -1);
  this->Dirty = 1;
  this->Parent->UpdateNewScript();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::NoMethodCallback()
{
  this->AddTraceEntry("$kw(%s) NoMethodCallback", this->GetTclName());
  this->Dirty = 1;
  this->SetCurrentMethod(0);
  this->SetScript(0);
  this->SetLabelAndScript(0, 0, 0);
  this->SwitchScriptTime(-1);

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::ScriptMethodCallback()
{
  this->AddTraceEntry("$kw(%s) ScriptMethodCallback", this->GetTclName());
  this->Dirty = 1;
  this->SetCurrentMethod(0);
  this->UpdateMethodMenu();
  if ( vtkString::Length(this->Script) == 0 )
    {
    this->SetLabelAndScript("Script", 0, 0);
    }
  this->Parent->UpdateNewScript();
  this->SwitchScriptTime(0);

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateMethodMenu(int samesource /* =1 */)
{
  vtkPVWidgetCollection *pvWidgets;
  vtkPVWidget *pvw;

  // Remove all previous items form the menu.
  vtkKWMenu* menu = this->GetMethodMenuButton()->GetMenu();
  menu->DeleteAllMenuItems();

  this->StartTimeEntry->EnabledOff();
  this->EndTimeEntry->EnabledOff();
  this->ResetRangeButton->EnabledOff();
  this->TimeEquationStyleEntry->EnabledOff();
  this->TimeEquationPhaseEntry->EnabledOff();
  this->TimeEquationFrequencyEntry->EnabledOff();
  if ( !samesource )
    {
    this->SetCurrentMethod(0);
    this->SetScript(0);
    }
  if (this->GetPVSource() == NULL)
    {
    return;
    }

  pvWidgets = this->GetPVSource()->GetWidgets();
  pvWidgets->InitTraversal();
  while ((pvw = static_cast<vtkPVWidget*>(pvWidgets->GetNextItemAsObject())))
    {
    if (pvw->GetSupportsAnimation())
      {
      pvw->AddAnimationScriptsToMenu(menu, this);
      }
    }
  char methodAndArgs[1024];
  sprintf(methodAndArgs, "ScriptMethodCallback");
#ifdef PARAVIEW_EXPERIMENTAL_USER
  menu->AddCommand("Script", this, methodAndArgs, 0,"");
#endif //PARAVIEW_EXPERIMENTAL_USER
  sprintf(methodAndArgs, "NoMethodCallback");
  menu->AddCommand("None", this, methodAndArgs, 0,"");

  if ( samesource && this->GetCurrentMethod() )
    {
    this->GetMethodMenuButton()->SetButtonText(this->GetCurrentMethod());
    this->StartTimeEntry->EnabledOn();
    this->EndTimeEntry->EnabledOn();
    if (this->ResetRangeButtonState)
      {
      this->ResetRangeButton->EnabledOn();
      }
    this->TimeEquationStyleEntry->EnabledOn();
    this->TimeEquationPhaseEntry->EnabledOn();
    this->TimeEquationFrequencyEntry->EnabledOn();
    }
  this->Parent->ShowEntryInFrame(this, -1);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTimeStart(float f)
{
  //cout << "Set Time start to: " << f << endl;
  if ( this->TimeStart == f )
    {
    return;
    }
  this->TimeStart = f;
  this->UpdateStartEndValueToEntry();
  if ( !this->StartTimeEntry->IsCreated() ||
       !this->EndTimeEntry->IsCreated() )
    {
    return;
    } 
  this->AddTraceEntry("$kw(%s) SetTimeStart %f", 
                      this->GetTclName(), f);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
  this->Parent->UpdateNewScript();

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTimeEnd(float f)
{
  //cout << "Set Time end to: " << f << endl;
  if ( this->TimeEnd == f )
    {
    return;
    }
  this->TimeEnd = f;
  this->UpdateStartEndValueToEntry();
  if ( !this->StartTimeEntry->IsCreated() ||
       !this->EndTimeEntry->IsCreated() )
    {
    return;
    } 
  this->AddTraceEntry("$kw(%s) SetTimeEnd %f", 
                      this->GetTclName(), f);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
  this->Parent->UpdateNewScript();

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateStartEndValueFromEntry()
{
  //cout << "UpdateStartEndValueFromEntry" << endl;
  if (this->UpdatingEntries)
    {
    return;
    }
  this->UpdatingEntries = 1;
  if ( !this->StartTimeEntry->IsCreated() ||
       !this->EndTimeEntry->IsCreated() )
    {
    return;
    }
  if ( this->TimeStart != this->StartTimeEntry->GetEntry()->GetValueAsFloat() )
    {
    this->SetTimeStart(this->StartTimeEntry->GetEntry()->GetValueAsFloat());
    }
  if ( this->TimeEnd != this->EndTimeEntry->GetEntry()->GetValueAsFloat() )
    {
    this->SetTimeEnd(this->EndTimeEntry->GetEntry()->GetValueAsFloat());
    }
  this->UpdatingEntries = 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateStartEndValueToEntry()
{
  this->StartTimeEntry->GetEntry()->SetValue(this->GetTimeStart());
  this->EndTimeEntry->GetEntry()->SetValue(this->GetTimeEnd());
}

//-----------------------------------------------------------------------------
float vtkPVAnimationInterfaceEntry::GetTimeStartValue()
{
  this->UpdateStartEndValueFromEntry();
  return this->GetTimeStart();
}

//-----------------------------------------------------------------------------
float vtkPVAnimationInterfaceEntry::GetTimeEndValue()
{
  this->UpdateStartEndValueFromEntry();
  return this->GetTimeEnd();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTimeEquationStyle(int s)
{
  //cout << "Set Time equation style to: " << s << endl;
  if ( this->TimeEquationStyle == s )
    {
    return;
    }
  this->TimeEquationStyle = s;
  this->UpdateTimeEquationValuesToEntry();
  if ( !this->TimeEquationStyleEntry->IsCreated() ||
       !this->TimeEquationPhaseEntry->IsCreated() ||
       !this->TimeEquationFrequencyEntry->IsCreated() )
    {
    return;
    } 
  this->AddTraceEntry("$kw(%s) SetTimeEquationStyle %d", 
                      this->GetTclName(), s);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
  this->Parent->UpdateNewScript();

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTimeEquationPhase(float p)
{
  //cout << "Set Time equation phase to: " << p << endl;
  if ( this->TimeEquationPhase == p )
    {
    return;
    }
  this->TimeEquationPhase = p;
  this->UpdateTimeEquationValuesToEntry();
  if ( !this->TimeEquationStyleEntry->IsCreated() ||
       !this->TimeEquationPhaseEntry->IsCreated() ||
       !this->TimeEquationFrequencyEntry->IsCreated() )
    {
    return;
    } 
  this->AddTraceEntry("$kw(%s) SetTimeEquationPhase %f", 
                      this->GetTclName(), p);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
  this->Parent->UpdateNewScript();

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTimeEquationFrequency(float f)
{
  //cout << "Set Time equation style to: " << s << endl;
  if ( this->TimeEquationFrequency == f )
    {
    return;
    }
  this->TimeEquationFrequency = f;
  this->UpdateTimeEquationValuesToEntry();
  if ( !this->TimeEquationStyleEntry->IsCreated() ||
       !this->TimeEquationPhaseEntry->IsCreated() ||
       !this->TimeEquationFrequencyEntry->IsCreated() )
    {
    return;
    } 
  this->AddTraceEntry("$kw(%s) SetTimeEquationFrequency %f", 
                      this->GetTclName(), f);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
  this->Parent->UpdateNewScript(); // ??? FIXME: Is this required?

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateTimeEquationValuesFromEntry()
{
  //cout << "UpdateTimeEquationValuesFromEntry" << endl;
  if (this->UpdatingEntries)
    {
    return;
    }
  this->UpdatingEntries = 1;
  if ( !this->TimeEquationStyleEntry->IsCreated() ||
       !this->TimeEquationPhaseEntry->IsCreated() ||
       !this->TimeEquationFrequencyEntry->IsCreated() )
    {
    return;
    }
  vtkPVApplication *pvApp = this->GetPVApplication();
  int style;
  if (!strcmp(this->TimeEquationStyleEntry->GetOptionMenu()->GetValue(),
              "Triangle"))
    {
    style = 1;
    pvApp->Script("catch {eval pack forget %s}",
                  this->TimeEquationFrame->GetWidgetName());
    }
  else if (!strcmp(this->TimeEquationStyleEntry->GetOptionMenu()->GetValue(),
                   "Sinusoid"))
    {
    style = 2;
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2",
                  this->TimeEquationFrame->GetWidgetName());
    }
  else
    { // default to linear interpolation
    style = 0;
    pvApp->Script("catch {eval pack forget %s}",
                  this->TimeEquationFrame->GetWidgetName());
    }
  if ( this->TimeEquationStyle != style )
    {
    this->SetTimeEquationStyle( style );
    }
  if ( this->TimeEquationPhase != this->TimeEquationPhaseEntry->GetValue() )
    {
    this->SetTimeEquationPhase( this->TimeEquationPhaseEntry->GetValue() );
    }
  if ( this->TimeEquationFrequency != this->TimeEquationFrequencyEntry->GetValue() )
    {
    this->SetTimeEquationFrequency( this->TimeEquationFrequencyEntry->GetValue() );
    }
  this->UpdatingEntries = 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateTimeEquationValuesToEntry()
{
  switch(this->TimeEquationStyle)
    {
    case 0:
      this->TimeEquationStyleEntry->GetOptionMenu()->SetValue("Ramp");
      break;
    case 1:
      this->TimeEquationStyleEntry->GetOptionMenu()->SetValue("Triangle");
      break;
    case 2:
      this->TimeEquationStyleEntry->GetOptionMenu()->SetValue("Sinusoid");
      break;
    }
  
  this->TimeEquationPhaseEntry->SetValue( this->TimeEquationPhase );
  this->TimeEquationFrequencyEntry->SetValue( this->TimeEquationFrequency );
}

//-----------------------------------------------------------------------------
int vtkPVAnimationInterfaceEntry::GetTimeEquationStyleValue()
{
  this->UpdateTimeEquationValuesFromEntry();
  return this->GetTimeEquationStyle();
}

//-----------------------------------------------------------------------------
float vtkPVAnimationInterfaceEntry::GetTimeEquationPhaseValue()
{
  this->UpdateTimeEquationValuesFromEntry();
  return this->GetTimeEquationPhase();
}

//-----------------------------------------------------------------------------
float vtkPVAnimationInterfaceEntry::GetTimeEquationFrequencyValue()
{
  this->UpdateTimeEquationValuesFromEntry();
  return this->GetTimeEquationFrequency();
}

//-----------------------------------------------------------------------------
const char* vtkPVAnimationInterfaceEntry::GetTimeEquation(float vtkNotUsed(tmax))
{
  if ( this->Dirty )
    {
    //cout << "GetTimeEquation; type is: " << this->TypeIsInt << endl;
    this->UpdateStartEndValueFromEntry();
    this->UpdateTimeEquationValuesFromEntry();
    float cmax = this->TimeEnd;
    float cmin = this->TimeStart;
    float range = vtkABS(cmax - cmin);

    // formula is:
    // (((((time - tmin) / trange) / tstep) * range) + cmin) * step
    ostrstream str;
    str << "set pvTime [ expr ";
    if ( this->TypeIsInt )
      {
      str << "round";
      }
    switch ( this->TimeEquationStyle )
      {
      case 0: // Linear ramp (sawtooth wave)
        str << "(((";
        if ( cmax < cmin )
          {
          str << "1 - ";
          }
        str << "$globalPVTime) * " << range << ") + ";
        if ( cmax < cmin )
          {
          str << cmax;
          }
        else
          {
          str << cmin;
          }
        str << " )";
        break;

      case 1: // Triangle wave
        // ( start*|2*t-1| + end*(1-|2*t-1|) )
        // this form is used because it _exactly_ interpolates
        // the start and end values.
        str << "("
            << cmin << " * abs(2*$globalPVTime - 1) + "
            << cmax << " * (1 - abs(2*$globalPVTime - 1)))";
        break;

      case 2: // Sinusoidal wave
        // ( start + (end-start)*sin( 2*pi* (freq*t + phase/360) )/2 )
        str << "((" << cmin << " + " << cmax 
            << " + (" << cmax << " - " << cmin << ") * sin( 8*atan(1)* ("
            << this->TimeEquationFrequency << "*$globalPVTime + "
            << this->TimeEquationPhase << "/360.)))/2.)";
        break;
      }
    str << " ]";
    // add deug? ; puts $pvTime";
    str << ends;
    this->SetTimeEquation(str.str());
    str.rdbuf()->freeze(0);
    this->Dirty = 0;
    }
  return this->GetTimeEquation();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::RemoveBinds()
{
  this->StartTimeEntry->GetEntry()->UnsetBind("<FocusOut>");
  this->StartTimeEntry->GetEntry()->UnsetBind("<KeyPress-Return>");
  this->EndTimeEntry->GetEntry()->UnsetBind("<FocusOut>");
  this->EndTimeEntry->GetEntry()->UnsetBind("<KeyPress-Return>");
  this->TimeEquationPhaseEntry->GetEntry()->UnsetBind("<FocusOut>");
  this->TimeEquationPhaseEntry->GetEntry()->UnsetBind("<KeyPress-Return>");
  this->TimeEquationFrequencyEntry->GetEntry()->UnsetBind("<FocusOut>");
  this->TimeEquationFrequencyEntry->GetEntry()->UnsetBind("<KeyPress-Return>");
  this->ScriptEditor->GetTextWidget()->UnsetBind("<FocusOut>");
  this->ScriptEditor->GetTextWidget()->UnsetBind("<KeyPress>");
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetupBinds()
{
  this->StartTimeEntry->GetEntry()->SetBind(this, "<FocusOut>",
    "UpdateStartEndValueFromEntry");
  this->StartTimeEntry->GetEntry()->SetBind(this, "<KeyPress-Return>",
    "UpdateStartEndValueFromEntry");
  this->EndTimeEntry->GetEntry()->SetBind(this, "<FocusOut>",
    "UpdateStartEndValueFromEntry");
  this->EndTimeEntry->GetEntry()->SetBind(this, "<KeyPress-Return>",
    "UpdateStartEndValueFromEntry"); 
  this->TimeEquationPhaseEntry->GetEntry()->SetBind(this, "<FocusOut>",
    "UpdateTimeEquationValuesFromEntry");
  this->TimeEquationPhaseEntry->GetEntry()->SetBind(this, "<KeyPress-Return>",
    "UpdateTimeEquationValuesFromEntry");
  this->TimeEquationFrequencyEntry->GetEntry()->SetBind(this, "<FocusOut>",
    "UpdateTimeEquationValuesFromEntry");
  this->TimeEquationFrequencyEntry->GetEntry()->SetBind(this, "<KeyPress-Return>",
    "UpdateTimeEquationValuesFromEntry");
  this->ScriptEditor->GetTextWidget()->SetBind(this, "<FocusOut>",
    "ScriptEditorCallback");
  this->ScriptEditor->GetTextWidget()->SetBind(this, "<KeyPress-Return>",
    "ScriptEditorCallback");
  this->ScriptEditor->GetTextWidget()->SetBind(this, "<KeyPress>",
    "MarkScriptEditorDirty");
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::MarkScriptEditorDirty()
{ 
  //cout << "MarkScriptEditorDirty" << endl;
  this->ScriptEditorDirty = 1;

  vtkPVApplication* app = this->GetPVApplication();
  if (app)
    {
    app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
    }

}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetCurrentSMProperty(vtkSMProperty *prop)
{
  if (prop != this->CurrentSMProperty)
    {
    if (this->CurrentSMProperty)
      {
      this->CurrentSMProperty->UnRegister(this);
      }
    this->CurrentSMProperty = prop;
    if (this->CurrentSMProperty)
      {
      this->CurrentSMProperty->Register(this);
      vtkPVApplication *app = this->GetPVApplication();
      if (app)
        {
        app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
        }
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetAnimationElement(int elem)
{
  if (elem != this->AnimationElement)
    {
    this->AnimationElement = elem;
    vtkPVApplication *app = this->GetPVApplication();
    if (app)
      {
      app->GetProcessModule()->GetRenderModule()->InvalidateAllGeometries();
      }
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetCustomScript(const char* script)
{
  this->CustomScript = 1;
  this->Dirty = 1;
  this->SetScript(script);
  if ( !this->Parent )
    {
    return;
    }
  this->AddTraceEntry("$kw(%s) SetCustomScript {%s}", this->GetTclName(),
                      script);
  this->GetMethodMenuButton()->SetButtonText("Script");
  this->Parent->UpdateNewScript();
  this->Parent->ShowEntryInFrame(this, -1);
  this->SwitchScriptTime(0);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::ScriptEditorCallback()
{
  if ( !this->ScriptEditorDirty )
    {
    return;
    }
  this->SetCustomScript(this->ScriptEditor->GetValue());
  this->ScriptEditorDirty = 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTypeToFloat()
{
  this->TypeIsInt = 0;
  this->Dirty = 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetTypeToInt()
{
  this->TypeIsInt = 1;
  this->Dirty = 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetParent(vtkPVAnimationInterface* ai)
{ 
  this->Parent = ai; 
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetLabelAndScript(const char* label,
                                                     const char* script,
                                                     const char* traceName)
{
  vtkstd::string new_script;
  if ( this->GetPVSource() && this->GetPVSource()->GetTclName() )
    {
    new_script = "# globalPVTime is provided by the animation\n"
      "# interface for convenience.\n"
      "# It varies linearly between 0 and 1 (0 at the\n"
      "# first frame, 1 at the last frame).\n"
      "\n"
      "# The source modified is: ";
    new_script += this->GetPVSource()->GetTclName();
    new_script += "\n";
    }
  if ( script )
    {
    new_script += script;
    }
  if ( !vtkString::Equals(this->CurrentMethod, label) )
    {
    this->SetCurrentMethod(label);
    //cout << __LINE__ << " Dirty" << endl;
    this->Dirty = 1;
    }
  this->GetMethodMenuButton()->SetButtonText(label);
  if ( !vtkString::Equals(this->Script, new_script.c_str()) )
    {
    this->SetScript(new_script.c_str());
    //cout << __LINE__ << " Dirty" << endl;
    this->Dirty = 1;
    }
  if ( this->Dirty )
    {
    this->SetTypeToFloat();
    //this->AddTraceEntry("$kw(%s) SetLabelAndScript {%s} {%s}", 
    //  this->GetTclName(), label, script);
    }
  this->Parent->UpdateNewScript();

  this->SetTraceName(traceName);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::Update()
{
  //cout << "Type is: " << this->TypeIsInt << endl;
  this->SwitchScriptTime(1);
  this->Parent->UpdateNewScript();
  this->Parent->ShowEntryInFrame(this, -1);
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SaveState(ofstream* file)
{
  if ( this->GetPVSource() )
    {
    *file << "$kw(" << this->GetTclName() << ") SetPVSource $kw(" 
          << this->GetPVSource()->GetTclName() << ")" << endl;
    if ( this->CurrentMethod )
      {
      *file << "$kw(" << this->GetTclName() << ") SetCurrentMethod {"
            << this->CurrentMethod << "}" << endl;
      if (this->CurrentSMProperty && this->CurrentSMDomain)
        {
        *file << "$kw(" << this->GetTclName() << ") SetCurrentSMProperty [["
              << "$kw(" << this->GetPVSource()->GetTclName()
              << ") GetPVWidget {" << this->GetTraceName()
              << "}] GetSMProperty]" << endl;
        *file << "$kw(" << this->GetTclName() << ") SetCurrentSMDomain [["
              << "$kw(" <<this->GetTclName()
              << ") GetCurrentSMProperty] GetDomain {range}]" << endl;
        }

      *file << "$kw(" << this->GetTclName() << ") SetLabelAndScript {"
            << this->CurrentMethod << "} \"\" " 
            << this->GetTraceName() << endl;
      *file << "$kw(" << this->GetTclName() << ") SetTimeStart " << this->TimeStart << endl;
      *file << "$kw(" << this->GetTclName() << ") SetTimeEnd " << this->TimeEnd << endl;
      *file << "$kw(" << this->GetTclName() << ") SetTimeEquationStyle " << this->TimeEquationStyle << endl;
      *file << "$kw(" << this->GetTclName() << ") SetTimeEquationPhase " << this->TimeEquationPhase << endl;
      *file << "$kw(" << this->GetTclName() << ") SetTimeEquationFrequency " << this->TimeEquationFrequency << endl;
      *file << "$kw(" << this->GetTclName() << ") Update" << endl;
      if ( this->SaveStateScript  && this->SaveStateObject )
        {
        *file << "$kw(" << this->SaveStateObject->GetTclName() << ") " << this->SaveStateScript << endl;
        }
      }
    if ( this->CustomScript )
      {
      *file << "$kw(" << this->GetTclName() << ") SetCustomScript {" 
            << this->Script << "}" << endl;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetScript(const char* scr)
{
  //cout << "SetScript: " << scr << endl;
  if ( vtkString::Equals(scr, this->Script) )
    {
    return;
    }
  if ( this->Script )
    {
    delete [] this->Script;
    this->Script = 0;
    }
  this->Script = vtkString::Duplicate(scr);

  if ( !this->ScriptEditor->IsCreated() && this->ScriptEditor->IsAlive() )
    {
    return;
    }
  //cout << "SetScriptEditor: " << scr << endl;
  this->ScriptEditor->SetValue(scr);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::Prepare()
{
  if ( this->ScriptEditorDirty )
    {
    this->ScriptEditorCallback();
    }
}

//-----------------------------------------------------------------------------
vtkPVApplication* vtkPVAnimationInterfaceEntry::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->SourceMethodFrame);
  this->PropagateEnableState(this->SourceLabel);
  this->PropagateEnableState(this->SourceMenuButton);
  this->PropagateEnableState(this->TimeScriptEntryFrame);
  this->PropagateEnableState(this->StartTimeEntry);
  this->PropagateEnableState(this->EndTimeEntry);
  this->PropagateEnableState(this->TimeEquationStyleEntry);
  this->PropagateEnableState(this->TimeEquationPhaseEntry);
  this->PropagateEnableState(this->TimeEquationFrequencyEntry);
  this->PropagateEnableState(this->MethodLabel);
  this->PropagateEnableState(this->MethodMenuButton);
  this->PropagateEnableState(this->TimeRange);
  this->PropagateEnableState(this->DummyFrame);
  this->PropagateEnableState(this->ScriptEditor);

  if (this->ResetRangeButtonState)
    {
    this->PropagateEnableState(this->ResetRangeButton);
    }
  else
    {
    this->ResetRangeButton->EnabledOff();
    this->ResetRangeButton->UpdateEnableState();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Label: " << (this->Label?this->Label:"(none)") << endl;
  os << indent << "Script: " << (this->Script?this->Script:"(none)") << endl;
  os << indent << "CurrentMethod: " 
     << (this->CurrentMethod?this->CurrentMethod:"(none)") << endl;
  os << indent << "TimeEquation: " 
     << (this->TimeEquation?this->TimeEquation:"(none)") << endl;
  os << indent << "TimeStart: " << this->TimeStart << endl;
  os << indent << "TimeEnd: " << this->TimeEnd<< endl;
  os << indent << "TimeEquationStyle: ";
  switch ( this->TimeEquationStyle )
    {
    case 0:
      os << "Ramp" << endl;
      break;
    case 1:
      os << "Triangle" << endl;
      break;
    case 2:
      os << "Sinusiod" << endl;
      break;
    default:
      os << "(Unknown! Danger Will Robinson!)" << endl;
    }
  os << indent << "TimeEquationPhase: " << this->TimeEquationPhase << endl;
  os << indent << "TimeEquationFrequency: " << this->TimeEquationFrequency << endl;
  os << indent << "Dirty: " << this->Dirty<< endl;

  os << indent << "SourceMenuButton: " << this->SourceMenuButton << endl;
  os << indent << "MethodMenuButton: " << this->MethodMenuButton << endl;
  os << indent << "PVSource: " << this->PVSource<< endl;
  os << indent << "TimeEquationStyleEntry: " << this->TimeEquationStyleEntry << endl;  os << indent << "TimeEquationPhaseEntry: " << this->TimeEquationPhaseEntry << endl;
  os << indent << "TimeEquationFrequencyEntry: " << this->TimeEquationFrequencyEntry << endl;

  os << indent << "SaveStateScript: " 
     << (this->SaveStateScript?this->SaveStateScript:"(none") << endl;
  
  os << indent << "CurrentSMProperty: " << this->CurrentSMProperty << endl;
  os << indent << "CurrentSMDomain: " << this->CurrentSMDomain << endl;
  
  os << indent << "CustomScript: " << this->CustomScript << endl;
  
  os << indent << "AnimationElement: " << this->AnimationElement << endl;

  os << indent << "ResetRangeButtonState: " << this->ResetRangeButtonState << endl;

  os << indent << "ResetRangeButton: " << this->ResetRangeButton << endl;
}
