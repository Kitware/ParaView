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

  this->Properties = vtkKWWidget::New();
  
  this->DataCreated = 0;

  this->NavigationFrame = vtkKWLabeledFrame::New();
  this->ParameterFrame = vtkKWLabeledFrame::New();
  this->AcceptButton = vtkKWPushButton::New();
  
  this->Widgets = vtkKWWidgetCollection::New();
  
  this->NumberOfAcceptCommands = 0;
  this->AcceptCommandArrayLength = 0;
  this->AcceptCommands = NULL;
  this->LastSelectionList = NULL;
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

  this->ParameterFrame->Delete();
  this->ParameterFrame = NULL;

  this->Widgets->Delete();
  this->Widgets = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;  
  
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    this->LastSelectionList = NULL;
    }

  this->DeleteAcceptCommands();
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

  this->ParameterFrame->SetParent(this->Properties);
  this->ParameterFrame->Create(this->Application);
  this->ParameterFrame->SetLabel("Parameters");
  this->Script("pack %s -fill x -expand t -side top", this->ParameterFrame->GetWidgetName());

  this->AcceptButton->SetParent(this->ParameterFrame->GetFrame());
  this->AcceptButton->Create(this->Application, "-text Accept");
  this->AcceptButton->SetCommand(this, "AcceptCallback");
  this->Script("pack %s", this->AcceptButton->GetWidgetName());

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
void vtkPVSource::AddLabeledEntry(char *label, char *setCmd, char *getCmd)
{
  vtkKWLabeledEntry *entry = vtkKWLabeledEntry::New();
  this->Widgets->AddItem(entry);
  entry->SetParent(this->ParameterFrame->GetFrame());
  entry->Create(this->Application);
  entry->SetLabel(label);

  // Get initial value from the vtk source.
  this->Script("%s SetValue [[%s GetVTKSource] %s]", 
               entry->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s [%s GetValue]",
                          this->GetTclName(), setCmd, entry->GetTclName()); 

  this->Script("pack %s", entry->GetWidgetName());
  entry->Delete();
}  
//----------------------------------------------------------------------------
void vtkPVSource::AddLabeledToggle(char *label, char *setCmd, char *getCmd)
{
  // First a frame to hold the other widgets.
  vtkKWWidget *frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    vtkKWLabel *labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
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

  // Get initial value from the vtk source.
  this->Script("%s SetState [[%s GetVTKSource] %s]", 
               check->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s [%s GetState]",
                          this->GetTclName(), setCmd, check->GetTclName()); 

  this->Script("pack %s -side left", check->GetWidgetName());

  frame->Delete();
  check->Delete();
}  

//----------------------------------------------------------------------------
void vtkPVSource::AddVector2Entry(char *label, char *l1, char *l2,
				  char *setCmd, char *getCmd)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *minEntry, *maxEntry;

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {  
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
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
  this->Script("pack %s -side left", minEntry->GetWidgetName());

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
  this->Script("pack %s -side left", maxEntry->GetWidgetName());

  // Get initial value from the vtk source.
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 0]", 
               minEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 1]", 
               maxEntry->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s \"[%s GetValue] [%s GetValue]\"",
                          this->GetTclName(), setCmd, minEntry->GetTclName(),
                          maxEntry->GetTclName());

  frame->Delete();
  minEntry->Delete();
  maxEntry->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSource::AddVector3Entry(char *label, char *l1, char *l2, char *l3,
				  char *setCmd, char *getCmd)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *xEntry, *yEntry, *zEntry;

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
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
  this->Script("pack %s -side left", xEntry->GetWidgetName());

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
  this->Script("pack %s -side left", yEntry->GetWidgetName());

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
  this->Script("pack %s -side left", zEntry->GetWidgetName());

  // Get initial value from the vtk source.
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 0]", 
               xEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 1]", 
               yEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 2]", 
               zEntry->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s \"[%s GetValue] [%s GetValue] [%s GetValue]\"",
                          this->GetTclName(), setCmd, xEntry->GetTclName(),
                          yEntry->GetTclName(), zEntry->GetTclName());

  frame->Delete();
  xEntry->Delete();
  yEntry->Delete();
  zEntry->Delete();
}


