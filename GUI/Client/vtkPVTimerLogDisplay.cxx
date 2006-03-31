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

#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVTimerInformation.h"
#include "vtkPVApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWText.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkKWScale.h"
#include "vtkClientServerStream.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVTimerLogDisplay );
vtkCxxRevisionMacro(vtkPVTimerLogDisplay, "1.42");

//----------------------------------------------------------------------------
vtkPVTimerLogDisplay::vtkPVTimerLogDisplay()
{
  this->ButtonFrame = vtkKWFrame::New();
  this->DismissButton = vtkKWPushButton::New();
  
  this->DisplayText = vtkKWTextWithScrollbars::New();

  this->ControlFrame = vtkKWFrame::New();
  this->SaveButton = vtkKWPushButton::New();
  this->ClearButton = vtkKWPushButton::New();
  this->RefreshButton = vtkKWPushButton::New();
  this->ThresholdLabel = vtkKWLabel::New();
  this->ThresholdMenu = vtkKWMenuButton::New();
  this->BufferLengthLabel = vtkKWLabel::New();
  this->BufferLengthMenu = vtkKWMenuButton::New();
  this->EnableLabel = vtkKWLabel::New();
  this->EnableCheck = vtkKWCheckButton::New();

  this->SetTitle("Log");
  
  //this->TagNumber = 1;
  //this->CommandIndex = 0;
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
  this->RefreshButton->Delete();
  this->RefreshButton = NULL;
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
  
  if (this->TimerInformation)
    {
    this->TimerInformation->Delete();
    this->TimerInformation = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create();
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());

  this->DismissButton->SetParent(this->ButtonFrame);
  this->DismissButton->Create();
  this->DismissButton->SetCommand(this, "Withdraw");
  this->DismissButton->SetText("Dismiss");
  this->Script("pack %s -side left -expand 1 -fill x",
               this->DismissButton->GetWidgetName());

  this->DisplayText->SetParent(this);
  this->DisplayText->Create();
  this->DisplayText->VerticalScrollbarVisibilityOn();

  vtkKWText *text = this->DisplayText->GetWidget();
  text->ResizeToGridOn();
  text->SetWidth(100);
  text->SetHeight(40);
  text->SetWrapToWord();
  text->ReadOnlyOn();

  this->Script("pack %s -side bottom -expand 1 -fill both",
               this->DisplayText->GetWidgetName());

  this->ControlFrame->SetParent(this);
  this->ControlFrame->Create();
  this->Script("pack %s -side bottom -fill x -expand 0 -pady 2m",
               this->ControlFrame->GetWidgetName());
  this->SaveButton->SetParent(this->ControlFrame);
  this->SaveButton->Create();
  this->SaveButton->SetCommand(this, "Save");
  this->SaveButton->SetText("Save");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->SaveButton->GetWidgetName());
  this->ClearButton->SetParent(this->ControlFrame);
  this->ClearButton->Create();
  this->ClearButton->SetCommand(this, "Clear");
  this->ClearButton->SetText("Clear");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->ClearButton->GetWidgetName());
  this->RefreshButton->SetParent(this->ControlFrame);
  this->RefreshButton->Create();
  this->RefreshButton->SetCommand(this, "Update");
  this->RefreshButton->SetText("Refresh");
  this->Script("pack %s -side left -expand 0 -fill none",
               this->RefreshButton->GetWidgetName());

  this->ThresholdLabel->SetParent(this->ControlFrame);
  this->ThresholdLabel->Create();
  this->ThresholdLabel->SetText("Time Threshold:");
  this->ThresholdLabel->SetBalloonHelpString("This option filters out short duration events.");
  this->ThresholdMenu->SetParent(this->ControlFrame);
  this->ThresholdMenu->Create();
  this->ThresholdMenu->GetMenu()->AddRadioButton(
    "0.0", this, "SetThreshold 0.001");
  this->ThresholdMenu->GetMenu()->AddRadioButton(
    "0.001", this, "SetThreshold 0.001");
  this->ThresholdMenu->GetMenu()->AddRadioButton(
    "0.01", this, "SetThreshold 0.01");
  this->ThresholdMenu->GetMenu()->AddRadioButton(
    "0.1", this, "SetThreshold 0.1");
  this->ThresholdMenu->SetValue("0.01");
  this->SetThreshold(0.01);

  this->ThresholdMenu->SetBalloonHelpString("This option filters out short duration events.");
  this->Script("pack %s %s -side left",
               this->ThresholdLabel->GetWidgetName(),
               this->ThresholdMenu->GetWidgetName());

  this->BufferLengthLabel->SetParent(this->ControlFrame);
  this->BufferLengthLabel->Create();
  this->BufferLengthLabel->SetText("Buffer Length:");
  this->BufferLengthLabel->SetBalloonHelpString("Set how many entries the log can have.");
  this->BufferLengthMenu->SetParent(this->ControlFrame);
  this->BufferLengthMenu->Create();
  this->BufferLengthMenu->GetMenu()->AddRadioButton(
    "100", this, "SetBufferLength 100");
  this->BufferLengthMenu->GetMenu()->AddRadioButton(
    "500", this, "SetBufferLength 500");
  this->BufferLengthMenu->GetMenu()->AddRadioButton(
    "1000", this, "SetBufferLength 1000");
  this->BufferLengthMenu->GetMenu()->AddRadioButton(
    "5000", this, "SetBufferLength 5000");
  this->BufferLengthMenu->SetValue("500");
  this->SetBufferLength(500);
  this->BufferLengthMenu->SetBalloonHelpString("Set how many entries the log can have.");
  this->Script("pack %s %s -side left",
               this->BufferLengthLabel->GetWidgetName(),
               this->BufferLengthMenu->GetWidgetName());

  this->EnableLabel->SetParent(this->ControlFrame);
  this->EnableLabel->Create();
  this->EnableLabel->SetText("Enable:");
  this->EnableLabel->SetBalloonHelpString("Enable or disable loging of new events.");
  this->EnableCheck->SetParent(this->ControlFrame);
  this->EnableCheck->Create();
  this->EnableCheck->SetSelectedState(1);
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
  this->Superclass::Display();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::SetThreshold(float val)
{
  this->Modified();

  vtkPVApplication* pvApp = this->GetPVApplication();
  if ( pvApp )
    {
    vtkProcessModule* pm = pvApp->GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << pm->GetProcessModuleID() << "SetLogThreshold" << val
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
      vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
    }

  this->Threshold = val;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::SetBufferLength(int length)
{
  if (length == vtkTimerLog::GetMaxEntries()/2)
    {
    return;
    }
  this->Modified();

  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID() << "SetLogBufferLength" << 2*length
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
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
  vtkProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID() << "ResetLog"
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVTimerLogDisplay::EnableCheckCallback(int state)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID() << "SetEnableLog" << state
         << vtkClientServerStream::End;
  pm->SendStream(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), 
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
  *fptr << this->DisplayText->GetWidget()->GetText() << endl;

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
  this->DisplayText->GetWidget()->AppendText(newStr);
  this->DisplayText->GetWidget()->AppendText("\n");
  delete [] newStr;

  // Try to get the scroll bar to initialize properly (show correct portion).
  this->Script("%s yview end", 
               this->DisplayText->GetWidget()->GetWidgetName());
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
  vtkProcessModule* pm = pvApp->GetProcessModule();
  pm->GatherInformation(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
    vtkProcessModule::DATA_SERVER,
    this->TimerInformation, pm->GetProcessModuleID());

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

  this->DisplayText->GetWidget()->SetText("");
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

  this->PropagateEnableState(this->ControlFrame);
  this->PropagateEnableState(this->SaveButton);
  this->PropagateEnableState(this->ClearButton);
  this->PropagateEnableState(this->RefreshButton);
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
