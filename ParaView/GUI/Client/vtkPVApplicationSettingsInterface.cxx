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

#include "vtkKWBalloonHelpManager.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSourceNotebook.h"
#include "vtkPVGUIClientOptions.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkPVApplicationSettingsInterface, "1.33");

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::vtkPVApplicationSettingsInterface()
{
  // Interface settings

  this->AutoAccept = 0;
  this->AutoAcceptCheckButton = 0;
  this->ShowSourcesDescriptionCheckButton = 0;
  this->ShowSourcesNameCheckButton = 0;
  this->ShowTraceFilesCheckButton = 0;
  this->CreateLogFilesCheckButton = 0;
  this->ResetButton = 0;
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
  if (this->CreateLogFilesCheckButton)
    {
    this->CreateLogFilesCheckButton->Delete();
    this->CreateLogFilesCheckButton = NULL;
    }
  if (this->ResetButton)
    {
    this->ResetButton->Delete();
    this->ResetButton = NULL;
    }
}

// ---------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create();

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
  this->AutoAcceptCheckButton->Create();
  this->AutoAcceptCheckButton->SetText("AutoAccept");
  this->AutoAcceptCheckButton->SetCommand(
    this, "AutoAcceptCallback");
  this->AutoAcceptCheckButton->SetBalloonHelpString(
    "Switch between manual accept and auto accept. "
    "In auto accept mode filters automatically update "
    "when a parameters is changed."
    " This option is also available in the accept button pulldown menu.");
  this->AutoAcceptCheckButton->SetSelectedState(this->AutoAccept);

  tk_cmd << "pack " << this->AutoAcceptCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : show source descriptions

  if (!this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesDescriptionCheckButton->SetParent(frame);
  this->ShowSourcesDescriptionCheckButton->Create();
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
  this->ShowSourcesNameCheckButton->Create();
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
  this->ShowTraceFilesCheckButton->Create();
  this->ShowTraceFilesCheckButton->SetText(
    "Show trace files on ParaView startup");
  this->ShowTraceFilesCheckButton->SetCommand(
    this, "ShowTraceFilesCallback");
  this->ShowTraceFilesCheckButton->SetBalloonHelpString(
    "When this advanced option is on, tracefiles will be detected and "
    "reported during startup. Turn this off to avoid unnecessary popup "
    "messages during startup.");

  vtkKWApplication *app = this->GetApplication();
  if (!app->GetRegistryValue(2,"RunTime", 
      VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY,0)||
    app->GetIntRegistryValue(2,"RunTime",VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY))
    {
    this->ShowTraceFilesCheckButton->SetSelectedState(1);
    }
  else
    {
    this->ShowTraceFilesCheckButton->SetSelectedState(0);
    }

  tk_cmd << "pack " << this->ShowTraceFilesCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : create log files

  if (!this->CreateLogFilesCheckButton)
    {
    this->CreateLogFilesCheckButton = vtkKWCheckButton::New();
    }

  this->CreateLogFilesCheckButton->SetParent(frame);
  this->CreateLogFilesCheckButton->Create();
  this->CreateLogFilesCheckButton->SetText(
    "Create per node log files on ParaView startup");
  this->CreateLogFilesCheckButton->SetCommand(
    this, "CreateLogFilesCallback");
  this->CreateLogFilesCheckButton->SetBalloonHelpString(
    "When this option is on, a log file will be created per server node "
    "to record information about activity on that node.");

  if (app->GetIntRegistryValue(2,"RunTime",
                               VTK_PV_ASI_CREATE_LOG_FILES_REG_KEY))
    {
    this->CreateLogFilesCheckButton->SetSelectedState(1);
    }
  else
    {
    this->CreateLogFilesCheckButton->SetSelectedState(0);
    }

  tk_cmd << "pack " << this->CreateLogFilesCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface customization

  // Not really supported by ParaView... (only in App Settings notebook)

  tk_cmd << "pack forget " 
         << this->InterfaceCustomizationFrame->GetWidgetName() << endl;


  if (!this->ResetButton)
    {
    this->ResetButton = vtkKWPushButton::New();
    this->ResetButton->SetParent(frame);
    }
  this->ResetButton->Create();
  this->ResetButton->SetText("Reset");
  this->ResetButton->SetWidth(20);
  this->ResetButton->SetBalloonHelpString("Reset Interface Setting to default.");
  this->ResetButton->SetCommand(this, "ResetSettingsToDefault");
  tk_cmd << "pack " <<  this->ResetButton->GetWidgetName() 
    << " -side top -anchor e -expand no -fill none -padx 2 -pady 10"
    << endl;
  

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
    this->ShowSourcesDescriptionCheckButton->SetSelectedState(
      app->GetShowSourcesLongHelp());
    }

  // Interface settings : show sources name

  if (this->ShowSourcesNameCheckButton && app)
    {
    this->ShowSourcesNameCheckButton->SetSelectedState(
      app->GetSourcesBrowserAlwaysShowName());
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ResetSettingsToDefault()
{
  this->GetApplication()->SetSaveUserInterfaceGeometry(1);
  this->GetApplication()->SetSplashScreenVisibility(1);
  this->GetApplication()->GetBalloonHelpManager()->SetVisibility(1);
  this->Window->SetViewPanelPositionToRight();
  this->ConfirmExitCallback(1);
  
  this->SetAutoAccept(0);
  this->ShowSourcesDescriptionCallback(1);
  this->ShowSourcesNameCallback(0);
  this->ShowTraceFilesCallback(1);
  this->CreateLogFilesCallback(0);

  this->ShowSourcesDescriptionCheckButton->SetSelectedState(1);
  this->ShowSourcesNameCheckButton->SetSelectedState(0);
  this->ShowTraceFilesCheckButton->SetSelectedState(1);
  this->CreateLogFilesCheckButton->SetSelectedState(0);

  // Now update the GUI for the Application settings.
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::AutoAcceptCallback(int state)
{
 this->SetAutoAccept(state);
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
  this->AutoAcceptCheckButton->SetSelectedState(val);

  // Let the PVSource notebook handle registry.
  // This class does not even get created until the GUI is shown.
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesDescriptionCallback(int state)
{
 this->GetApplication()->SetRegistryValue(
   2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", state);

 vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
 if (app)
   {
   app->SetShowSourcesLongHelp(state);
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowTraceFilesCallback(int state)
{
  this->GetApplication()->SetRegistryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY, "%d", state);
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::CreateLogFilesCallback(int state)
{
  this->GetApplication()->SetRegistryValue(
    2, "RunTime", VTK_PV_ASI_CREATE_LOG_FILES_REG_KEY, "%d", state);
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesNameCallback(int state)
{
 this->GetApplication()->SetRegistryValue(
   2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", state);

 vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->GetApplication());
 if (app)
   {
   app->SetSourcesBrowserAlwaysShowName(state);
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Interface settings

  if (this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoAccept: " << this->AutoAccept << endl;
}
