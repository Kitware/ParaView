/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWTclInteractor.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWTclInteractor.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWTclInteractor );

int vtkKWTclInteractorCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

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

void vtkKWTclInteractor::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Interactor already created");
    return;
    }
  
  this->SetApplication(app);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("toplevel %s", wname);
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

void vtkKWTclInteractor::Display()
{
  this->Script("wm deiconify %s", this->GetWidgetName());
}

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
  this->Script("%s insert end [eval [list %s]]",
               this->DisplayText->GetWidgetName(),
               this->CommandEntry->GetValue());
  this->Script("%s insert end \"\n\n\"", this->DisplayText->GetWidgetName());
  this->Script("%s configure -state disabled",
               this->DisplayText->GetWidgetName());
  this->Script("%s yview end", this->DisplayText->GetWidgetName());
  
  this->CommandEntry->SetValue("");
}

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
