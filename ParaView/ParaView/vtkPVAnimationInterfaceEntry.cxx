/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationInterfaceEntry.cxx
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
#include "vtkPVAnimationInterfaceEntry.h"

#include "vtkCollection.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWEntry.h"
#include "vtkPVAnimationInterface.h"
#include "vtkKWRange.h"
#include "vtkPVApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkPVWidget.h"
#include "vtkCommand.h"
#include "vtkKWText.h"
#include "vtkPVWidgetProperty.h"
#include "vtkString.h"

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
vtkCxxRevisionMacro(vtkPVAnimationInterfaceEntry, "1.19.2.3");

vtkCxxSetObjectMacro(vtkPVAnimationInterfaceEntry, CurrentProperty,
                     vtkPVWidgetProperty);

//-----------------------------------------------------------------------------
vtkPVAnimationInterfaceEntry::vtkPVAnimationInterfaceEntry()
{
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
  this->TimeRange = vtkKWRange::New();
  this->ScriptEditorFrame = vtkKWFrame::New();
  this->ScriptEditorScroll = vtkKWWidget::New();
  this->ScriptEditor = vtkKWText::New();
  this->DummyFrame = vtkKWFrame::New();

  this->PVSource = 0;
  this->Script = 0;
  this->CurrentMethod = 0;
  this->TimeStart = 0;
  this->TimeEnd = 100;
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
  
  this->CurrentProperty = NULL;
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
const void vtkPVAnimationInterfaceEntry::CreateLabel(int idx)
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
  this->TimeRange->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->DummyFrame->SetParent(this->TimeScriptEntryFrame->GetFrame());

  this->ScriptEditorFrame->SetParent(this->TimeScriptEntryFrame->GetFrame());
  this->ScriptEditorFrame->Create(pvApp, 0);

  this->ScriptEditor->SetParent(this->ScriptEditorFrame->GetFrame());
  this->ScriptEditorScroll->SetParent(this->ScriptEditorFrame->GetFrame());

  this->SourceMenuButton->GetMenu()->SetTearOff(0);
  this->MethodMenuButton->GetMenu()->SetTearOff(0);

  this->TimeRange->ShowEntriesOn();

  this->SourceLabel->Create(pvApp, 0);
  this->SourceMenuButton->Create(pvApp, 0);
  this->MethodLabel->Create(pvApp, 0);
  this->MethodMenuButton->Create(pvApp, 0);

  this->StartTimeEntry->Create(pvApp, 0);
  this->EndTimeEntry->Create(pvApp, 0);
  this->TimeRange->Create(pvApp, 0);
  this->ScriptEditor->Create(pvApp, "-height 8");
  this->ScriptEditorScroll->Create(pvApp, "scrollbar", "-orient vertical");
  this->DummyFrame->Create(pvApp, "-height 1");

  this->StartTimeEntry->SetLabel("Start value");
  this->EndTimeEntry->SetLabel("End value");

  this->SourceMenuButton->SetBalloonHelpString(
    "Select the filter/source which will be modified by the current action.");
  this->MethodMenuButton->SetBalloonHelpString(
    "Select the property of the selected filter/source to be modified.");
  this->StartTimeEntry->SetBalloonHelpString(
    "This is the value of the property for frame 0. "
    "The value of the selected property is linearly interpolated "
    "between the first and the last frame.");
  this->EndTimeEntry->SetBalloonHelpString(
    "This is the value of the property for the last frame."
    "The value of the selected property is linearly interpolated "
    "between the first and the last frame.");

  if (this->PVSource)
    {
    this->SourceMenuButton->SetButtonText(this->PVSource->GetName());
    }
  else
    {
    this->SourceMenuButton->SetButtonText("None");
    }

  this->SourceLabel->SetLabel("Source");
  this->MethodLabel->SetLabel("Parameter");
  pvApp->Script("grid %s %s -sticky news -pady 2 -padx 2", 
    this->SourceLabel->GetWidgetName(), this->SourceMenuButton->GetWidgetName());
  pvApp->Script("grid %s %s -sticky news -pady 2 -padx 2", 
    this->MethodLabel->GetWidgetName(), this->MethodMenuButton->GetWidgetName());

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

  pvApp->Script(
    "grid %s %s -sticky news", 
    this->ScriptEditor->GetWidgetName(),
    this->ScriptEditorScroll->GetWidgetName());

  pvApp->Script(
    "%s configure -yscrollcommand {%s set}",
    this->ScriptEditor->GetWidgetName(),
    this->ScriptEditorScroll->GetWidgetName());
  pvApp->Script(
    "%s configure -command {%s yview}",
    this->ScriptEditorScroll->GetWidgetName(),
    this->ScriptEditor->GetWidgetName());
  pvApp->Script(
    "grid rowconfigure %s 0 -weight 1\n"
    "grid columnconfigure %s 0 -weight 1",
    this->ScriptEditor->GetParent()->GetWidgetName(),
    this->ScriptEditor->GetParent()->GetWidgetName());

  frame->Delete();
  this->UpdateStartEndValueToEntry();
  this->SetupBinds();

  this->SetLabelAndScript("None", 0);
  this->SwitchScriptTime(-1);
}

