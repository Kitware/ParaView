/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWLabeledEntry.h"
#include "vtkKWMessageDialog.h"

#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkPVAssignment.h"
#include "vtkKWView.h"
#include "vtkKWScale.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSelectionList.h"
#include "vtkPVCommandList.h"


int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;
  this->Name = NULL;
  
  this->Inputs = NULL;
  this->NumberOfInputs = 0;
  this->PVOutput = NULL;
  this->VTKSource = NULL;
  this->VTKSourceTclName = NULL;

  this->Properties = vtkKWWidget::New();
  
  this->DataCreated = 0;

  this->NavigationFrame = vtkKWLabeledFrame::New();
  this->NavigationCanvas = vtkKWWidget::New();
  this->ParameterFrame = vtkKWLabeledFrame::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();
  
  this->Widgets = vtkKWWidgetCollection::New();
  this->LastSelectionList = NULL;
  
  this->AcceptCommands = vtkPVCommandList::New();
  this->CancelCommands = vtkPVCommandList::New();
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  int i;
  
  if (this->PVOutput)
    {
    this->PVOutput->UnRegister(this);
    this->PVOutput = NULL;
    }
  
  for (i = 0; i < this->NumberOfInputs; i++)
    {
    if (this->Inputs[i])
      {
      this->Inputs[i]->UnRegister(this);
      this->Inputs[i] = NULL;
      }
    }

  if (this->Inputs)
    {
    delete [] this->Inputs;
    this->Inputs = 0;
    }
  
  this->NumberOfInputs = 0;

  this->SetVTKSource(NULL);

  this->SetName(NULL);
  this->Properties->Delete();
  this->Properties = NULL;

  this->NavigationFrame->Delete();
  this->NavigationFrame = NULL;
  this->NavigationCanvas->Delete();
  this->NavigationCanvas = NULL;

  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->Widgets->Delete();
  this->Widgets = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;  
  
  this->CancelButton->Delete();
  this->CancelButton = NULL;  
  
  this->DeleteButton->Delete();
  this->DeleteButton = NULL;
  
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    this->LastSelectionList = NULL;
    }

  this->AcceptCommands->Delete();
  this->AcceptCommands = NULL;  
  this->CancelCommands->Delete();
  this->CancelCommands = NULL;
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVSource::New()
{
  return new vtkPVSource();
}

//----------------------------------------------------------------------------
void vtkPVSource::Clone(vtkPVApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);

  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());

  // The clones might as well have an application.
  pvApp->BroadcastScript("%s SetApplication %s", this->GetTclName(),
			 pvApp->GetTclName());  
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void vtkPVSourceStartProgress(void *arg)
{
  vtkPVSource *me = (vtkPVSource*)arg;
  vtkSource *vtkSource = me->GetVTKSource();
  static char str[200];
  
  if (vtkSource && me->GetWindow())
    {
    sprintf(str, "Processing %s", vtkSource->GetClassName());
    me->GetWindow()->SetStatusText(str);
    }
}
//----------------------------------------------------------------------------
void vtkPVSourceReportProgress(void *arg)
{
  vtkPVSource *me = (vtkPVSource*)arg;
  vtkSource *vtkSource = me->GetVTKSource();

  if (me->GetWindow())
    {
    me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
    }
}
//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void *arg)
{
  vtkPVSource *me = (vtkPVSource*)arg;
  
  if (me->GetWindow())
    {
    me->GetWindow()->SetStatusText("");
    me->GetWindow()->GetProgressGauge()->SetValue(0);
    }
}
//----------------------------------------------------------------------------
void vtkPVSource::SetVTKSource(vtkSource *source)
{
  if (this->VTKSource == source)
    {
    return;
    }
  this->Modified();

  // Get rid of the old TclName string
  if (this->VTKSourceTclName)
    {
    delete this->VTKSourceTclName;
    this->VTKSourceTclName = NULL;
    }

  // Get rid of old VTKSource reference.
  if (this->VTKSource)
    {
    // Be extra careful of circular references. (not important here...)
    vtkSource *tmp = this->VTKSource;
    this->VTKSource = NULL;
    // Should we unset the progress methods?
    tmp->UnRegister(this);
    }
  if (source)
    {
    this->VTKSource = source;
    source->Register(this);
    // Set up the progress methods.
    source->SetStartMethod(vtkPVSourceStartProgress, this);
    source->SetProgressMethod(vtkPVSourceReportProgress, this);
    source->SetEndMethod(vtkPVSourceEndProgress, this);
    }
}

