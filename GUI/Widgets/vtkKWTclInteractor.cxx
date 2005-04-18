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
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

#include <kwsys/stl/string>

//-------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTclInteractor );
vtkCxxRevisionMacro(vtkKWTclInteractor, "1.30");

int vtkKWTclInteractorCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWTclInteractor::vtkKWTclInteractor()
{
  this->CommandFunction = vtkKWTclInteractorCommand;
  
  this->ButtonFrame = vtkKWFrame::New();
  this->DismissButton = vtkKWPushButton::New();

  this->CommandFrame = vtkKWFrame::New();
  this->CommandLabel = vtkKWLabel::New();
  this->CommandEntry = vtkKWEntry::New();
  
  this->DisplayText = vtkKWText::New();
  
  this->SetTitle("Tcl Interactor");
  
  this->TagNumber = 1;
  this->CommandIndex = 0;
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
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app, args);

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "");
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());
  
  this->DismissButton->SetParent(this->ButtonFrame);
  this->DismissButton->Create(app, NULL);
  this->DismissButton->SetCommand(this, "Withdraw");
  this->DismissButton->SetText("Dismiss");
  this->Script("pack %s -side left -expand 1 -fill x",
               this->DismissButton->GetWidgetName());

  this->CommandFrame->SetParent(this);
  this->CommandFrame->Create(app, "");
  
  this->CommandLabel->SetParent(this->CommandFrame);
  this->CommandLabel->Create(app, "");
  this->CommandLabel->SetText("Command:");
  
  this->CommandEntry->SetParent(this->CommandFrame);
  this->CommandEntry->Create(app, "-width 40 -highlightthickness 1");
  this->Script("bind %s <Return> {%s Evaluate}",
               this->CommandEntry->GetWidgetName(), this->GetTclName());
  
  this->Script("pack %s -side left", this->CommandLabel->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill x",
               this->CommandEntry->GetWidgetName());
  
  this->DisplayText->SetParent(this);
  this->DisplayText->Create(app, "-setgrid true");
  this->DisplayText->SetWidth(100);
  this->DisplayText->SetHeight(20);
  this->DisplayText->SetWrapToWord();
  this->DisplayText->EditableTextOff();
  this->DisplayText->UseVerticalScrollbarOn();

  this->Script("pack %s -side bottom -expand 1 -fill both",
               this->DisplayText->GetWidgetName());

  this->Script("pack %s -pady 3m -padx 2m -side bottom -fill x",
               this->CommandFrame->GetWidgetName());

  this->Script("set commandList \"\"");

  this->Script("bind %s <Down> {%s DownCallback}",
               this->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Up> {%s UpCallback}",
               this->GetWidgetName(), this->GetTclName());
  
  this->Withdraw();
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::Display()
{
  if (this->MasterWindow)
    {
    int width, height, x, y;
    const char *res = 
      this->Script("wm geometry %s", this->MasterWindow->GetWidgetName());
    sscanf(res, "%dx%d+%d+%d", &width, &height, &x, &y);
    x += width / 3;
    y += height / 3;
    this->SetPosition(x, y);
    }

  this->Superclass::Display();
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::Evaluate()
{
  this->CommandIndex = this->TagNumber;
  this->TagNumber++;

  char buffer_tag[32];
  sprintf(buffer_tag, "%d", this->CommandIndex);

  this->DisplayText->AppendValue(this->CommandEntry->GetValue(),
                                 buffer_tag);
  this->DisplayText->AppendValue("\n");

  this->Script("set commandList [linsert $commandList end [concat {%s}]]",
               this->CommandEntry->GetValue());

  this->Register(this);

  this->Script("catch {eval [list %s]} _tmp_err",  
               this->CommandEntry->GetValue());

  if (this->GetApplication()->GetInExit())
    {
    this->UnRegister(this);
    return;
    }
  this->UnRegister(this);

  kwsys_stl::string res(this->Script("set _tmp_err"));
  this->DisplayText->AppendValue(res.c_str());
  this->DisplayText->AppendValue("\n\n");

  this->Script("%s yview end", 
               this->DisplayText->GetTextWidget()->GetWidgetName());
  
  this->CommandEntry->SetValue("");
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::AppendText(const char* text)
{
  this->DisplayText->AppendValue(text);
  this->DisplayText->AppendValue("\n");
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::DownCallback()
{
  if ( ! this->IsCreated())
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
  if ( ! this->IsCreated())
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

  if (this->DisplayText)
    {
    this->DisplayText->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWTclInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

