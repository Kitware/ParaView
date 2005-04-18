/*=========================================================================

  Module:    vtkPVSourceNotebook.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSourceNotebook.h"
#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkKWNotebook.h"
#include "vtkPVApplicationSettingsInterface.h"
#include "vtkPVWindow.h"
#include "vtkPVInformationGUI.h"
#include "vtkPVDisplayGUI.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWPushButtonWithMenu.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWMenu.h"
#include "vtkKWLabel.h"
#include "vtkKWTkUtilities.h"

#define VTK_PV_AUTO_ACCEPT_REG_KEY "AutoAccept"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSourceNotebook);
vtkCxxRevisionMacro(vtkPVSourceNotebook, "1.6.2.3");

//----------------------------------------------------------------------------
int vtkPVSourceNotebookCommand(ClientData cd, Tcl_Interp *interp,
                         int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSourceNotebook::vtkPVSourceNotebook()
{
  this->PVSource = 0;

  this->Notebook = vtkKWNotebook::New();
  this->Notebook->AlwaysShowTabsOn();

  this->DisplayGUI = vtkPVDisplayGUI::New();
  this->InformationGUI = vtkPVInformationGUI::New();

  this->DescriptionFrame = vtkKWWidget::New();
  this->NameLabel = vtkKWLabeledLabel::New();
  this->TypeLabel = vtkKWLabeledLabel::New();
  this->LongHelpLabel = vtkKWLabeledLabel::New();
  this->LabelEntry = vtkKWLabeledEntry::New();

  this->ButtonFrame = vtkKWWidget::New();
  this->MainParameterFrame = vtkKWWidget::New();
  this->AcceptButton = vtkKWPushButtonWithMenu::New();
  this->AcceptPullDownArrow = vtkKWPushButton::New();
  this->ResetButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();
      
  this->AcceptButtonRed = 0;
  this->AutoAccept = 0;
  this->TimerToken = 0;
  this->CloneInitializeLock = 0;
}

//----------------------------------------------------------------------------
vtkPVSourceNotebook::~vtkPVSourceNotebook()
{
  this->SetPVSource(0);

  this->Notebook->Delete();
  this->Notebook = 0;

  this->DisplayGUI->SetParent(0);
  this->DisplayGUI->Delete();
  this->DisplayGUI = 0;

  this->InformationGUI->SetParent(0);
  this->InformationGUI->Delete();
  this->InformationGUI = 0;

  this->DescriptionFrame->Delete();
  this->DescriptionFrame = NULL;

  this->NameLabel->Delete();
  this->NameLabel = NULL;

  this->TypeLabel->Delete();
  this->TypeLabel = NULL;

  this->LongHelpLabel->Delete();
  this->LongHelpLabel = NULL;

  this->LabelEntry->Delete();
  this->LabelEntry = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;  
  
  this->AcceptPullDownArrow->Delete();
  this->AcceptPullDownArrow = NULL;  
  
  this->ResetButton->Delete();
  this->ResetButton = NULL;  
  
  this->DeleteButton->Delete();
  this->DeleteButton = NULL;

  this->MainParameterFrame->Delete();
  this->MainParameterFrame = NULL;

  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
}

//----------------------------------------------------------------------------
// I am not using a macro, because I expect we will have to propagate
// this source when we move the display and information GUIs into
// this object.
void vtkPVSourceNotebook::SetPVSource(vtkPVSource* pvs)
{
  // Do not register here because it causes a memory leak (circular reference).
  this->PVSource = pvs;
    
  if (this->DisplayGUI)
    {
    this->DisplayGUI->SetPVSource(pvs);
    }
  if (this->DisplayGUI)
    {
    this->DisplayGUI->SetPVSource(pvs);
    }
}
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::Close()
{
  this->DisplayGUI->Close();
}
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::Update()
{
  if (this->PVSource == 0)  
    {
    return;
    }
  this->UpdateEnableStateWithSource(this->PVSource);
  this->UpdateDescriptionFrame(this->PVSource);
  this->DisplayGUI->Update();
  this->InformationGUI->Update(this->PVSource);
}
  

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::UpdateEnableStateWithSource(vtkPVSource* pvs)
{
  this->UpdateEnableState();
  
  if ( pvs->IsDeletable() )
    {
    this->PropagateEnableState(this->DeleteButton);
    }
  else
    {
    this->DeleteButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::UpdateEnableState()
{
  this->PropagateEnableState(this->Notebook);
  this->PropagateEnableState(this->DisplayGUI);
  this->PropagateEnableState(this->InformationGUI);
  this->PropagateEnableState(this->DescriptionFrame);
  this->PropagateEnableState(this->NameLabel);
  this->PropagateEnableState(this->TypeLabel);
  this->PropagateEnableState(this->LabelEntry);
  this->PropagateEnableState(this->LongHelpLabel);
  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->AcceptButton);
  this->PropagateEnableState(this->AcceptPullDownArrow);
  this->PropagateEnableState(this->ResetButton);
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::Raise(const char* pageName)
{
  this->Notebook->Raise(pageName);
}  
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::HidePage(const char* pageName)
{
  this->Notebook->HidePage(pageName);
}
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::ShowPage(const char* pageName)
{
  this->Notebook->ShowPage(pageName);
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::Create(vtkKWApplication* app, const char* args)
{
  if (!this->Superclass::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
    
  this->Notebook->SetParent(this);
  this->Notebook->Create(app, args);
  this->Notebook->AddPage("Parameters");
  this->Notebook->AddPage("Display");
  this->Notebook->AddPage("Information");
  this->Script("pack %s -fill both -expand t", 
               this->Notebook->GetWidgetName());

  // Create the easiest (modular) pages first
  // Create the display GUI.
  this->DisplayGUI->SetParent(this->Notebook->GetFrame("Display"));
  this->DisplayGUI->ScrollableOn();
  this->DisplayGUI->Create(app, 0);
  this->Script("pack %s -fill both -expand yes -side top",
                this->DisplayGUI->GetWidgetName());
  // Create the information page.
  this->InformationGUI->SetParent(
        this->Notebook->GetFrame("Information"));
  this->InformationGUI->ScrollableOn();
  this->InformationGUI->Create(app, 0);
  this->Script("pack %s -fill both -expand yes -side top",
               this->InformationGUI->GetWidgetName());


  // Now create the parameters page item by item.
  // one frame is left blank for the source to pack when selected.
  this->DescriptionFrame->SetParent(this->Notebook->GetFrame("Parameters"));
  this->DescriptionFrame->Create(this->GetApplication(), "frame", "");
  this->Script("pack %s -fill both -expand t -side top -padx 2 -pady 2", 
               this->DescriptionFrame->GetWidgetName());

  const char *label1_opt = "-width 12 -anchor e";

  this->NameLabel->SetParent(this->DescriptionFrame);
  this->NameLabel->Create(this->GetApplication());
  this->NameLabel->SetLabel("Name:");
  this->Script("%s configure -anchor w", 
               this->NameLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->NameLabel->GetLabel()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->NameLabel->GetLabel2()->GetWidgetName());
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->GetApplication()->GetMainInterp(),
    this->NameLabel->GetLabel2()->GetWidgetName());

  this->TypeLabel->SetParent(this->DescriptionFrame);
  this->TypeLabel->Create(this->GetApplication());
  this->TypeLabel->GetLabel()->SetLabel("Class:");
  this->Script("%s configure -anchor w", 
               this->TypeLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->TypeLabel->GetLabel()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->TypeLabel->GetLabel2()->GetWidgetName());

  this->LabelEntry->SetParent(this->DescriptionFrame);
  this->LabelEntry->Create(this->GetApplication());
  this->LabelEntry->GetLabel()->SetLabel("Label:");
  this->Script("%s config %s", 
               this->LabelEntry->GetLabel()->GetWidgetName(),label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->LabelEntry->GetEntry()->GetWidgetName());
  this->Script("bind %s <KeyPress-Return> {%s LabelEntryCallback}",
               this->LabelEntry->GetEntry()->GetWidgetName(), 
               this->GetTclName());

  this->LongHelpLabel->SetParent(this->DescriptionFrame);
  this->LongHelpLabel->Create(this->GetApplication());
  this->LongHelpLabel->GetLabel()->SetLabel("Description:");
  this->LongHelpLabel->GetLabel2()->AdjustWrapLengthToWidthOn();
  this->Script("%s configure -anchor w", 
               this->LongHelpLabel->GetLabel2()->GetWidgetName());
  this->Script("%s config %s", 
               this->LongHelpLabel->GetLabel()->GetWidgetName(), label1_opt);
  this->Script("pack %s -fill x -expand t", 
               this->LongHelpLabel->GetLabel2()->GetWidgetName());

  this->Script("grid %s -sticky news", 
               this->NameLabel->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->TypeLabel->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->LabelEntry->GetWidgetName());
  this->Script("grid %s -sticky news", 
               this->LongHelpLabel->GetWidgetName());
  this->Script("grid columnconfigure %s 0 -weight 1", 
               this->LongHelpLabel->GetParent()->GetWidgetName());
               
  this->ButtonFrame->SetParent(this->Notebook->GetFrame("Parameters"));
  this->ButtonFrame->Create(this->GetApplication(), "frame", "");
  this->Script("pack %s -fill both -expand t -side top", 
               this->ButtonFrame->GetWidgetName());

  // Why do the buttons need two nested frames?
  vtkKWWidget *frame = vtkKWWidget::New();
  frame->SetParent(this->ButtonFrame);
  frame->Create(this->GetApplication(), "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());  
  
  this->AcceptButton->SetParent(frame);
  this->AcceptButton->Create(this->GetApplication(), "");
  if (this->AutoAccept)
    {
    this->AcceptButton->SetLabel("Auto Accept");
    this->Script("%s config -relief flat", this->AcceptButton->GetWidgetName());
    }
  else
    {
    this->AcceptButton->SetLabel("Accept");
    this->Script("%s config -relief raised", this->AcceptButton->GetWidgetName());
    }    
  this->AcceptButton->SetCommand(this, "AcceptButtonCallback");
  this->AcceptButton->SetBalloonHelpString(
    "Cause the current values in the user interface to take effect "
    "(key shortcut: Ctrl+Enter)");

  this->AcceptPullDownArrow->SetParent(this->AcceptButton);
  this->AcceptPullDownArrow->Create(this->GetApplication(), 
                                    "-image PVPullDownArrow");
  this->Script("place %s -relx 0 -rely 1 -x -5 -y 5 -anchor se", 
                this->AcceptPullDownArrow->GetWidgetName());

  if (app->GetRegisteryValue(2,"RunTime", 
          VTK_PV_AUTO_ACCEPT_REG_KEY,0))
    {
    this->SetAutoAccept(app->GetIntRegisteryValue(2,"RunTime",
                                  VTK_PV_AUTO_ACCEPT_REG_KEY));
    }

  vtkKWMenu* menu = this->AcceptButton->GetMenu();
  char* var = menu->CreateRadioButtonVariable(this, "Auto");
  menu->AddRadioButton(0, "Manual", var, 
      this, "SetAutoAccept 0",
      "You have to press accept after changes to a modules parameters.");
  menu->AddRadioButton(1, "Auto", var, 
      this, "SetAutoAccept 1",
      "Accept is automatically called every time a module is modified.");
  //menu->AddRadioButton(2, "Interactive", var, 
  //    this, "SetAutoAccept 2",
  //    "Accept is automatically called every time a module is modified.");
  this->Script("set %s %d", var, this->AutoAccept);
  delete [] var;

  this->ResetButton->SetParent(frame);
  this->ResetButton->Create(this->GetApplication(), "-text Reset");
  this->ResetButton->SetCommand(this, "ResetButtonCallback");
  this->ResetButton->SetBalloonHelpString(
    "Revert to the previous parameters of the module.");

  this->DeleteButton->SetParent(frame);
  this->DeleteButton->Create(this->GetApplication(), "-text Delete");
  this->DeleteButton->SetCommand(this, "DeleteButtonCallback");
  this->DeleteButton->SetBalloonHelpString(
    "Remove the current module.  "
    "This can only be done if no other modules depends on the current one.");

  this->Script("pack %s %s %s -padx 2 -pady 2 -side left -fill x -expand t",
               this->AcceptButton->GetWidgetName(), 
               this->ResetButton->GetWidgetName(), 
               this->DeleteButton->GetWidgetName());
  this->Script("bind %s <Enter> {+focus %s}",
               this->AcceptButton->GetWidgetName(),
               this->AcceptButton->GetWidgetName());

  frame->Delete();  
 
  // This is left blank for the source to pack when selected.
  this->MainParameterFrame->SetParent(this->Notebook->GetFrame("Parameters"));
  this->MainParameterFrame->Create(this->GetApplication(), "frame", "");
  this->Script("pack %s -fill both -expand t -side top", 
               this->MainParameterFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::SetAutoAccept(int val)
{
  if (this->AutoAccept == val)
    {
    return;
    }
  this->AutoAccept = val;

 this->GetApplication()->SetRegisteryValue(
   2, "RunTime", VTK_PV_AUTO_ACCEPT_REG_KEY, "%d", val);

  // Synchronize the two auto accept guis.
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVApplicationSettingsInterface* appInt = 
      vtkPVApplicationSettingsInterface::SafeDownCast(
        pvApp->GetMainWindow()->GetApplicationSettingsInterface());
  appInt->SetAutoAccept(this->AutoAccept);

  if (val)
    {
    this->AcceptButton->SetLabel("Auto Accept");
    // Just in case the source is already modified.
    this->AcceptButtonCallback();
    }
  else
    {
    this->AcceptButton->SetLabel("Accept");
    }
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::AcceptButtonCallback()
{
  if (this->PVSource)
    {
    this->PVSource->PreAcceptCallback();
    }
}
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::ResetButtonCallback()
{
  if (this->PVSource)
    {
    this->PVSource->ResetCallback();
    }
}
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::DeleteButtonCallback()
{
  if (this->PVSource)
    {
    this->PVSource->DeleteCallback();
    }

  // In case this delete came from a half formed source 
  // (accept not called yet).
  this->ShowPage("Display");
  this->ShowPage("Information");
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::SetAcceptButtonColorToModified()
{
  if (this->AcceptButtonRed)
    {
    return;
    }
  if( !this->CloneInitializeLock ) 
    {
    this->AcceptButtonRed = 1;
    }
  if ( this->PVSource 
   && !this->PVSource->GetOverideAutoAccept())
    {
    if (this->AutoAccept == 1)
      {
      this->EventuallyAccept();
      return;
      }
    if (this->AutoAccept == 2)
      {
      this->AcceptButtonCallback();
      return;
      }
    }

  if ( this->GetPVApplication()->GetMainWindow()->GetInDemo() )
    {
    return;
    }
  
  this->Script("%s configure -background #17b27e",
               this->AcceptButton->GetWidgetName());
  this->Script("%s configure -activebackground #17b27e",
               this->AcceptButton->GetWidgetName());
}
//----------------------------------------------------------------------------
void vtkPVSourceNotebook::SetAcceptButtonColorToUnmodified()
{
  if (!this->AcceptButtonRed)
    {
    return;
    }
  this->AcceptButtonRed = 0;

#ifdef _WIN32
  this->Script("%s configure -background [lindex [%s configure -background] 3]",
               this->AcceptButton->GetWidgetName(),
               this->AcceptButton->GetWidgetName());
  this->Script("%s configure -activebackground "
               "[lindex [%s configure -activebackground] 3]",
               this->AcceptButton->GetWidgetName(),
               this->AcceptButton->GetWidgetName());
#else
  this->Script("%s configure -background #ccc",
               this->AcceptButton->GetWidgetName());
  this->Script("%s configure -activebackground #eee",
               this->AcceptButton->GetWidgetName());
#endif
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::LabelEntryCallback()
{
  if (this->PVSource)
    {
    this->PVSource->SetLabel(this->LabelEntry->GetEntry()->GetValue());
    }
  else
    {
    vtkErrorMacro("Source had not been set.");
    }
} 



//----------------------------------------------------------------------------
void vtkPVSourceNotebook::UpdateDescriptionFrame(vtkPVSource* pvs)
{
  if (!this->GetApplication())
    {
    return;
    }

  if (this->NameLabel && this->NameLabel->IsCreated())
    {
    this->NameLabel->GetLabel2()->SetLabel(pvs->GetName() ? pvs->GetName() : "");
    }

  if (this->TypeLabel && this->TypeLabel->IsCreated())
    {
    if (pvs->GetSourceClassName()) 
      {
      this->TypeLabel->GetLabel2()->SetLabel(
        pvs->GetSourceClassName());
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid %s", this->TypeLabel->GetWidgetName());
        }
      }
    else
      {
      this->TypeLabel->GetLabel2()->SetLabel("");
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid remove %s", this->TypeLabel->GetWidgetName());
        }
      }
    }

  if (this->LabelEntry && this->LabelEntry->IsCreated())
    {
    this->LabelEntry->GetEntry()->SetValue(pvs->GetLabel());
    }

  if (this->LongHelpLabel && this->LongHelpLabel->IsCreated())
    {
    if (pvs->GetLongHelp() && 
        !(this->GetPVApplication() && 
          !this->GetPVApplication()->GetShowSourcesLongHelp())) 
      {
      this->LongHelpLabel->GetLabel2()->SetLabel(pvs->GetLongHelp());
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid %s", this->LongHelpLabel->GetWidgetName());
        }
      }
    else
      {
      this->LongHelpLabel->GetLabel2()->SetLabel("");
      if (this->DescriptionFrame->IsPacked())
        {
        this->Script("grid remove %s", this->LongHelpLabel->GetWidgetName());
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSourceNotebook::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
extern "C" { void PVSourceNotebook_IdleAccept(ClientData arg); }
void PVSourceNotebook_IdleAccept(ClientData arg)
{
  vtkPVSourceNotebook *me = (vtkPVSourceNotebook *)arg;
  me->EventuallyAcceptCallBack();
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::EventuallyAccept()
{
  vtkDebugMacro("Enqueue EventuallyAccept request");
  if ( !this->TimerToken )
    {
    this->TimerToken = Tcl_CreateTimerHandler(990, 
                                              PVSourceNotebook_IdleAccept, 
                                              (ClientData)this);
    }
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::EventuallyAcceptCallBack()
{  
  this->TimerToken = NULL;
  this->AcceptButtonCallback();
}

//----------------------------------------------------------------------------
void vtkPVSourceNotebook::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoAccept: " << this->AutoAccept << endl;
  os << indent << "AcceptButtonRed: " << this->AcceptButtonRed << endl;
  os << indent << "DisplayGUI: " << this->DisplayGUI << endl;
  os << indent << "MainParameterFrame: " << this->MainParameterFrame << endl;
  os << indent << "PVSource: " << this->PVSource << endl;
  os << indent << "CloneInitializeLock: " << this->CloneInitializeLock << endl;
}