//----------------------------------------------------------------------------
const char *vtkPVSource::GetVTKSourceTclName()
{
 const char *myTclName;

 if (this->VTKSourceTclName == NULL)
    {
    // Create the new VTKSourceTclName.
    myTclName = this->GetTclName();
    this->VTKSourceTclName = new char[strlen(myTclName) + 17];
    sprintf(this->VTKSourceTclName, "[%s GetVTKSource]", myTclName);
    }
  return this->VTKSourceTclName;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVData(vtkPVData *data)
{
  if (this->PVOutput == data)
    {
    return;
    }
  this->Modified();

  if (this->PVOutput)
    {
    // extra careful for circular references
    vtkPVData *tmp = this->PVOutput;
    this->PVOutput = NULL;
    // Manage double pointer.
    tmp->SetPVSource(NULL);
    tmp->UnRegister(this);
    }
  if (data)
    {
    this->PVOutput = data;
    data->Register(this);
    // Manage double pointer.
    data->SetPVSource(this);
    }
}

//----------------------------------------------------------------------------
vtkPVWindow* vtkPVSource::GetWindow()
{
  if (this->View == NULL || this->View->GetParentWindow() == NULL)
    {
    return NULL;
    }
  
  return vtkPVWindow::SafeDownCast(this->View->GetParentWindow());
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSource::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateProperties()
{ 
  const char *sourcePage;
  vtkPVApplication *app = this->GetPVApplication();

  // invoke super
  this->vtkKWComposite::CreateProperties();  

  // Set up the pages of the notebook.
  sourcePage = this->GetClassName();
  this->Notebook->AddPage(sourcePage);
  this->Properties->SetParent(this->Notebook->GetFrame(sourcePage));
  this->Properties->Create(this->Application,"frame","");
  this->Script("pack %s -pady 2 -fill x -expand yes",
               this->Properties->GetWidgetName());
  
  // Setup the source page of the notebook.
  this->NavigationFrame->SetParent(this->Properties);
  this->NavigationFrame->Create(this->Application);
  this->NavigationFrame->SetLabel("Navigation");
  this->Script("pack %s -fill x -expand t -side top", this->NavigationFrame->GetWidgetName());
  this->NavigationCanvas->SetParent(this->NavigationFrame->GetFrame());
  this->NavigationCanvas->Create(this->Application, "canvas", "-height 45 -bg white"); 
  this->Script("pack %s -fill x -expand t -side top", this->NavigationCanvas->GetWidgetName());

  this->ParameterFrame->SetParent(this->Properties);
  this->ParameterFrame->Create(this->Application);
  this->ParameterFrame->SetLabel("Parameters");
  this->Script("pack %s -fill x -expand t -side top", this->ParameterFrame->GetWidgetName());

  vtkKWWidget *frame = vtkKWWidget::New();
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());  
  
  this->AcceptButton->SetParent(frame);
  this->AcceptButton->Create(this->Application, "-text Accept");
  this->AcceptButton->SetCommand(this, "AcceptCallback");
  this->Script("pack %s -side left -fill x -expand t", 
	       this->AcceptButton->GetWidgetName());

  this->CancelButton->SetParent(frame);
  this->CancelButton->Create(this->Application, "-text Cancel");
  this->CancelButton->SetCommand(this, "CancelCallback");
  this->Script("pack %s -side left -fill x -expand t", 
	       this->CancelButton->GetWidgetName());

  this->DeleteButton->SetParent(frame);
  this->DeleteButton->Create(this->Application, "-text Delete");
  this->DeleteButton->SetCommand(this, "DeleteCallback");
  this->Script("pack %s -side left -fill x -expand t",
               this->DeleteButton->GetWidgetName());
  
  // Every source has a name.
  this->AddLabeledEntry("Name:", "SetName", "GetName", this);

  this->UpdateNavigationCanvas();
  this->UpdateParameterWidgets();
}

//----------------------------------------------------------------------------
void vtkPVSource::CreateDataPage()
{
  if (!this->DataCreated)
    {
    const char *dataPage;
    vtkPVData *data = this->GetPVData();
    
    if (data == NULL)
      {
      vtkErrorMacro("must have data before creating data page");
      return;
      }
    
    dataPage = data->GetClassName();
    this->Notebook->AddPage(dataPage);
    
    data->SetParent(this->Notebook->GetFrame(dataPage));
    data->Create("");
    this->Script("pack %s", data->GetWidgetName());
    
    this->DataCreated = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::Select(vtkKWView *v)
{
  // invoke super
  this->vtkKWComposite::Select(v); 
  vtkKWMenu* MenuProperties = v->GetParentWindow()->GetMenuProperties();
  char* rbv = 
    MenuProperties->CreateRadioButtonVariable(MenuProperties,"Radio");

  // now add our own menu options
  if (MenuProperties->GetRadioButtonValue(MenuProperties,"Radio") >= 10)
    {
    if (this->LastSelectedProperty == 10)
      {
      this->View->ShowViewProperties();
      }
    if (this->LastSelectedProperty == 100 || this->LastSelectedProperty == -1)
      {
      this->ShowProperties();
      }
    }

  this->UpdateNavigationCanvas();

  MenuProperties->AddRadioButton(100, " Data", rbv, this, "ShowProperties");
  delete [] rbv;
}

//----------------------------------------------------------------------------
void vtkPVSource::Deselect(vtkKWView *v)
{
  // invoke super
  this->vtkKWComposite::Deselect(v);
  v->GetParentWindow()->GetMenuProperties()->DeleteMenuItem(" Data");
}

//----------------------------------------------------------------------------
void vtkPVSource::ShowProperties()
{
  vtkKWWindow *pw = this->View->GetParentWindow();
  pw->ShowProperties();
  pw->GetMenuProperties()->CheckRadioButton(
    pw->GetMenuProperties(),"Radio",100);
  
  // unpack any current children
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->View->GetPropertiesParent()->GetWidgetName());
  
  if (!this->PropertiesCreated)
    {
    this->InitializeProperties();
    }
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->View->PackProperties();
}

//----------------------------------------------------------------------------
char* vtkPVSource::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkPVSource::SetName (const char* arg) 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " 
                << this->Name << " to " << arg ); 
  if ( this->Name && arg && (!strcmp(this->Name,arg))) 
    { 
    return;
    } 
  if (this->Name) 
    { 
    delete [] this->Name; 
    } 
  if (arg) 
    { 
    this->Name = new char[strlen(arg)+1]; 
    strcpy(this->Name,arg); 
    } 
  else 
    { 
    this->Name = NULL;
    }
  this->Modified(); 
} 