//-----------------------------------------------------------------------------
vtkPVAnimationInterfaceEntry::~vtkPVAnimationInterfaceEntry()
{
  this->SetCurrentMethod(0);
  this->SetLabel(0);
  this->SetPVSource(0);
  this->SetSaveStateObject(0);
  this->SetSaveStateScript(0);
  this->SetScript(0);
  this->SetTimeEquation(0);

  this->DummyFrame->Delete();
  this->EndTimeEntry->Delete();
  this->MethodLabel->Delete();
  this->MethodMenuButton->Delete();
  this->Observer->Delete();
  this->ScriptEditor->Delete();
  this->ScriptEditorFrame->Delete();
  this->ScriptEditorScroll->Delete();
  this->SourceLabel->Delete();
  this->SourceMenuButton->Delete();
  this->SourceMethodFrame->Delete();
  this->StartTimeEntry->Delete();
  this->TimeRange->Delete();
  this->TimeScriptEntryFrame->Delete();
  
  this->SetCurrentProperty(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SwitchScriptTime(int i)
{
  vtkKWApplication* pvApp = this->StartTimeEntry->GetApplication();
  pvApp->Script("pack forget %s %s %s %s",
    this->DummyFrame->GetWidgetName(),
    this->ScriptEditorFrame->GetWidgetName(),
    this->StartTimeEntry->GetWidgetName(),
    this->EndTimeEntry->GetWidgetName());
  this->CustomScript = 0;
  if ( i > 0)
    {
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
      this->StartTimeEntry->GetWidgetName());
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
      this->EndTimeEntry->GetWidgetName());
    }
  else if ( ! i )
    {
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
      this->ScriptEditorFrame->GetWidgetName());
    this->CustomScript = 1;
    this->GetMethodMenuButton()->SetButtonText("Script");
    }
  else
    {
    pvApp->Script("pack %s -fill x -expand 1 -pady 2 -padx 2", 
      this->DummyFrame->GetWidgetName());
    this->GetMethodMenuButton()->SetButtonText("None");
    }
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
    button->SetButtonText(this->PVSource->GetName());
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
  this->SetLabelAndScript(0, 0);
  this->SwitchScriptTime(-1);
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
    this->SetLabelAndScript("Script", 0);
    }
  this->Parent->UpdateNewScript();
  this->SwitchScriptTime(0);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateMethodMenu(int samesource /* =1 */)
{
  vtkCollection *pvwProps;
  vtkPVWidgetProperty *pvwp;
  
  // Remove all previous items form the menu.
  vtkKWMenu* menu = this->GetMethodMenuButton()->GetMenu();
  menu->DeleteAllMenuItems();

  this->StartTimeEntry->EnabledOff();
  this->EndTimeEntry->EnabledOff();
  if ( !samesource )
    {
    this->SetCurrentMethod(0);
    this->SetScript(0);
    }
  if (this->GetPVSource() == NULL)
    {
    return;
    }
  
  pvwProps = this->GetPVSource()->GetWidgetProperties();
  pvwProps->InitTraversal();
  while ((pvwp =
          static_cast<vtkPVWidgetProperty*>(pvwProps->GetNextItemAsObject())))
    {
    pvwp->GetWidget()->AddAnimationScriptsToMenu(menu, this);
    }
  char methodAndArgs[1024];
  sprintf(methodAndArgs, "ScriptMethodCallback");
  menu->AddCommand("Script", this, methodAndArgs, 0,"");
  sprintf(methodAndArgs, "NoMethodCallback");
  menu->AddCommand("None", this, methodAndArgs, 0,"");

  if ( samesource && this->GetCurrentMethod() )
    {
    this->GetMethodMenuButton()->SetButtonText(this->GetCurrentMethod());
    this->StartTimeEntry->EnabledOn();
    this->EndTimeEntry->EnabledOn();
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
const char* vtkPVAnimationInterfaceEntry::GetTimeEquation(float vtkNotUsed(tmax))
{
  if ( this->Dirty )
    {
    //cout << "GetTimeEquation; type is: " << this->TypeIsInt << endl;
    this->UpdateStartEndValueFromEntry();
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
    str << " ) ]";
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
  this->ScriptEditor->UnsetBind("<FocusOut>");
  this->ScriptEditor->UnsetBind("<KeyPress>");
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
  this->ScriptEditor->SetBind(this, "<FocusOut>",
    "ScriptEditorCallback");
  this->ScriptEditor->SetBind(this, "<KeyPress-Return>",
    "ScriptEditorCallback");
  this->ScriptEditor->SetBind(this, "<KeyPress>",
    "MarkScriptEditorDirty");
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::MarkScriptEditorDirty()
{ 
  //cout << "MarkScriptEditorDirty" << endl;
  this->ScriptEditorDirty = 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::SetCustomScript(const char* script)
{
  this->CustomScript = 1;
  this->Dirty = 1;
  this->SetScript(script);
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
                                                const char* script)
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
    new_script += this->GetPVSource()->GetName();
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
      *file << "$kw(" << this->GetTclName() << ") SetTimeStart " << this->TimeStart << endl;
      *file << "$kw(" << this->GetTclName() << ") SetTimeEnd " << this->TimeEnd << endl;
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
  os << indent << "Dirty: " << this->Dirty<< endl;

  os << indent << "SourceMenuButton: " << this->SourceMenuButton << endl;
  os << indent << "MethodMenuButton: " << this->MethodMenuButton << endl;
  os << indent << "PVSource: " << this->PVSource<< endl;

  os << indent << "SaveStateScript: " 
    << (this->SaveStateScript?this->SaveStateScript:"(none") << endl;
  
  os << indent << "CurrentProperty: " << this->CurrentProperty << endl;
}

