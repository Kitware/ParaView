/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderGroupDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderGroupDialog.h"

#include "vtkKWApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWWindow.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVRenderGroupDialog );
vtkCxxRevisionMacro(vtkPVRenderGroupDialog, "1.8");

int vtkPVRenderGroupDialogCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRenderGroupDialog::vtkPVRenderGroupDialog()
{
  this->CommandFunction = vtkPVRenderGroupDialogCommand;
  
  this->ButtonFrame = vtkKWWidget::New();
  this->AcceptButton = vtkKWPushButton::New();
  
  // Created only for unix.
  this->DisplayFlag = 0;
  this->DisplayFrame = vtkKWWidget::New();
  this->Display0Label = vtkKWLabel::New();
  this->DisplayEntries = NULL;
  this->DisplayStringRoot = NULL;

  this->ControlFrame = vtkKWWidget::New();
  this->NumberLabel = vtkKWLabel::New();
  this->NumberEntry = vtkKWEntry::New();

  this->Title = NULL;
  this->SetTitle("Select Rendering Group");
  
  this->MasterWindow = 0;
  this->NumberOfProcessesInGroup = 0;
  this->Writable = 0;

  this->AcceptedFlag = 1;
}

//----------------------------------------------------------------------------
vtkPVRenderGroupDialog::~vtkPVRenderGroupDialog()
{
  int idx;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  
  this->DisplayFrame->Delete();
  this->DisplayFrame = NULL;
  this->Display0Label->Delete();
  this->Display0Label = NULL;

  for ( idx = 0; idx < this->NumberOfProcessesInGroup; ++idx)
    {
    if (this->DisplayEntries[idx])
      {
      this->DisplayEntries[idx]->Delete();
      this->DisplayEntries[idx] = NULL;
      }
    }
  if (this->DisplayEntries)
    {
    delete [] this->DisplayEntries;
    this->DisplayEntries = NULL;
    }
  if (this->DisplayStringRoot)
    {
    delete [] this->DisplayStringRoot;
    this->DisplayStringRoot = NULL;
    }
 
  this->ControlFrame->Delete();
  this->ControlFrame = NULL;
  this->NumberLabel->Delete();
  this->NumberLabel = NULL;
  this->NumberEntry->Delete();
  this->NumberEntry = NULL;
  
  this->SetTitle(NULL);
  this->SetMasterWindow(0);
}