//----------------------------------------------------------------------------
void vtkPVSource::SetVisibility(int v)
{
  vtkProp * p = this->GetProp();
  vtkPVApplication *pvApp;
  
  if (p)
    {
    p->SetVisibility(v);
    }
  
  pvApp = (vtkPVApplication*)(this->Application);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetVisibility %d", this->GetTclName(), v);
    }
}

  
//----------------------------------------------------------------------------
int vtkPVSource::GetVisibility()
{
  vtkProp *p = this->GetProp();
  
  if (p == NULL)
    {
    return 0;
    }
  
  return p->GetVisibility();
}



//----------------------------------------------------------------------------
vtkKWCheckButton *vtkPVSource::AddLabeledToggle(char *label, char *setCmd, char *getCmd, 
                                                vtkKWObject *o)
{
  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // A frame to hold the other widgets.
  vtkKWWidget *frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    vtkKWLabel *labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // Now the check button
  vtkKWCheckButton *check = vtkKWCheckButton::New();
  this->Widgets->AddItem(check);
  check->SetParent(frame);
  check->Create(this->Application, "");
  this->Script("pack %s -side left", check->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand("%s SetState [%s %s]",
          check->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s [%s GetState]",
          this->GetTclName(), tclName, setCmd, check->GetTclName()); 

  this->Script("pack %s -side left", check->GetWidgetName());

  frame->Delete();
  check->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return check;
}  
//----------------------------------------------------------------------------
vtkKWEntry *vtkPVSource::AddFileEntry(char *label, char *setCmd, char *getCmd,
                                      char *ext, vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *entry;
  vtkKWPushButton *browseButton;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());


  browseButton = vtkKWPushButton::New();
  this->Widgets->AddItem(browseButton);
  browseButton->SetParent(frame);
  browseButton->Create(this->Application, "");
  browseButton->SetLabel("Browse");
  this->Script("pack %s -side left", browseButton->GetWidgetName());
  if (ext)
    {
    char str[1000];
    sprintf(str, "SetValue [tk_getOpenFile -filetypes {{{} {.%s}}}]", ext);
    browseButton->SetCommand(entry, str);
    }
  else
    {
    browseButton->SetCommand(entry, "SetValue [tk_getOpenFile]");
    }
  browseButton->Delete();
  browseButton = NULL;

  // Command to update the UI.
  this->CancelCommands->AddCommand("%s SetValue [%s %s]",
             entry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s [%s GetValue]",
             this->GetTclName(), tclName, setCmd, entry->GetTclName());

  frame->Delete();
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return entry;
}

