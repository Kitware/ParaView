/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTimerLogDisplay.cxx
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
#include "vtkPVTimerLogDisplay.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTimerLogDisplay );

int vtkPVTimerLogDisplayCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVTimerLogDisplay::vtkPVTimerLogDisplay()
{
  this->CommandFunction = vtkPVTimerLogDisplayCommand;
  
  this->ButtonFrame = vtkKWWidget::New();
  this->DismissButton = vtkKWPushButton::New();
  
  this->DisplayFrame = vtkKWWidget::New();
  this->DisplayText = vtkKWText::New();
  this->DisplayScrollBar = vtkKWWidget::New();

  this->ControlFrame = vtkKWWidget::New();
  this->SaveButton = vtkKWPushButton::New();
  this->ClearButton = vtkKWPushButton::New();
  this->ThresholdLabel = vtkKWLabel::New();
  this->ThresholdMenu = vtkKWOptionMenu::New();
  this->BufferLengthLabel = vtkKWLabel::New();
  this->BufferLengthMenu = vtkKWOptionMenu::New();
  this->EnableLabel = vtkKWLabel::New();
  this->EnableCheck = vtkKWCheckButton::New();

  this->Title = NULL;
  this->SetTitle("Log");
  
  //this->TagNumber = 1;
  //this->CommandIndex = 0;
  this->MasterWindow = 0;
  this->Threshold = 0.01;
  this->Writable = 0;
}

