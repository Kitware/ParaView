/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerLogDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTimerLogDisplay.h"

#include "vtkPVProcessModule.h"
#include "vtkPVTimerInformation.h"
#include "vtkPVApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkKWScale.h"
#include "vtkClientServerStream.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTimerLogDisplay );
vtkCxxRevisionMacro(vtkPVTimerLogDisplay, "1.26");

int vtkPVTimerLogDisplayCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVTimerLogDisplay::vtkPVTimerLogDisplay()
{
  this->CommandFunction = vtkPVTimerLogDisplayCommand;
  
  this->ButtonFrame = vtkKWWidget::New();
  this->DismissButton = vtkKWPushButton::New();
  
  this->DisplayText = vtkKWText::New();

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

  this->TimerInformation = NULL;
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

  if (this->TimerInformation)
    {
    this->TimerInformation->Delete();
    this->TimerInformation = NULL;
    }
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
      if (this->IsCreated())
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
  // Call the superclass to set the appropriate flags then create manually

  if (!this->vtkKWWidget::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  const char *wname = this->GetWidgetName();
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
  this->Script("wm withdraw %s", wname);
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
  this->DismissButton->SetText("Dismiss");
  this->Script("pack %s -side left -expand 1 -fill x",
               this->DismissButton->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW {%s Dismiss}",
               wname, this->GetTclName());
    
  this->DisplayText->SetParent(this);
  this->DisplayText->Create(app, "-setgrid true");
  this->DisplayText->SetWidth(60);
  this->DisplayText->SetHeight(8);
  this->DisplayText->SetWrapToWord();
  this->DisplayText->EditableTextOff();
  this->DisplayText->UseVerticalScrollbarOn();

  this->Script("pack %s -side bottom -expand 1 -fill both",
               this->DisplayText->GetWidgetName());

  this->ControlFrame->SetParent(this);
  this->ControlFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill x -expand 0 -pady 2m",
               this->ControlFrame->GetWidgetName());
  this->SaveButton->SetParent(this->ControlFrame);
  this->SaveButton->Create(app, "");
  this->SaveButton->SetCommand(this, "Save");
  this->SaveButton->SetText("Save");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->SaveButton->GetWidgetName());
  this->ClearButton->SetParent(this->ControlFrame);
  this->ClearButton->Create(app, "");
  this->ClearButton->SetCommand(this, "Clear");
  this->ClearButton->SetText("Clear");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->ClearButton->GetWidgetName());

  this->ThresholdLabel->SetParent(this->ControlFrame);
  this->ThresholdLabel->Create(app, "");
  this->ThresholdLabel->SetText("Time Threshold:");
  this->ThresholdLabel->SetBalloonHelpString("This option filters out short duration events.");
  this->ThresholdMenu->SetParent(this->ControlFrame);
  this->ThresholdMenu->Create(app, "");
  this->ThresholdMenu->AddEntryWithCommand("0.001", this, "SetThreshold 0.001");
  this->ThresholdMenu->AddEntryWithCommand("0.01", this, "SetThreshold 0.01");
  this->ThresholdMenu->AddEntryWithCommand("0.1", this, "SetThreshold 0.1");
  this->ThresholdMenu->SetCurrentEntry("0.01");
  this->SetThreshold(0.01);

  this->ThresholdMenu->SetBalloonHelpString("This option filters out short duration events.");
  this->Script("pack %s %s -side left",
               this->ThresholdLabel->GetWidgetName(),
               this->ThresholdMenu->GetWidgetName());

  this->BufferLengthLabel->SetParent(this->ControlFrame);
  this->BufferLengthLabel->Create(app, "");
  this->BufferLengthLabel->SetText("Buffer Length:");
  this->BufferLengthLabel->SetBalloonHelpString("Set how many entries the log can have.");
  this->BufferLengthMenu->SetParent(this->ControlFrame);
  this->BufferLengthMenu->Create(app, "");
  this->BufferLengthMenu->AddEntryWithCommand("100", this, "SetBufferLength 100");
  this->BufferLengthMenu->AddEntryWithCommand("500", this, "SetBufferLength 500");
  this->BufferLengthMenu->AddEntryWithCommand("1000", this, "SetBufferLength 1000");
  this->BufferLengthMenu->SetCurrentEntry("500");
  this->SetBufferLength(500);
  this->BufferLengthMenu->SetBalloonHelpString("Set how many entries the log can have.");
  this->Script("pack %s %s -side left",
               this->BufferLengthLabel->GetWidgetName(),
               this->BufferLengthMenu->GetWidgetName());

  this->EnableLabel->SetParent(this->ControlFrame);
  this->EnableLabel->Create(app, "");
  this->EnableLabel->SetText("Enable:");
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
  this->Modified();

  vtkPVApplication* pvApp = this->GetPVApplication();
  if ( pvApp )
    {
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << pm->GetProcessModuleID() << "SetLogThreshold" << val
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
    }

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

  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID() << "SetLogBufferLength" << length
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
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
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID() << "ResetLog"
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::EnableCheckCallback()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID() << "SetEnableLog" << this->EnableCheck->GetState()
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Save()
{
  char *filename;

  this->Script("tk_getSaveFile -filetypes {{{Text} {.txt}}} -defaultextension .txt -initialfile ParaViewLog.txt");
  filename = new char[strlen(this->GetApplication()->GetMainInterp()->result)+1];
  sprintf(filename, "%s", this->GetApplication()->GetMainInterp()->result);
  
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

  // Read back the log from the list.
  this->Update();
  *fptr << this->DisplayText->GetValue() << endl;

  fptr->close();
  delete fptr;
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Append(const char* msg)
{
  if (msg == NULL)
    {
    return;
    }

  char* str;
  char* newStr = new char[strlen(msg) + 1];
  memcpy(newStr, msg, strlen(msg)+1);
  // Get Rid of problem characters
  str = newStr;
  while (*str != '\0')
    {
    if (*str == '{' || *str == '}' || *str == '\\')
      {
      *str = ' ';
      }
    ++str;
    }
  this->DisplayText->AppendValue(newStr);
  this->DisplayText->AppendValue("\n");
  delete [] newStr;

  // Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview end", 
               this->DisplayText->GetTextWidget()->GetWidgetName());
  this->Script("update");                   
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Update()
{
  vtkPVApplication *pvApp;

  pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  if (pvApp == NULL)
    {
    vtkErrorMacro("Could not get pv application.");
    return;
    }

  if (this->TimerInformation)
    {
    // I have no method to clear the information yet.
    this->TimerInformation->Delete();
    this->TimerInformation = NULL;
    }
  this->TimerInformation = vtkPVTimerInformation::New();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GatherInformation(this->TimerInformation, pm->GetProcessModuleID());

  // Special case for client-server.  add the client process as a log.
  if (pvApp->GetOptions()->GetClientMode())
    {
    vtkPVTimerInformation* tmp = vtkPVTimerInformation::New();
    tmp->CopyFromObject(pvApp);
    tmp->AddInformation(this->TimerInformation);
    this->TimerInformation->Delete();
    this->TimerInformation = tmp;
    }

  this->DisplayLog();
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::DisplayLog()
{
  int numLogs;
  int id;
 
  numLogs = this->TimerInformation->GetNumberOfLogs();
  
  this->DisplayText->SetValue("");
  for (id = 0; id < numLogs; ++id)
    {
    char* str = this->TimerInformation->GetLog(id);

    if (numLogs > 1)
      {
      char tmp[128];
      sprintf(tmp, "Log %d:", id);
      this->Append("");
      this->Append(tmp);
      }

    if (str == NULL)
      {
      vtkWarningMacro("Null Log. " << id << " of " << numLogs);
      return;
      }

    char *start, *end;
    int count, length;
    length = strlen(str);
    char* strCopy = new char[length+1];
    memcpy(strCopy, str, length+1);

    //this->DisplayText->SetValue(strCopy);
    // Put the strings in one by one to avoid a Tk bug.
    // Strings seemed to be put in recursively, so we run out of stack.
    count = 0;
    start = end = strCopy;
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
    delete [] strCopy;
    strCopy = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Dismiss()
{
  this->Script("grab release %s", this->GetWidgetName());
  this->Script("wm withdraw %s", this->GetWidgetName());
}


//----------------------------------------------------------------------------
vtkPVTimerInformation* vtkPVTimerLogDisplay::GetTimerInformation()
{
  return this->TimerInformation;
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVTimerLogDisplay::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->MasterWindow);

  this->PropagateEnableState(this->ControlFrame);
  this->PropagateEnableState(this->SaveButton);
  this->PropagateEnableState(this->ClearButton);
  this->PropagateEnableState(this->ThresholdLabel);
  this->PropagateEnableState(this->ThresholdMenu);
  this->PropagateEnableState(this->BufferLengthLabel);
  this->PropagateEnableState(this->BufferLengthMenu);
  this->PropagateEnableState(this->EnableLabel);
  this->PropagateEnableState(this->EnableCheck);

  this->PropagateEnableState(this->DisplayText);

  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->DismissButton);
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "Threshold: " << this->Threshold << endl;
  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "TimerInformation:";
  if ( this->TimerInformation )
    {
    os << "\n";
    this->TimerInformation->PrintSelf(os, i2);
    }
  else
    {
    os << " (none)" << endl;
    }
}