//----------------------------------------------------------------------------
vtkKWEntry *vtkPVSource::AddLabeledEntry(char *label, char *setCmd, char *getCmd,
                                         vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *entry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  entry = vtkKWEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(frame);
  entry->Create(this->Application, "");
  this->Script("pack %s -side left -fill x -expand t", entry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand("%s SetValue [%s %s]",
             entry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s [%s GetValue]",
             this->GetTclName(), tclName, setCmd, entry->GetTclName());

  frame->Delete();
  entry->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return entry;
}

//----------------------------------------------------------------------------
void vtkPVSource::AddVector2Entry(char *label, char *l1, char *l2,
				                          char *setCmd, char *getCmd, vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *minEntry, *maxEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // Min
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  minEntry = vtkKWEntry::New();
  this->Widgets->AddItem(minEntry);
  minEntry->SetParent(frame);
  minEntry->Create(this->Application, "-width 7");
  this->Script("pack %s -side left -fill x -expand t", minEntry->GetWidgetName());

  // Max
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }  
  maxEntry = vtkKWEntry::New();
  this->Widgets->AddItem(maxEntry);
  maxEntry->SetParent(frame);
  maxEntry->Create(this->Application, "-width 7");
  this->Script("pack %s -side left -fill x -expand t", maxEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 0]", minEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 1]", maxEntry->GetTclName(), tclName, getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), tclName, setCmd, minEntry->GetTclName(),
                        maxEntry->GetTclName());

  frame->Delete();
  minEntry->Delete();
  maxEntry->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::AddVector3Entry(char *label, char *l1, char *l2, char *l3,
				  char *setCmd, char *getCmd, vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *xEntry, *yEntry, *zEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // X
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  xEntry = vtkKWEntry::New();
  this->Widgets->AddItem(xEntry);
  xEntry->SetParent(frame);
  xEntry->Create(this->Application, "-width 6");
  this->Script("pack %s -side left -fill x -expand t", xEntry->GetWidgetName());

  // Y
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  yEntry = vtkKWEntry::New();
  this->Widgets->AddItem(yEntry);
  yEntry->SetParent(frame);
  yEntry->Create(this->Application, "-width 6");
  this->Script("pack %s -side left -fill x -expand t", yEntry->GetWidgetName());

  // Z
  if (l3 && l3[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l3);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  zEntry = vtkKWEntry::New();
  this->Widgets->AddItem(zEntry);
  zEntry->SetParent(frame);
  zEntry->Create(this->Application, "-width 6");
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand("%s SetValue [lindex [%s %s] 0]", 
             xEntry->GetTclName(), tclName, getCmd); 
  this->CancelCommands->AddCommand("%s SetValue [lindex [%s %s] 1]", 
             yEntry->GetTclName(), tclName, getCmd); 
  this->CancelCommands->AddCommand("%s SetValue [lindex [%s %s] 2]", 
             zEntry->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), tclName, setCmd, xEntry->GetTclName(),
                        yEntry->GetTclName(), zEntry->GetTclName());

  frame->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
}


//----------------------------------------------------------------------------
void vtkPVSource::AddVector4Entry(char *label, char *l1, char *l2, char *l3,
                                  char *l4, char *setCmd, char *getCmd,
                                  vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *xEntry, *yEntry, *zEntry, *wEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // X
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  xEntry = vtkKWEntry::New();
  this->Widgets->AddItem(xEntry);
  xEntry->SetParent(frame);
  xEntry->Create(this->Application, "-width 5");
  this->Script("pack %s -side left -fill x -expand t", xEntry->GetWidgetName());

  // Y
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  yEntry = vtkKWEntry::New();
  this->Widgets->AddItem(yEntry);
  yEntry->SetParent(frame);
  yEntry->Create(this->Application, "-width 5");
  this->Script("pack %s -side left -fill x -expand t", yEntry->GetWidgetName());

  // Z
  if (l3 && l3[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l3);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  zEntry = vtkKWEntry::New();
  this->Widgets->AddItem(zEntry);
  zEntry->SetParent(frame);
  zEntry->Create(this->Application, "-width 5");
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // W
  if (l4 && l4[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l4);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  wEntry = vtkKWEntry::New();
  this->Widgets->AddItem(wEntry);
  wEntry->SetParent(frame);
  wEntry->Create(this->Application, "-width 5");
  this->Script("pack %s -side left -fill x -expand t", wEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 0]", xEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 1]", yEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 2]", zEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 3]", wEntry->GetTclName(), tclName, getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), tclName, setCmd, xEntry->GetTclName(),
                        yEntry->GetTclName(), zEntry->GetTclName(), wEntry->GetTclName());

  frame->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
  wEntry->Delete();
}

//----------------------------------------------------------------------------
// It might make sence here to store the labels in an array 
// so that a loop can be used to create the widgets.
void vtkPVSource::AddVector6Entry(char *label, char *l1, char *l2, char *l3,
                                  char *l4, char *l5, char *l6,
                                  char *setCmd, char *getCmd, vtkKWObject *o)