//----------------------------------------------------------------------------
void vtkPVSource::AddVector4Entry(char *label, char *l1, char *l2, char *l3,
				  char *l4, char *setCmd, char *getCmd)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry *xEntry, *yEntry, *zEntry, *wEntry;

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
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
  this->Script("pack %s -side left", xEntry->GetWidgetName());

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
  this->Script("pack %s -side left", yEntry->GetWidgetName());

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
  this->Script("pack %s -side left", zEntry->GetWidgetName());

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
  this->Script("pack %s -side left", wEntry->GetWidgetName());

  // Get initial value from the vtk source.
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 0]", 
               xEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 1]", 
               yEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 2]", 
               zEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 3]", 
               wEntry->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s \"[%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue]\"",
                          this->GetTclName(), setCmd, xEntry->GetTclName(),
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
				  char *setCmd, char *getCmd)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWEntry  *uEntry, *vEntry, *wEntry, *xEntry, *yEntry, *zEntry;

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s", frame->GetWidgetName());

  // Now a label
  if (label && label[0] != '\0')
    {
    labelWidget = vtkKWLabel::New();
    this->Widgets->AddItem(labelWidget);
    labelWidget->SetParent(frame);
    labelWidget->Create(this->Application, "");
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
  this->Script("pack %s -side left", uEntry->GetWidgetName());

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
  this->Script("pack %s -side left", vEntry->GetWidgetName());

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
  this->Script("pack %s -side left", wEntry->GetWidgetName());

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
  this->Script("pack %s -side left", xEntry->GetWidgetName());

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
  this->Script("pack %s -side left", yEntry->GetWidgetName());

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
  this->Script("pack %s -side left", zEntry->GetWidgetName());

  // Get initial value from the vtk source.
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 0]", 
               uEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 1]", 
               vEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 2]", 
               wEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 3]", 
               xEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 4]", 
               yEntry->GetTclName(), this->GetTclName(), getCmd); 
  this->Script("%s SetValue [lindex [[%s GetVTKSource] %s] 5]", 
               zEntry->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s \"[%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue] [%s GetValue]\"",
			 this->GetTclName(), setCmd, uEntry->GetTclName(), 
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
void vtkPVSource::AddScale(char *label, char *setCmd, char *getCmd,
			   float min, float max, float resolution)
{
  vtkKWWidget *frame;
  vtkKWLabel *labelWidget;
  vtkKWScale *slider;

  // First a frame to hold the other widgets.
  frame = vtkKWWidget::New();
  this->Widgets->AddItem(frame);
  frame->SetParent(this->ParameterFrame->GetFrame());
  frame->Create(this->Application, "frame", "");
  this->Script("pack %s", frame->GetWidgetName());

  // Now a label
  labelWidget = vtkKWLabel::New();
  this->Widgets->AddItem(labelWidget);
  labelWidget->SetParent(frame);
  labelWidget->Create(this->Application, "");
  labelWidget->SetLabel(label);
  this->Script("pack %s -side left", labelWidget->GetWidgetName());

  slider = vtkKWScale::New();
  this->Widgets->AddItem(slider);
  slider->SetParent(frame);
  slider->Create(this->Application, "-showvalue 1");
  slider->SetRange(min, max);
  slider->SetResolution(resolution);
  this->Script("pack %s -side left", slider->GetWidgetName());

  // Get initial value from the vtk source.
  this->Script("%s SetValue [[%s GetVTKSource] %s]", 
               slider->GetTclName(), this->GetTclName(), getCmd); 

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s [%s GetValue]",
			 this->GetTclName(), setCmd, slider->GetTclName());

  frame->Delete();
  labelWidget->Delete();
  slider->Delete();
}


//----------------------------------------------------------------------------
void vtkPVSource::AddModeList(char *label, char *setCmd, char *getCmd)
{
  vtkPVSelectionList *sl = vtkPVSelectionList::New();
  
  sl->SetLabel(label);
  this->Widgets->AddItem(sl);
  sl->SetParent(this->ParameterFrame->GetFrame());
  sl->Create(this->Application);  
  this->Script("pack %s -side left", sl->GetWidgetName());
    
  this->Script("%s SetValue [[%s GetVTKSource] %s]",
	       sl->GetTclName(), this->GetTclName(), getCmd);
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AddAcceptCommand("%s AcceptHelper %s [%s GetValue]",
			 this->GetTclName(), setCmd, sl->GetTclName());
  
  // Save this selection list so the user can add items to it.
  if (this->LastSelectionList)
    {
    this->LastSelectionList->UnRegister(this);
    }
  sl->Register(this);
  this->LastSelectionList = sl;

  sl->Delete();
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
  for (i = 0; i < this->NumberOfAcceptCommands; ++i)
    {
    this->Script(this->AcceptCommands[i]);
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
  this->GetView()->Render();
  window->GetSourceList()->Update();
}


//----------------------------------------------------------------------------
void vtkPVSource::AcceptHelper(char *method, char *args)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->Script("[%s GetVTKSource] %s %s", this->GetTclName(),
                method, args);

  vtkDebugMacro("[" << this->GetTclName() << " GetVTKSource] " 
                << method << " " << args);


  pvApp->BroadcastScript("[%s GetVTKSource] %s %s", this->GetTclName(),
                         method, args);
}

//----------------------------------------------------------------------------
void vtkPVSource::AddAcceptCommand(const char *format, ...)
{
  static char event[16000];

  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  // Check to see if we need to extent to array of commands.
  if (this->AcceptCommandArrayLength <= this->NumberOfAcceptCommands)
    { // Yes.
    int i;
    // Allocate a new array
    this->AcceptCommandArrayLength += 20;
    char **tmp = new char* [this->AcceptCommandArrayLength];
    // Copy array elements.
    for (i = 0; i < this->NumberOfAcceptCommands; ++i)
      {
      tmp[i] = this->AcceptCommands[i];
      }
    // Delete the old array.
    if (this->AcceptCommands)
      {
      delete [] this->AcceptCommands;
      this->AcceptCommands = NULL;
      }
    // Set the new array.
    this->AcceptCommands = tmp;
    tmp = NULL;
    }

  // Allocate the string for and set the new command.
  this->AcceptCommands[this->NumberOfAcceptCommands] 
              = new char[strlen(event) + 2];
  strcpy(this->AcceptCommands[this->NumberOfAcceptCommands], event);
  this->NumberOfAcceptCommands += 1;
}

//----------------------------------------------------------------------------
void vtkPVSource::DeleteAcceptCommands()
{
  int i;

  for (i = 0; i < this->NumberOfAcceptCommands; ++i)
    {
    delete [] this->AcceptCommands[i];
    this->AcceptCommands[i] = NULL;
    }
  delete [] this->AcceptCommands;
  this->AcceptCommands = NULL;
  this->NumberOfAcceptCommands = 0;
  this->AcceptCommandArrayLength = 0;
}

