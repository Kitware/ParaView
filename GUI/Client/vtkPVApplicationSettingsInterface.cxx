/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplicationSettingsInterface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVApplicationSettingsInterface.h"

#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSourceNotebook.h"
#include "vtkPVGUIClientOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkPVApplicationSettingsInterface, "1.18");

int vtkPVApplicationSettingsInterfaceCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::vtkPVApplicationSettingsInterface()
{
  // Interface settings

  this->AutoAccept = 0;
  this->AutoAcceptCheckButton = 0;
  this->ShowSourcesDescriptionCheckButton = 0;
  this->ShowSourcesNameCheckButton = 0;
  this->ShowTraceFilesCheckButton = 0;
}

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::~vtkPVApplicationSettingsInterface()
{
  this->SetWindow(NULL);

  // Interface settings

  if (this->AutoAcceptCheckButton)
    {
    this->AutoAcceptCheckButton->Delete();
    this->AutoAcceptCheckButton = NULL;
    }

  if (this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton->Delete();
    this->ShowSourcesDescriptionCheckButton = NULL;
    }

  if (this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton->Delete();
    this->ShowSourcesNameCheckButton = NULL;
    }
  if (this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton->Delete();
    this->ShowTraceFilesCheckButton = NULL;
    }
}

// ---------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create(app);

  ostrstream tk_cmd;
  vtkKWWidget *frame;

  // --------------------------------------------------------------
  // Interface settings : continuing...

  frame = this->InterfaceSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Interface settings : auto accept

  if (!this->AutoAcceptCheckButton)
    {
    this->AutoAcceptCheckButton = vtkKWCheckButton::New();
    }
  this->AutoAcceptCheckButton->SetParent(frame);
  this->AutoAcceptCheckButton->Create(app, 0);
  this->AutoAcceptCheckButton->SetText("AutoAccept");
  this->AutoAcceptCheckButton->SetCommand(
    this, "AutoAcceptCallback");
  this->AutoAcceptCheckButton->SetBalloonHelpString(
    "Switch between manual accept and auto accept. "
    "In auto accept mode filters automatically update "
    "when a parameters is changed."
    " This option is also available in the accept button pulldown menu.");
  this->AutoAcceptCheckButton->SetState(this->AutoAccept);

  tk_cmd << "pack " << this->AutoAcceptCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : show source descriptions

  if (!this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesDescriptionCheckButton->SetParent(frame);
  this->ShowSourcesDescriptionCheckButton->Create(app, 0);
  this->ShowSourcesDescriptionCheckButton->SetText("Show source descriptions");
  this->ShowSourcesDescriptionCheckButton->SetCommand(
    this, "ShowSourcesDescriptionCallback");
  this->ShowSourcesDescriptionCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the source descriptions "
    "are shown in the parameters page.");

  tk_cmd << "pack " << this->ShowSourcesDescriptionCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : show sources name

  if (!this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesNameCheckButton->SetParent(frame);
  this->ShowSourcesNameCheckButton->Create(app, 0);
  this->ShowSourcesNameCheckButton->SetText(
    "Show source names in browsers");
  this->ShowSourcesNameCheckButton->SetCommand(
    this, "ShowSourcesNameCallback");
  this->ShowSourcesNameCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the unique source names "
    "are shown in the source browsers. This name is normally useful "
    "only to script developers.");

  tk_cmd << "pack " << this->ShowSourcesNameCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  if (!this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton = vtkKWCheckButton::New();
    }

  this->ShowTraceFilesCheckButton->SetParent(frame);
  this->ShowTraceFilesCheckButton->Create(app, 0);
  this->ShowTraceFilesCheckButton->SetText(
    "Show trace files on ParaView startup");
  this->ShowTraceFilesCheckButton->SetCommand(
    this, "ShowTraceFilesCallback");
  this->ShowTraceFilesCheckButton->SetBalloonHelpString(
    "When this advanced option is on, tracefiles will be detected and "
    "reported during startup. Turn this off to avoid unnecessary popup "
    "messages during startup.");

  if (!app->GetRegisteryValue(2,"RunTime", 
      VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY,0)||
    app->GetIntRegisteryValue(2,"RunTime",VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY))
    {
    this->ShowTraceFilesCheckButton->SetState(1);
    }
  else
    {
    this->ShowTraceFilesCheckButton->SetState(0);
    }

  tk_cmd << "pack " << this->ShowTraceFilesCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;


  // --------------------------------------------------------------
  // Interface settings : show most recent panels

  // Not really supported by ParaView... (only in App Settings notebook)

  tk_cmd << "pack forget " 
         << this->ShowMostRecentPanelsCheckButton->GetWidgetName() << endl;

  // --------------------------------------------------------------
  // Interface customization

  // Not really supported by ParaView... (only in App Settings notebook)

  tk_cmd << "pack forget " 
         << this->InterfaceCustomizationFrame->GetWidgetName() << endl;

  // --------------------------------------------------------------
  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Update according to the current Window

  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::Update()
{
  this->Superclass::Update();

  if (!this->IsCreated() || !this->Window)
    {
    return;
    }

  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());

  // Interface settings : show sources description

  if (this->ShowSourcesDescriptionCheckButton && app)
    {
    this->ShowSourcesDescriptionCheckButton->SetState(
      app->GetShowSourcesLongHelp());
    }

  // Interface settings : show sources name

  if (this->ShowSourcesNameCheckButton && app)
    {
    this->ShowSourcesNameCheckButton->SetState(
      app->GetSourcesBrowserAlwaysShowName());
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::AutoAcceptCallback()
{
 if (!this->AutoAcceptCheckButton ||
     !this->AutoAcceptCheckButton->IsCreated())
   {
   return;
   }

 int flag = this->AutoAcceptCheckButton->GetState() ? 1 : 0;
 this->SetAutoAccept(flag);
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetAutoAccept(int val)
{
  if (this->AutoAccept == val)
    {
    return;
    }
  this->AutoAccept = val;
  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
  if (app)
    {
    app->GetMainView()->GetSourceNotebook()->SetAutoAccept(val);
    }

  if (!this->AutoAcceptCheckButton ||
       !this->AutoAcceptCheckButton->IsCreated())
    {
    return;
    }
  this->AutoAcceptCheckButton->SetState(val);

  // Let the PVSource notebook handle registry.
  // This class does not even get created until the GUI is shown.
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesDescriptionCallback()
{
 if (!this->ShowSourcesDescriptionCheckButton ||
     !this->ShowSourcesDescriptionCheckButton->IsCreated())
   {
   return;
   }

 int flag = this->ShowSourcesDescriptionCheckButton->GetState() ? 1 : 0;

 this->GetApplication()->SetRegisteryValue(
   2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", flag);

 vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
 if (app)
   {
   app->SetShowSourcesLongHelp(flag);
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowTraceFilesCallback()
{
  if (!this->ShowTraceFilesCheckButton ||
    !this->ShowTraceFilesCheckButton->IsCreated())
    {
    return;
    }

  int flag = this->ShowTraceFilesCheckButton->GetState() ? 1 : 0;

  this->GetApplication()->SetRegisteryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY, "%d", flag);

  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
  if (app)
    {
    app->SetSourcesBrowserAlwaysShowName(flag);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesNameCallback()
{
 if (!this->ShowSourcesNameCheckButton ||
     !this->ShowSourcesNameCheckButton->IsCreated())
   {
   return;
   }

 int flag = this->ShowSourcesNameCheckButton->GetState() ? 1 : 0;

 this->GetApplication()->SetRegisteryValue(
   2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", flag);

 vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
 if (app)
   {
   app->SetSourcesBrowserAlwaysShowName(flag);
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Interface settings

  if (this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoAccept: " << this->AutoAccept << endl;
}