{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry  *uEntry, *vEntry, *wEntry, *xEntry, *yEntry, *zEntry;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "-width 19 -justify right");
    labelWidget->SetLabel(label);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  
  // U
  if (l1 && l1[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l1);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  uEntry = vtkKWEntry::New();
  this->Widgets->AddItem(uEntry);
  uEntry->SetParent(frame);
  uEntry->Create(this->Application, "-width 4");
  this->Script("pack %s -side left -fill x -expand t", uEntry->GetWidgetName());

  // V
  if (l2 && l2[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l2);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  vEntry = vtkKWEntry::New();
  this->Widgets->AddItem(vEntry);
  vEntry->SetParent(frame);
  vEntry->Create(this->Application, "-width 4");
  this->Script("pack %s -side left -fill x -expand t", vEntry->GetWidgetName());

  // W
  if (l3 && l3[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l3);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  wEntry = vtkKWEntry::New();
  this->Widgets->AddItem(wEntry);
  wEntry->SetParent(frame);
  wEntry->Create(this->Application, "-width 4");
  this->Script("pack %s -side left -fill x -expand t", wEntry->GetWidgetName());

  // X
  if (l4 && l4[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l4);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  xEntry = vtkKWEntry::New();
  this->Widgets->AddItem(xEntry);
  xEntry->SetParent(frame);
  xEntry->Create(this->Application, "-width 4");
  this->Script("pack %s -side left -fill x -expand t", xEntry->GetWidgetName());

  // Y
  if (l5 && l5[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l5);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  yEntry = vtkKWEntry::New();
  this->Widgets->AddItem(yEntry);
  yEntry->SetParent(frame);
  yEntry->Create(this->Application, "-width 4");
  this->Script("pack %s -side left -fill x -expand t", yEntry->GetWidgetName());

  // Z
  if (l6 && l6[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
    labelWidget->SetLabel(l6);
    this->Script("pack %s -side left", labelWidget->GetWidgetName());
    labelWidget->Delete();
    labelWidget = NULL;
    }
  zEntry = vtkKWEntry::New();
  this->Widgets->AddItem(zEntry);
  zEntry->SetParent(frame);
  zEntry->Create(this->Application, "-width 4");
  this->Script("pack %s -side left -fill x -expand t", zEntry->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 0]",uEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 1]",vEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 2]",wEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 3]",xEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 4]",yEntry->GetTclName(), tclName, getCmd);
  this->CancelCommands->AddCommand(
    "%s SetValue [lindex [%s %s] 5]",zEntry->GetTclName(), tclName, getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue]\"",
  	 this->GetTclName(), tclName, setCmd, uEntry->GetTclName(), 
         vEntry->GetTclName(), wEntry->GetTclName(),
         xEntry->GetTclName(), yEntry->GetTclName(), zEntry->GetTclName());

  frame->Delete();
  uEntry->Delete();
  vEntry->Delete();
  wEntry->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
}


//----------------------------------------------------------------------------
vtkKWScale *vtkPVSource::AddScale(char *label, char *setCmd, char *getCmd,
                                  float min, float max, float resolution,
                                  vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWScale *slider;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "-width 19 -justify right");
  labelWidget->SetLabel(label);
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  slider = vtkKWScale::New();
  this->Widgets->AddItem(slider);
  slider->SetParent(frame);
  slider->Create(this->Application, "-showvalue 1");
  slider->SetRange(min, max);
  slider->SetResolution(resolution);
  this->Script("pack %s -side left -fill x -expand t", slider->GetWidgetName());

  // Command to update the UI.
  this->CancelCommands->AddCommand("%s SetValue [%s %s]",
                  slider->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s [%s GetValue]",
                  this->GetTclName(), tclName, setCmd, slider->GetTclName());

  frame->Delete();
  labelWidget->Delete();
  slider->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return slider;
}


//----------------------------------------------------------------------------
vtkPVSelectionList *vtkPVSource::AddModeList(char *label, char *setCmd, char *getCmd,
                                             vtkKWObject *o)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;

  // Find the Tcl name of the object whose methods will be called.
  const char *tclName = this->GetVTKSourceTclName();
  if (o)
    {
    tclName = o->GetTclName();
    }

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s -fill x -expand t", frame->GetWidgetName());

  // Now a label
  labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "-width 19 -justify right");
  labelWidget->SetLabel(label);
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  vtkPVSelectionList *sl = vtkPVSelectionList::New();  
  this->Widgets->AddItem(sl);
  sl->SetParent(frame);
  sl->Create(this->Application);  
  this->Script("pack %s -fill x -expand t", sl->GetWidgetName());
    
  // Command to update the UI.
  this->CancelCommands->AddCommand("%s SetCurrentValue [%s %s]",
                   sl->GetTclName(), tclName, getCmd); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddCommand("%s AcceptHelper2 %s %s [%s GetCurrentValue]",
                   this->GetTclName(), tclName, setCmd, sl->GetTclName());
    
  // Save this selection list so the user can add items to it.
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    }
  sl->Register(this);
  this->LastSelectionList = sl;

  sl->Delete();
  labelWidget->Delete();
  frame->Delete();

  // Although it has been deleted, it did not destruct.
  // We need to change this into a PVWidget.
  return sl;
}

