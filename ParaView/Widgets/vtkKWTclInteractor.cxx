/*=========================================================================

  Module:    vtkKWTclInteractor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWTclInteractor.h"

#include "vtkKWApplication.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTclInteractor );
vtkCxxRevisionMacro(vtkKWTclInteractor, "1.18");

int vtkKWTclInteractorCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWTclInteractor::vtkKWTclInteractor()
{
  this->CommandFunction = vtkKWTclInteractorCommand;
  
  this->ButtonFrame = vtkKWWidget::New();
  this->DismissButton = vtkKWPushButton::New();

  this->CommandFrame = vtkKWWidget::New();
  this->CommandLabel = vtkKWLabel::New();
  this->CommandEntry = vtkKWEntry::New();
  
  this->DisplayFrame = vtkKWWidget::New();
  this->DisplayText = vtkKWText::New();
  this->DisplayScrollBar = vtkKWWidget::New();
  
  this->Title = NULL;
  this->SetTitle("VTK Interactor");
  
  this->TagNumber = 1;
  this->CommandIndex = 0;
  this->MasterWindow = 0;
}

//----------------------------------------------------------------------------
vtkKWTclInteractor::~vtkKWTclInteractor()
{
  this->DismissButton->Delete();
  this->DismissButton = NULL;
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  
  this->CommandLabel->Delete();
  this->CommandLabel = NULL;
  this->CommandEntry->Delete();
  this->CommandEntry = NULL;
  this->CommandFrame->Delete();
  this->CommandFrame = NULL;
  
  this->DisplayText->Delete();
  this->DisplayText = NULL;
  this->DisplayScrollBar->Delete();
  this->DisplayScrollBar = NULL;
  this->DisplayFrame->Delete();
  this->DisplayFrame = NULL;
  
  this->SetTitle(NULL);
  this->SetMasterWindow(0);
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::SetMasterWindow(vtkKWWindow* win)
{
  if (this->MasterWindow != win) 
    { 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->UnRegister(this); 
      }
    this->MasterWindow = win; 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->Register(this); 
      if (this->Application)
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    } 
  
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Interactor already created");
    return;
    }
  
  this->SetApplication(app);
  
  // create the top level
  wname = this->GetWidgetName();
  if (this->MasterWindow)
    {
    this->Script("toplevel %s -class %s", 
                 wname,
                 this->MasterWindow->GetClassName());
    }
  else
    {
    this->Script("toplevel %s", wname);
    }
  this->Script("wm title %s \"%s\"", wname, this->Title);
  this->Script("wm iconname %s \"vtk\"", wname);
  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }
  
  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());
  
  char buttonCommand[100];
  sprintf(buttonCommand, "-command \"wm withdraw %s\"", wname);
  this->DismissButton->SetParent(this->ButtonFrame);
  this->DismissButton->Create(app, buttonCommand);
  this->DismissButton->SetLabel("Dismiss");
  this->Script("pack %s -side left -expand 1 -fill x",
               this->DismissButton->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW {wm withdraw %s}",
               wname, wname);
  
  this->CommandFrame->SetParent(this);
  this->CommandFrame->Create(app, "frame", "");
  
  this->CommandLabel->SetParent(this->CommandFrame);
  this->CommandLabel->Create(app, "");
  this->CommandLabel->SetLabel("Command:");
  
  this->CommandEntry->SetParent(this->CommandFrame);
  this->CommandEntry->Create(app, "-width 40 -highlightthickness 1");
  this->Script("bind %s <Return> {%s Evaluate}",
               this->CommandEntry->GetWidgetName(), this->GetTclName());
  
  this->Script("pack %s -side left", this->CommandLabel->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill x",
               this->CommandEntry->GetWidgetName());
  
  char scrollBarCommand[100];
  this->DisplayFrame->SetParent(this);
  this->DisplayFrame->Create(app, "frame", "");
  this->DisplayText->SetParent(this->DisplayFrame);
  this->DisplayText->Create(app, "-setgrid true -width 60 -height 8 -wrap word -state disabled");
  this->DisplayScrollBar->SetParent(this->DisplayFrame);
  sprintf(scrollBarCommand, "-command \"%s yview\"",
          this->DisplayText->GetWidgetName());
  this->DisplayScrollBar->Create(app, "scrollbar", scrollBarCommand);
  this->Script("%s configure -yscrollcommand \"%s set\"",
               this->DisplayText->GetWidgetName(),
               this->DisplayScrollBar->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill both",
               this->DisplayText->GetWidgetName());
  this->Script("pack %s -side left -expand 0 -fill y",
               this->DisplayScrollBar->GetWidgetName());
  this->Script("pack %s -side bottom -expand 1 -fill both",
               this->DisplayFrame->GetWidgetName());
  this->Script("pack %s -pady 3m -padx 2m -side bottom -fill x",
               this->CommandFrame->GetWidgetName());

  this->Script("set commandList \"\"");

  this->Script("bind %s <Down> {%s DownCallback}",
               this->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Up> {%s UpCallback}",
               this->GetWidgetName(), this->GetTclName());
  
  this->Script("wm withdraw %s", wname);
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::Display()
{
  this->Script("wm deiconify %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::Evaluate()
{
  this->CommandIndex = this->TagNumber;
  this->TagNumber++;
  
  this->Script("%s configure -state normal",
               this->DisplayText->GetWidgetName());
  this->Script("%s insert end [concat {%s}] %d",
               this->DisplayText->GetWidgetName(),
               this->CommandEntry->GetValue(), this->CommandIndex);
  this->Script("set commandList [linsert $commandList end [concat {%s}]]",
               this->CommandEntry->GetValue());
  this->Script("%s insert end \"\n\"", this->DisplayText->GetWidgetName());
  this->Register(this);
  this->Script("catch {eval [list %s]} _tmp_err",  
               this->CommandEntry->GetValue());
  if ( this->Application->GetApplicationExited() )
    {
    this->UnRegister(this);
    return;
    }
  this->UnRegister(this);
  this->Script("set _tmp_err");
  this->Script("%s insert end {%s}", 
               this->DisplayText->GetWidgetName(),
               this->Application->GetMainInterp()->result);
  this->Script("%s insert end \"\n\n\"", this->DisplayText->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->DisplayText->GetWidgetName());
  this->Script("%s yview end", this->DisplayText->GetWidgetName());
  
  this->CommandEntry->SetValue("");
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::AppendText(const char* text)
{
  this->Script("%s insert end {%s}", 
               this->DisplayText->GetWidgetName(),
               text);
  this->Script("%s insert end \\n", 
               this->DisplayText->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::DownCallback()
{
  if ( ! this->Application)
    {
    return;
    }
  
  if (this->CommandIndex < this->TagNumber-1)
    {
    this->CommandIndex++;
    this->Script("set commandString [lindex $commandList %d]",
                 this->CommandIndex);
    this->Script("%s delete 0 end", this->CommandEntry->GetWidgetName());
    this->Script("%s insert end $commandString",
                 this->CommandEntry->GetWidgetName());
    }
  else
    {
    this->CommandEntry->SetValue("");
    }
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::UpCallback()
{
  if ( ! this->Application)
    {
    return;
    }
  
  if (this->CommandIndex > 0)
    {
    this->CommandIndex--;
    this->Script("set commandString [lindex $commandList %d]",
                 this->CommandIndex);
    this->Script("%s delete 0 end", this->CommandEntry->GetWidgetName());
    this->Script("%s insert end $commandString",
                 this->CommandEntry->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->ButtonFrame)
    {
    this->ButtonFrame->SetEnabled(this->Enabled);
    }

  if (this->DismissButton)
    {
    this->DismissButton->SetEnabled(this->Enabled);
    }

  if (this->CommandFrame)
    {
    this->CommandFrame->SetEnabled(this->Enabled);
    }

  if (this->CommandLabel)
    {
    this->CommandLabel->SetEnabled(this->Enabled);
    }

  if (this->CommandEntry)
    {
    this->CommandEntry->SetEnabled(this->Enabled);
    }

  if (this->DisplayFrame)
    {
    this->DisplayFrame->SetEnabled(this->Enabled);
    }

  if (this->DisplayText)
    {
    this->DisplayText->SetEnabled(this->Enabled);
    }

  if (this->DisplayScrollBar)
    {
    this->DisplayScrollBar->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Title: " << this->GetTitle() << endl;
}