//----------------------------------------------------------------------------
vtkPVTimerLogDisplay::~vtkPVTimerLogDisplay()
{
  this->DismissButton->Delete();
  this->DismissButton = NULL;
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  
  this->DisplayText->Delete();
  this->DisplayText = NULL;
  this->DisplayScrollBar->Delete();
  this->DisplayScrollBar = NULL;
  this->DisplayFrame->Delete();
  this->DisplayFrame = NULL;

  this->ControlFrame->Delete();
  this->ControlFrame = NULL;
  this->SaveButton->Delete();
  this->SaveButton = NULL;
  this->ClearButton->Delete();
  this->ClearButton = NULL;
  this->ThresholdLabel->Delete();
  this->ThresholdLabel = NULL;
  this->ThresholdMenu->Delete();
  this->ThresholdMenu = NULL;
  this->BufferLengthLabel->Delete();
  this->BufferLengthLabel = NULL;
  this->BufferLengthMenu->Delete();
  this->BufferLengthMenu = NULL;
  this->EnableLabel->Delete();
  this->EnableLabel = NULL;
  this->EnableCheck->Delete();
  this->EnableCheck = NULL;
  
  this->SetTitle(NULL);
  this->SetMasterWindow(0);
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::SetMasterWindow(vtkKWWindow* win)
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
void vtkPVTimerLogDisplay::Create(vtkKWApplication *app)
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
  this->DismissButton->SetParent(this->ButtonFrame);
  this->DismissButton->Create(app, "");
  this->DismissButton->SetCommand(this, "Dismiss");
  this->DismissButton->SetLabel("Dismiss");
  this->Script("pack %s -side left -expand 1 -fill x",
               this->DismissButton->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW {wm withdraw %s}",
               wname, wname);
    
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



  this->ControlFrame->SetParent(this);
  this->ControlFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill x -expand 0 -pady 2m",
               this->ControlFrame->GetWidgetName());
  this->SaveButton->SetParent(this->ControlFrame);
  this->SaveButton->Create(app, "");
  this->SaveButton->SetCommand(this, "Save");
  this->SaveButton->SetLabel("Save");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->SaveButton->GetWidgetName());
  this->ClearButton->SetParent(this->ControlFrame);
  this->ClearButton->Create(app, "");
  this->ClearButton->SetCommand(this, "Clear");
  this->ClearButton->SetLabel("Clear");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->ClearButton->GetWidgetName());

  this->ThresholdLabel->SetParent(this->ControlFrame);
  this->ThresholdLabel->Create(app, "");
  this->ThresholdLabel->SetLabel("Time Threshold:");
  this->ThresholdLabel->SetBalloonHelpString("This option filters out short duration events.");
  this->ThresholdMenu->SetParent(this->ControlFrame);
  this->ThresholdMenu->Create(app, "");
  this->ThresholdMenu->AddEntryWithCommand("0.001", this, "SetThreshold 0.001");
  this->ThresholdMenu->AddEntryWithCommand("0.01", this, "SetThreshold 0.01");
  this->ThresholdMenu->AddEntryWithCommand("0.1", this, "SetThreshold 0.1");
  this->ThresholdMenu->SetCurrentEntry("0.01");
  this->ThresholdMenu->SetBalloonHelpString("This option filters out short duration events.");
  this->Script("pack %s %s -side left",
               this->ThresholdLabel->GetWidgetName(),
               this->ThresholdMenu->GetWidgetName());

  this->BufferLengthLabel->SetParent(this->ControlFrame);
  this->BufferLengthLabel->Create(app, "");
  this->BufferLengthLabel->SetLabel("Buffer Length:");
  this->BufferLengthLabel->SetBalloonHelpString("Set how many entries the log can have.");
  this->BufferLengthMenu->SetParent(this->ControlFrame);
  this->BufferLengthMenu->Create(app, "");
  this->BufferLengthMenu->AddEntryWithCommand("100", this, "SetBufferLength 100");
  this->BufferLengthMenu->AddEntryWithCommand("500", this, "SetBufferLength 500");
  this->BufferLengthMenu->AddEntryWithCommand("1000", this, "SetBufferLength 1000");
  this->BufferLengthMenu->SetCurrentEntry("500");
  this->BufferLengthMenu->SetBalloonHelpString("Set how many entries the log can have.");
  this->Script("pack %s %s -side left",
               this->BufferLengthLabel->GetWidgetName(),
               this->BufferLengthMenu->GetWidgetName());

  this->EnableLabel->SetParent(this->ControlFrame);
  this->EnableLabel->Create(app, "");
  this->EnableLabel->SetLabel("Enable:");
  this->EnableLabel->SetBalloonHelpString("Enable or disable loging of new events.");
  this->EnableCheck->SetParent(this->ControlFrame);
  this->EnableCheck->Create(app, "");
  this->EnableCheck->SetState(1);
  this->EnableCheck->SetCommand(this, "EnableCheckCallback");
  this->EnableCheck->SetText("");
  this->EnableCheck->SetBalloonHelpString("Enable or disable loging of new events.");
  this->Script("pack %s %s -side left -expand 0 -fill none",
               this->EnableLabel->GetWidgetName(),
               this->EnableCheck->GetWidgetName());



  this->Script("set commandList \"\"");
  
  this->Script("wm withdraw %s", wname);
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Display()
{   
  this->Script("wm deiconify %s", this->GetWidgetName());
  this->Script("grab %s", this->GetWidgetName());

  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::SetThreshold(float val)
{
  if (val == this->Threshold)
    {
    return;
    }
  this->Modified();
  this->Threshold = val;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::SetBufferLength(int length)
{
  if (length == vtkTimerLog::GetMaxEntries())
    {
    return;
    }
  this->Modified();
  vtkTimerLog::SetMaxEntries(length);
  this->Update();
}


//----------------------------------------------------------------------------
int vtkPVTimerLogDisplay::GetBufferLength()
{
  return vtkTimerLog::GetMaxEntries();
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Clear()
{
  vtkTimerLog::ResetLog();
  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::EnableCheckCallback()
{
  if (this->EnableCheck->GetState())
    {
    vtkTimerLog::LoggingOn();
    }
  else
    {
    vtkTimerLog::LoggingOff();
    }
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Save()
{
  char *filename;

  this->Script("tk_getSaveFile -filetypes {{{Text} {.txt}}} -defaultextension .txt -initialfile ParaViewLog.txt");
  filename = new char[strlen(this->Application->GetMainInterp()->result)+1];
  sprintf(filename, "%s", this->Application->GetMainInterp()->result);
  
  if (strcmp(filename, "") == 0)
    {
    delete [] filename;
    return;
    }
  this->Save(filename);
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Save(const char *fileName)
{
  ofstream *fptr;
 
  fptr = new ofstream(fileName);

  if (fptr->fail())
    {
    vtkErrorMacro(<< "Could not open" << fileName);
    delete fptr;
    return;
    }
  vtkTimerLog::DumpLogWithIndents(fptr, this->Threshold);
  fptr->close();
  delete fptr;
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::EnableWrite()
{
  this->Script("%s configure -state normal", 
               this->DisplayText->GetWidgetName());
  this->Writable = 1;
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::DisableWrite()
{
  this->Writable = 0;
  this->Script("%s yview end", this->DisplayText->GetWidgetName());
  // Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("update");                   
  this->Script("%s configure -state disabled",
               this->DisplayText->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Append(const char* msg)
{
  int w = this->Writable;
  if ( !w )
    {
    this->EnableWrite();
    }

  this->Script("%s insert end {%s%c}",
	       this->DisplayText->GetWidgetName(), msg, '\n');
  if ( !w )
    {
    this->DisableWrite();
    }
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Update()
{
  ostrstream *fptr;
  int length;
  char *str;
   
  this->EnableWrite();
  this->DisplayText->SetValue("");

  length = vtkTimerLog::GetNumberOfEvents() * 40;
  if (length > 0)
    {
    str = new char [length];
    fptr = new ostrstream(str, length);

    if (fptr->fail())
      {
      vtkErrorMacro(<< "Unable to string stream");
      }
    else
      {
      //*fptr << "Hello world !!!\n ()";
      vtkTimerLog::DumpLogWithIndents(fptr, this->Threshold);

      length = fptr->pcount();
      str[length] = '\0';

      delete fptr;
      
      //this->DisplayText->SetValue(str);
      // Put the strings in one by one to avoid a Tk bug.
      // Strings seemed to be put in recursively, so we run out of stack.
      char *start, *end;
      int count;
      length = vtkString::Length(str);
      count = 0;
      start = end = str;
      while (count < length)
        {
        // Find the next line break.
        while (*end != '\n' && count < length)
          {
          ++end;
          ++count;
          }
        // Set it to an end of string.
        *end = '\0';
        // Add the string.
	this->Append(start);
        // Initialize the search for the next string.
        start = end+1;
        end = start;
        ++count;
        }

      delete [] str;
      }
    }  
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Dismiss()
{
  this->Script("grab release %s", this->GetWidgetName());
  this->Script("wm withdraw %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "Threshold: " << this->Threshold << endl;
}