//----------------------------------------------------------------------------
void vtkPVSource::AddModeListItem(char *name, int value)
{
  if (this->LastSelectionList == NULL)
    {
    vtkErrorMacro("No selection list exists yet.");
    return;
    }
  this->LastSelectionList->AddItem(name, value);
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptCallback()
{
  int i;
  vtkPVWindow *window;

  window = this->GetWindow();
  
  // Call the commands to set ivars from widget values.
  for (i = 0; i < this->AcceptCommands->GetLength(); ++i)
    {
    this->Script(this->AcceptCommands->GetCommand(i));
    }  
  
  // Initialize the output if necessary.
  if (this->GetPVData() == NULL && this->GetVTKSource())
    { // This is the first time, initialize data.  
    vtkPVData *input;
    vtkPVActorComposite *ac;
    
    input = this->GetNthInput(0);
    this->InitializeOutput();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    // Make the last data invisible.
    if (input)
      {
      input->GetActorComposite()->SetVisibility(0);
      }
    window->GetMainView()->ResetCamera();
    }

  window->GetMainView()->SetSelectedComposite(this);  
  this->UpdateNavigationCanvas();
  this->GetView()->Render();
  window->GetSourceList()->Update();
}

//----------------------------------------------------------------------------
void vtkPVSource::CancelCallback()
{
  vtkPVApplication *pvApp = (vtkPVApplication*)this->Application;
  vtkPVSource *prev;
  int i;
  
  if (this->PVOutput == NULL)
    { // Accept has not been called yet.  Delete the object.
    // Need to remove the data connected with this source from the list of
    // inputs, but not sure how to do that since the PVData is NULL at this
    // point.
    
    for (i = 0; i < this->GetNumberOfInputs(); i++)
      {
      this->GetNthInput(i)->RemovePVSourceFromUsers(this);
      }
    
    // We need to unpack the notebook for this source and pack the one for the
    // source of this source (if there is one).

    prev = this->GetWindow()->GetPreviousSource();
    this->GetWindow()->SetCurrentSource(prev);
    if (prev)
      {
      prev->ShowProperties();
      }
    
    // We need to remove this source from the Source List.    
    this->GetWindow()->GetSourceList()->GetSources()->RemoveItem(this);
    this->GetWindow()->GetSourceList()->Update();    

    // Delete the source on the other processes.  Removing it from the
    // view's list of composites amounts to deleting the source on the
    // local processor because it's the last thing that has a reference to it.
    if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
      {
      pvApp->BroadcastScript("%s Delete", this->GetTclName());
      }
    this->GetWindow()->GetMainView()->RemoveComposite(this);
    pvApp->Script("%s Delete", this->GetTclName());
    }
  else
    {
    this->UpdateParameterWidgets();
    }
}

//---------------------------------------------------------------------------
void vtkPVSource::DeleteCallback()
{
  vtkPVApplication *pvApp = (vtkPVApplication*)this->Application;
  vtkPVSource *prev;
  int i;
  
  if (this->PVOutput == NULL)
    {
    // Accept button hasn't been clicked yet, so this is the same
    // functionality as hitting Cancel under these circumstances.
    this->CancelCallback();
    }
  else if (this->PVOutput->GetPVSourceUsers()->GetNumberOfItems() == 0)
    {
    // Need to remove the data connected with this source from the list of
    // inputs, but not sure how to do that since the PVData is NULL at this
    // point.
    
    for (i = 0; i < this->GetNumberOfInputs(); i++)
      {
      this->GetNthInput(i)->RemovePVSourceFromUsers(this);
      }
    
    // We need to unpack the notebook for this source and pack the one for the
    // source of this source (if there is one).

    prev = this->GetWindow()->GetPreviousSource();
    this->GetWindow()->SetCurrentSource(prev);
    if (prev)
      {
      prev->ShowProperties();
      }
    
    // We need to remove this source from the Source List.    
    this->GetWindow()->GetSourceList()->GetSources()->RemoveItem(this);
    this->GetWindow()->GetSourceList()->Update();    

    // Delete the source on the other processes.  Removing it from the
    // view's list of composites amounts to deleting the source on the
    // local processor because it's the last thing that has a reference to it.
    if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
      {
      pvApp->BroadcastScript("%s Delete", this->GetTclName());
      pvApp->BroadcastScript("%s Delete", this->GetVTKSourceTclName());
      }
    this->GetVTKSource()->Delete();
    this->PVOutput->GetActorComposite()->VisibilityOff();
    if (prev)
      {
      prev->GetPVData()->GetActorComposite()->VisibilityOn();
      }
    this->SetPVData(NULL);
    this->GetView()->Render();
    this->GetWindow()->GetMainView()->RemoveComposite(this);
    pvApp->Script("%s Delete", this->GetTclName());
    }
  else
    {
    // disable the delete button
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateParameterWidgets()
{
  int num, i;
  char *cmd;

  // Copy the ivars from the vtk object to the UI.
  num = this->CancelCommands->GetLength();
  for (i = 0; i < num; ++i)
    {
    cmd = this->CancelCommands->GetCommand(i);
    if (cmd)
      {
      this->Script(cmd);
      }
    } 
}


//----------------------------------------------------------------------------
void vtkPVSource::AcceptHelper(char *method, char *args)
{
}

//----------------------------------------------------------------------------
void vtkPVSource::AcceptHelper2(char *name, char *method, char *args)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  vtkDebugMacro("AcceptHelper2 " << name << ", " << method << ", " << args);

  pvApp->Script("%s %s %s", name, method, args);
  pvApp->BroadcastScript("%s %s %s", name,  method, args);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateNavigationCanvas()
{
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";
  char *result;
  int bbox[4], bboxOut[4];
  int xMid, yMid, y;
  char *tmp;
  vtkPVSource *source;
  vtkPVSourceCollection *outs, *moreOuts;
  vtkPVData *moreOut;
  int i;
  
  // Clear the canvas
  this->Script("%s delete all",
               this->NavigationCanvas->GetWidgetName());

  // Put the inputs in the canvas.
  if (this->Inputs)
    {
    y = 10;
    for (i = 0; i < this->NumberOfInputs; i++)
      {
      source = this->Inputs[i]->GetPVSource();
      if (source)
        {
        // Draw the name of the assembly.
        this->Script(
          "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
          this->NavigationCanvas->GetWidgetName(), 20, y,
          source->GetName(), font);
        
        result = this->Application->GetMainInterp()->result;
        tmp = new char[strlen(result)+1];
        strcpy(tmp,result);
        this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
                     this->NavigationCanvas->GetWidgetName(), tmp,
                     this->GetTclName(), source->GetTclName());
        
        // Get the bounding box for the name. We may need to highlight it.
        this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(),
                      tmp);
        delete [] tmp;
        tmp = NULL;
        result = this->Application->GetMainInterp()->result;
        sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
        if (i == 0)
          {
          // only want to set xMid and yMid once
          yMid = (int)(0.5 * (bbox[1]+bbox[3]));
          xMid = (int)(0.5 * (bbox[2]+120));
          }
        
        // Draw a line from input to source.
        if (y == 10)
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                       this->NavigationCanvas->GetWidgetName(), bbox[2], yMid,
                       125, yMid);
          }
        else
          {
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), xMid, yMid,
                       xMid, yMid+15);
          yMid += 15;
          this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                       this->NavigationCanvas->GetWidgetName(), bbox[2],
                       yMid, xMid, yMid);
          }
        
        if (source->GetInputs())
          {
          if (source->GetNthInput(0)->GetPVSource())
            {
            // Draw ellipsis indicating that this source has a source.
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 6, yMid, 8,
                         yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 10, yMid, 12,
                         yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(), 14, yMid, 16,
                         yMid);
            }
          }
        }
      y += 15;
      }
    }

  // Draw the name of the assembly.
  this->Script(
    "%s create text %d %d -text {%s} -font %s -anchor w -tags x",
    this->NavigationCanvas->GetWidgetName(), 130, 10, this->GetName(), font);
  result = this->Application->GetMainInterp()->result;
  tmp = new char[strlen(result)+1];
  strcpy(tmp,result);
  // Get the bounding box for the name. We may need to highlight it.
  this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(), tmp);
  delete [] tmp;
  tmp = NULL;
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
  yMid = (int)(0.5 * (bbox[1]+bbox[3]));
  xMid = (int)(0.5 * (bbox[2] + 245));

  // Put the outputs in the canvas.
  outs = NULL;
  if (this->PVOutput)
    {
    outs = this->PVOutput->GetPVSourceUsers();
    }
  if (outs)
    {
    outs->InitTraversal();
    y = 10;
    while ( (source = outs->GetNextPVSource()) )
      {
      // Draw the name of the assembly.
      this->Script(
         "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
         this->NavigationCanvas->GetWidgetName(), 250, y,
         source->GetName(), font);

      result = this->Application->GetMainInterp()->result;
      tmp = new char[strlen(result)+1];
      strcpy(tmp,result);
      this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
                   this->NavigationCanvas->GetWidgetName(), tmp,
	           this->GetTclName(), source->GetTclName());
      // Get the bounding box for the name. We may need to highlight it.
      this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(), tmp);
      delete [] tmp;
      tmp = NULL;
      result = this->Application->GetMainInterp()->result;
      sscanf(result, "%d %d %d %d", bboxOut, bboxOut+1, bboxOut+2, bboxOut+3);

      // Draw to output.
      if (y == 10)
        { // first is a special case (single line).
        this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                     this->NavigationCanvas->GetWidgetName(), bbox[2], yMid,
                     245, yMid);
        }
      else
        {
        this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
                     this->NavigationCanvas->GetWidgetName(), xMid, yMid, xMid,
                     yMid+15);
        yMid += 15;
        this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
                     this->NavigationCanvas->GetWidgetName(), xMid, yMid,
                     245, yMid);
        }
      if (moreOut = source->GetPVData())
        {
        if (moreOuts = moreOut->GetPVSourceUsers())
          {
          moreOuts->InitTraversal();
          if (moreOuts->GetNextPVSource())
            {
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(),
                         bboxOut[2]+10, yMid, bboxOut[2]+12, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(),
                         bboxOut[2]+14, yMid, bboxOut[2]+16, yMid);
            this->Script("%s create line %d %d %d %d",
                         this->NavigationCanvas->GetWidgetName(),
                         bboxOut[2]+18, yMid, bboxOut[2]+20, yMid);
            }
          }
        }
      y += 15;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::SelectSource(vtkPVSource *source)
{
  if (source)
    {
    this->GetWindow()->SetCurrentSource(source);
    source->ShowProperties();
    source->GetNotebook()->Raise(0);
    if (source->GetPVData() &&
        source->GetPVData()->GetPVSourceUsers()->GetNumberOfItems() > 0)
      {
      this->Script("%s configure -state disabled",
                   source->GetDeleteButton()->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -state normal",
                   source->GetDeleteButton()->GetWidgetName());
      }
    }
}

