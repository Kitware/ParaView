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
#include "vtkPVWidgetCollection.h"
#include "vtkPVWidget.h"
#include "vtkCommand.h"

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
vtkCxxRevisionMacro(vtkPVAnimationInterfaceEntry, "1.4");

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

  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
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
  //cout << "Source deleted" << endl;
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
  //cout << "Current index: " << this->CurrentIndex << " (" << idx << ")" << endl;
  if ( this->CurrentIndex == idx )
    {
    return;
    }
  this->CurrentIndex = idx;
  this->TraceInitialized = 0;
  char buffer[1024];
  sprintf(buffer, "GetSourceEntry %d", idx);
  this->SetTraceReferenceCommand(buffer);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::Create(vtkPVApplication* pvApp, const char*)
{
  this->SourceMethodFrame->Create(pvApp, 0);
  this->SourceLabel->SetParent(this->SourceMethodFrame->GetFrame());
  this->SourceMenuButton->SetParent(this->SourceMethodFrame->GetFrame());
  this->MethodLabel->SetParent(this->SourceMethodFrame->GetFrame());
  this->MethodMenuButton->SetParent(this->SourceMethodFrame->GetFrame());
  this->StartTimeEntry->SetParent(this->SourceMethodFrame->GetFrame());
  this->EndTimeEntry->SetParent(this->SourceMethodFrame->GetFrame());
  this->TimeRange->SetParent(this->SourceMethodFrame->GetFrame());

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
  this->MethodLabel->SetLabel("Method");
  pvApp->Script("grid %s %s -sticky news -pady 2 -padx 2", 
    this->SourceLabel->GetWidgetName(), this->SourceMenuButton->GetWidgetName());
  pvApp->Script("grid %s %s -sticky news -pady 2 -padx 2", 
    this->MethodLabel->GetWidgetName(), this->MethodMenuButton->GetWidgetName());
  pvApp->Script("grid %s - -sticky news -pady 2 -padx 2", 
    this->StartTimeEntry->GetWidgetName());
  pvApp->Script("grid %s - -sticky news -pady 2 -padx 2", 
    this->EndTimeEntry->GetWidgetName());
  /*
  pvApp->Script("grid %s - - - -sticky news -pady 2 -padx 2", 
  this->TimeRange->GetWidgetName());
  */

  vtkKWWidget* w = this->SourceMethodFrame->GetFrame();
  pvApp->Script(
    "grid columnconfigure %s 0 -weight 0\n"
    "grid columnconfigure %s 1 -weight 1\n",
    w->GetWidgetName(),
    w->GetWidgetName(),
    w->GetWidgetName(),
    w->GetWidgetName());
  this->UpdateStartEndValueToEntry();
  this->SetupBinds();
}

//-----------------------------------------------------------------------------
vtkPVAnimationInterfaceEntry::~vtkPVAnimationInterfaceEntry()
{
  this->SetPVSource(0);
  this->Observer->Delete();
  this->TimeRange->Delete();
  this->SourceMethodFrame->Delete();
  this->SourceLabel->Delete();
  this->SourceMenuButton->Delete();
  this->MethodLabel->Delete();
  this->MethodMenuButton->Delete();
  this->StartTimeEntry->Delete();
  this->EndTimeEntry->Delete();
  this->SetScript(0);
  this->SetCurrentMethod(0);
  this->SetTimeEquation(0);
  this->SetLabel(0);
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
  this->Parent->ShowEntryInFrame(this);
  //cout << __LINE__ << " Dirty" << endl;
  this->Dirty = 1;
  this->Parent->UpdateNewScript();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::NoMethodCallback()
{
  this->Dirty = 1;
  this->SetCurrentMethod(0);
  this->SetScript(0);
  this->UpdateMethodMenu();
  this->Parent->UpdateNewScript();
}
//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::UpdateMethodMenu(int samesource /* =1 */)
{
  vtkPVWidgetCollection *pvWidgets;
  vtkPVWidget *pvw;

  // Remove all previous items form the menu.
  vtkKWMenu* menu = this->GetMethodMenuButton()->GetMenu();
  menu->DeleteAllMenuItems();

  this->GetMethodMenuButton()->SetButtonText("None");
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
  
  pvWidgets = this->GetPVSource()->GetWidgets();
  pvWidgets->InitTraversal();
  while ( (pvw = pvWidgets->GetNextPVWidget()) )
    {
    pvw->AddAnimationScriptsToMenu(menu, this);
    }
  char methodAndArgs[1024];
  sprintf(methodAndArgs, "NoMethodCallback");
  menu->AddCommand("None", this, methodAndArgs, 0,"");

  if ( samesource && this->GetCurrentMethod() )
    {
    this->GetMethodMenuButton()->SetButtonText(this->GetCurrentMethod());
    this->StartTimeEntry->EnabledOn();
    this->EndTimeEntry->EnabledOn();
    }
  this->Parent->ShowEntryInFrame(this);
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
  if ( !vtkString::Equals(this->CurrentMethod, label) )
    {
    this->SetCurrentMethod(label);
    //cout << __LINE__ << " Dirty" << endl;
    this->Dirty = 1;
    }
  this->GetMethodMenuButton()->SetButtonText(label);
  if ( !vtkString::Equals(this->Script, script) )
    {
    this->SetScript(script);
    //cout << __LINE__ << " Dirty" << endl;
    this->Dirty = 1;
    }
  if ( this->Dirty )
    {
    this->SetTypeToFloat();
    this->AddTraceEntry("$kw(%s) SetLabelAndScript {%s} {%s}", this->GetTclName(), label, script);
    }
  this->Parent->UpdateNewScript();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationInterfaceEntry::Update()
{
  //cout << "Type is: " << this->TypeIsInt << endl;
  this->Parent->UpdateNewScript();
  this->Parent->ShowEntryInFrame(this);
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
}

