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
  
  this->Input = NULL;
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
  
  this->Widgets = vtkKWWidgetCollection::New();
  this->LastSelectionList = NULL;
  
  this->AcceptCommands = vtkPVCommandList::New();
  this->CancelCommands = vtkPVCommandList::New();
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  if (this->PVOutput)
    {
    this->PVOutput->UnRegister(this);
    this->PVOutput = NULL;
    }
  
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }

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
  
  if (vtkSource)
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

  me->GetWindow()->GetProgressGauge()->SetValue((int)(vtkSource->GetProgress() * 100));
}
//----------------------------------------------------------------------------
void vtkPVSourceEndProgress(void *arg)
{
  vtkPVSource *me = (vtkPVSource*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
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
  if (this->GetPVData() == NULL)
    { // This is the first time, initialize data.  
    vtkPVData *input;
    vtkPVActorComposite *ac;
    
    input = this->GetInput();
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
  if (this->PVOutput == NULL)
    { // Accept has not been called yet.  Delete the object.
    // ???
    this->GetWindow()->SetCurrentSource(NULL);
    this->GetWindow()->GetMainView()->RemoveComposite(this);
    // How do we delete the sources in all processes ???
    // this->Delete();
    }
  else
    {
    this->UpdateParameterWidgets();
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
  pvApp->Script("%s %s %s", name, method, args);

  vtkDebugMacro(<< name << " " << method << " " << args);

  pvApp->BroadcastScript("%s %s %s", name, this->GetTclName(), method, args);
}

//----------------------------------------------------------------------------
void vtkPVSource::UpdateNavigationCanvas()
{  
  static char *font = "-adobe-helvetica-medium-r-normal-*-14-100-100-100-p-76-iso8859-1";
  char *result;
  int bbox[4];
  int xMid, yMid, y;
  char *tmp;
  vtkPVSource *source;
  vtkPVSourceCollection *outs;
  
  // Clear the canvas
  this->Script("%s delete all",
               this->NavigationCanvas->GetWidgetName());

  // Put the inputs in the canvas.
  if (this->Input)
    {
    source = this->Input->GetPVSource();
    if (source)
      {
      // Draw the name of the assembly.
      this->Script(
         "%s create text %d %d -text {%s} -font %s -anchor w -tags x -fill blue",
         this->NavigationCanvas->GetWidgetName(), 10, 10, source->GetName(), font);

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
      sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
      yMid = 0.5 * (bbox[1]+bbox[3]);

      // Draw a line from input to source.
      this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
              this->NavigationCanvas->GetWidgetName(), bbox[2], yMid, 115, yMid);

      }
    }

  // Draw the name of the assembly.
  this->Script(
    "%s create text %d %d -text {%s} -font %s -anchor w -tags x",
    this->NavigationCanvas->GetWidgetName(), 120, 10, this->GetName(), font);
  result = this->Application->GetMainInterp()->result;
  tmp = new char[strlen(result)+1];
  strcpy(tmp,result);
  // Get the bounding box for the name. We may need to highlight it.
  this->Script( "%s bbox %s",this->NavigationCanvas->GetWidgetName(), tmp);
  delete [] tmp;
  tmp = NULL;
  result = this->Application->GetMainInterp()->result;
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
  yMid = 0.5 * (bbox[1]+bbox[3]);
  xMid = 0.5 * (bbox[2] + 235);

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
         this->NavigationCanvas->GetWidgetName(), 240, y, source->GetName(), font);

      result = this->Application->GetMainInterp()->result;
      tmp = new char[strlen(result)+1];
      strcpy(tmp,result);
      this->Script("%s bind %s <ButtonPress-1> {%s SelectSource %s}",
                   this->NavigationCanvas->GetWidgetName(), tmp,
	           this->GetTclName(), source->GetTclName());

      // Draw to output.
      if (y == 10)
        { // first is a special case (single line).
        this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
            this->NavigationCanvas->GetWidgetName(), bbox[2], yMid, 235, yMid);
        }
      else
        {
        this->Script("%s create line %d %d %d %d -fill gray50 -arrow none",
            this->NavigationCanvas->GetWidgetName(), xMid, yMid, xMid, yMid+15);
        yMid += 15;
        this->Script("%s create line %d %d %d %d -fill gray50 -arrow last",
            this->NavigationCanvas->GetWidgetName(), xMid, yMid, 235, yMid);
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
    }
}