typedef vtkPVData *vtkPVDataPointer;
//---------------------------------------------------------------------------
void vtkPVSource::SetNumberOfInputs(int num)
{
  int idx;
  vtkPVDataPointer *inputs;

  // in case nothing has changed.
  if (num == this->NumberOfInputs)
    {
    return;
    }
  
  // Allocate new arrays.
  inputs = new vtkPVDataPointer[num];

  // Initialize with NULLs.
  for (idx = 0; idx < num; ++idx)
    {
    inputs[idx] = NULL;
    }

  // Copy old inputs
  for (idx = 0; idx < num && idx < this->NumberOfInputs; ++idx)
    {
    inputs[idx] = this->Inputs[idx];
    }
  
  // delete the previous arrays
  if (this->Inputs)
    {
    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    }
  
  // Set the new array
  this->Inputs = inputs;
  
  this->NumberOfInputs = num;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::SetNthInput(int idx, vtkPVData *input)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  // Handle parallelism.
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   input->GetTclName());
    }
  
  if (idx < 0)
    {
    vtkErrorMacro(<< "SetNthInput: " << idx << ", cannot set input. ");
    return;
    }
  // Expand array if necessary.
  if (idx >= this->NumberOfInputs)
    {
    this->SetNumberOfInputs(idx + 1);
    }
  
  // does this change anything?
  if (input == this->Inputs[idx])
    {
    return;
    }
  
  if (this->Inputs[idx])
    {
    this->Inputs[idx]->RemovePVSourceFromUsers(this);
    this->Inputs[idx]->UnRegister(this);
    this->Inputs[idx] = NULL;
    }
  else
    {
    this->Inputs[idx] = vtkPVData::New();
    }
  
  if (input)
    {
    input->Register(this);
    }

  this->Inputs[idx] = input;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::AddInput(vtkPVData *input)
{
  int idx;
  
  if (input)
    {
    input->Register(this);
    }
  this->Modified();
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == NULL)
      {
      this->Inputs[idx] = input;
      return;
      }
    }
  
  this->SetNumberOfInputs(this->NumberOfInputs + 1);
  this->Inputs[this->NumberOfInputs - 1] = input;
}