//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::SetMasterWindow(vtkKWWindow* win)
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
void vtkPVRenderGroupDialog::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "toplevel", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  const char *wname = this->GetWidgetName();
  this->Script("wm title %s \"%s\"", wname, this->Title);
  this->Script("wm iconname %s \"vtk\"", wname);

  int idx;

  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }
  else
    {
    int sw, sh;
    this->Script("concat [winfo screenwidth %s] [winfo screenheight %s]",
                 this->GetWidgetName(), this->GetWidgetName());
    sscanf(app->GetMainInterp()->result, "%d %d", &sw, &sh);

    int ww, wh;
    this->Script("concat [winfo reqwidth %s] [winfo reqheight %s]",
                 this->GetWidgetName(), this->GetWidgetName());
    sscanf(app->GetMainInterp()->result, "%d %d", &ww, &wh);
    this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), 
                 (sw-ww)/2, (sh-wh)/2);
    }

  this->ControlFrame->SetParent(this);
  this->ControlFrame->Create(app, "frame", "");
  this->Script("pack %s -side top -fill x -expand 0 -pady 2m",
               this->ControlFrame->GetWidgetName());
  this->NumberLabel->SetParent(this->ControlFrame);
  this->NumberLabel->Create(app, "");
  this->NumberLabel->SetLabel("Number of Processes in Rendering Group:");
  this->NumberLabel->SetBalloonHelpString(
    "Specify how many processes you want to use for rendering.");
  this->NumberEntry->SetParent(this->ControlFrame);
  this->NumberEntry->Create(app, "");
  this->NumberEntry->SetBalloonHelpString(
    "This option filters out short duration events.");
  this->Script("pack %s %s -side left",
               this->NumberLabel->GetWidgetName(),
               this->NumberEntry->GetWidgetName());
  this->Script("bind %s <KeyPress-Return> {%s NumberEntryCallback}",
               this->NumberEntry->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <FocusOut> {%s NumberEntryCallback}",
               this->NumberEntry->GetWidgetName(), this->GetTclName());

  this->DisplayFrame->SetParent(this);
  this->DisplayFrame->Create(app, "frame", "");
  this->Display0Label->SetParent(this->DisplayFrame);
  this->Display0Label->Create(app, " -background white -justify left");

  for (idx = 0; idx < this->NumberOfProcessesInGroup; ++idx)
    {
    this->DisplayEntries[idx]->Create(app, "");
    }

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());
  this->AcceptButton->SetParent(this->ButtonFrame);
  this->AcceptButton->Create(app, "");
  this->AcceptButton->SetCommand(this, "Accept");
  this->AcceptButton->SetLabel("Accept");
  this->Script("pack %s -side left -expand 1 -fill x",
               this->AcceptButton->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW {wm withdraw %s}",
               wname, wname);

  this->Script("wm withdraw %s", wname);

  this->Update();

  this->Script("wm protocol %s WM_DELETE_WINDOW { %s Accept }",
               this->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::Invoke()
{   
  if (this->NumberOfProcessesInGroup == 0)
    {
    vtkErrorMacro("RenderGroupDialog has not been initialized.");
    }
  this->Script("wm deiconify %s", this->GetWidgetName());
  this->Script("grab %s", this->GetWidgetName());

  this->AcceptedFlag = 0;
  this->Update();
  while (this->AcceptedFlag == 0)
    {
    // I assume the update will process multiple events.
    this->Script("update");
    if (this->AcceptedFlag == 0)
      {
      this->Script("after 100");
      }
    }

  this->Script("grab release %s", this->GetWidgetName());
  this->Script("wm withdraw %s", this->GetWidgetName());
}



//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::Accept()
{
  // Accept might be pressed to set the value of number of processes.
  if ((this->NumberOfProcessesInGroup != this->NumberEntry->GetValueAsInt()))
    {
    this->NumberEntryCallback();
    // They might want to change the display variables
    if (this->DisplayStringRoot)
      {
      return;
      }
    }
  this->AcceptedFlag = 1;
}




//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::NumberEntryCallback()
{
  int num;

  num = this->NumberEntry->GetValueAsInt();
  if (num == this->NumberOfProcessesInGroup)
    {
    return;
    }
  // Do not allow less than 1 rendering process.
  if (num < 1)
    {
    num = 1;;
    }
  vtkPVApplication* pvApp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  if (pvApp)
    {
    int numProcs = pvApp->GetProcessModule()->GetNumberOfPartitions();
    if (num > numProcs) { num = numProcs; }
    }
  this->SetNumberOfProcessesInGroup(num);
}


//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::SetNumberOfProcessesInGroup(int num)
{
  int idx;

  // Protect against crazy input.
  if (num > 100000)
    {
    vtkErrorMacro("NumberOfProcesses is too big.");
    return;
    }
  if (num < 1)
    {
    num = 1;
    }
  if (num == this->NumberOfProcessesInGroup)
    {
    return;
    }

  this->Modified();
  // Always create these display entries, even when we do not use them.
  if (num < this->NumberOfProcessesInGroup)
    {
    for (idx = num; idx < this->NumberOfProcessesInGroup; ++idx)
      {
      this->DisplayEntries[idx]->Delete();
      this->DisplayEntries[idx] = NULL;
      }
    } 
  else
    {
    vtkKWEntry **tmp;
    tmp = new vtkKWEntry*[num];
    // Copy from old to new.
    for (idx = 0; idx < this->NumberOfProcessesInGroup; ++idx)
      {
      tmp[idx] = this->DisplayEntries[idx];
      this->DisplayEntries[idx] = NULL;
      }
    // Initialize the rest.
    for ( idx = this->NumberOfProcessesInGroup; idx < num; ++idx)
      {
      tmp[idx] = vtkKWEntry::New();
      tmp[idx]->SetParent(this->DisplayFrame);
      if (this->IsCreated())
        {
        tmp[idx]->Create(this->GetApplication(), "");
        }
      }
      if (this->DisplayEntries)
        {
        delete [] this->DisplayEntries;
        this->DisplayEntries = NULL;
        }
      this->DisplayEntries = tmp;
      tmp = NULL;
    }

  this->NumberOfProcessesInGroup = num;
  this->Update();
}


//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::SetDisplayString(int idx, const char* str)
{
  // Let the first display string be set before the number of processes.
  if (this->NumberOfProcessesInGroup == 0)
    {
    this->SetNumberOfProcessesInGroup(1);
    }

  if (idx < 0 || idx >= this->NumberOfProcessesInGroup)
    {
    vtkErrorMacro("Index not in process group.");
    return;
    }
  
  if (str)
    {
    this->DisplayFlag = 1;
    if (idx == 0)
      {
      this->ComputeDisplayStringRoot(str);
      this->Display0Label->SetLabel(str);
      }
    else
      {
      this->DisplayEntries[idx]->SetValue(str);
      }
    }
  else  
    {
    vtkErrorMacro("Cannot set display to NULL");
    }

  this->Update();
}



//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::ComputeDisplayStringRoot(const char* str)
{
  if (this->DisplayStringRoot)
    {
    delete [] this->DisplayStringRoot;
    this->DisplayStringRoot = NULL;
    }

  if (str == NULL || strlen(str) == 0)
    {
    return;
    }

  // Extract the position of the display from the string.
  int len = -1;
  int j, i = 0;
  while (i < 80)
    {
    if (str[i] == ':')
      {
      j = i+1;
      while (j < 80)
        {
        if (str[j] == '.')
          {
          len = j+1;
          break;
          }
        j++;
        }
      break;
      }
    i++;
    }
  
  if (len == -1)
    {
    len = static_cast<int>(strlen(str));
    this->DisplayStringRoot = new char[len+2];
    strcpy(this->DisplayStringRoot, str);
    this->DisplayStringRoot[len] = '.';
    this->DisplayStringRoot[len+1] = '\0';
    }
  else
    {
    this->DisplayStringRoot = new char[len+1];
    strncpy(this->DisplayStringRoot, str, len);
    this->DisplayStringRoot[len] = '\0';
    }
}


//----------------------------------------------------------------------------
const char* vtkPVRenderGroupDialog::GetDisplayString(int idx)
{
  if (idx < 0 || idx >= this->NumberOfProcessesInGroup)
    {
    vtkErrorMacro("Index not in process group.");
    return NULL;
    }
  
  if (idx == 0)
    {
    return this->Display0Label->GetLabel();
    }
  else
    {
    return this->DisplayEntries[idx]->GetValue();
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::Update()
{
  if (this->GetApplication() == NULL)
    {
    return;
    }

  this->NumberEntry->SetValue(this->NumberOfProcessesInGroup);

  if (this->DisplayFlag)
    {
    int idx;
    this->Script("pack %s -side top -after %s -fill x -expand 1", 
                 this->DisplayFrame->GetWidgetName(),
                 this->ControlFrame->GetWidgetName());
    this->Script("catch {eval pack forget [pack slaves %s]}",
                 this->DisplayFrame->GetWidgetName());
    this->Script("pack %s -side top -fill x -expand 1", 
                 this->Display0Label->GetWidgetName());
    for (idx = 1; idx < this->NumberOfProcessesInGroup; ++idx)
      {
      const char *oldStr = this->DisplayEntries[idx]->GetValue();
      // Use the display root to initialize the entry.
      if (this->DisplayStringRoot && (oldStr == NULL || strlen(oldStr) == 0))
        {
        char *str = new char[strlen(this->DisplayStringRoot) + 10];
        sprintf(str, "%s%d", this->DisplayStringRoot, idx);
        this->DisplayEntries[idx]->SetValue(str);
        delete [] str;      
        }
      this->Script("pack %s -side top -fill x -expand 1", 
                   this->DisplayEntries[idx]->GetWidgetName());
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderGroupDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "NumberOfProcessesInGroup: " 
     << this->NumberOfProcessesInGroup << endl;
}