//---------------------------------------------------------------------------
void vtkPVSource::RemoveInput(vtkPVData *input)
{
  int idx, loc;
  
  if (!input)
    {
    return;
    }
  
  // find the input in the list of inputs
  loc = -1;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == input)
      {
      loc = idx;
      }
    }
  if (loc == -1)
    {
    vtkDebugMacro("tried to remove an input that was not in the list");
    return;
    }
  
  this->Inputs[loc]->UnRegister(this);
  this->Inputs[loc] = NULL;

  // if that was the last input, then shrink the list
  if (loc == this->NumberOfInputs - 1)
    {
    this->SetNumberOfInputs(this->NumberOfInputs - 1);
    }
  
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkPVSource::SqueezeInputArray()
{
  int idx, loc;
  
  // move NULL entries to the end
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] == NULL)
      {
      for (loc = idx+1; loc < this->NumberOfInputs; loc++)
        {
        this->Inputs[loc-1] = this->Inputs[loc];
        }
      this->Inputs[this->NumberOfInputs -1] = NULL;
      }
    }

  // adjust the size of the array
  loc = -1;
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (loc == -1 && this->Inputs[idx] == NULL)
      {
      loc = idx;
      }
    }
  if (loc > 0)
    {
    this->SetNumberOfInputs(loc);
    }
}

//---------------------------------------------------------------------------
void vtkPVSource::RemoveAllInputs()
{
  if ( this->Inputs )
    {
    for (int idx = 0; idx < this->NumberOfInputs; ++idx)
      {
      if ( this->Inputs[idx] )
        {
        this->Inputs[idx]->UnRegister(this);
        this->Inputs[idx] = NULL;
        }
      }

    delete [] this->Inputs;
    this->Inputs = NULL;
    this->NumberOfInputs = 0;
    this->Modified();
    }
}

//---------------------------------------------------------------------------
vtkPVData *vtkPVSource::GetNthInput(int idx)
{
  if (idx >= this->NumberOfInputs)
    {
    return NULL;
    }
  
  return (vtkPVData *)(this->Inputs[idx]);
}
