/*=========================================================================

  Module:    vtkKWApplicationSettingsInterface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWApplicationSettingsInterface.h"

#include "vtkKWApplication.h"
#include "vtkKWBalloonHelpManager.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWOptionMenuLabeled.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

#define VTK_KW_APPLICATION_SETTINGS_UIP_LABEL "Application Settings"
#define VTK_KW_APPLICATION_SETTINGS_DPI_FORMAT "%.1lf"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkKWApplicationSettingsInterface, "1.34");

int vtkKWApplicationSettingsInterfaceCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface::vtkKWApplicationSettingsInterface()
{
  this->SetName(VTK_KW_APPLICATION_SETTINGS_UIP_LABEL);

  this->Window = 0;

  // Interface settings

  this->InterfaceSettingsFrame = 0;
  this->ConfirmExitCheckButton = 0;
  this->SaveUserInterfaceGeometryCheckButton = 0;
  this->ShowSplashScreenCheckButton = 0;
  this->ShowBalloonHelpCheckButton = 0;

  // Interface customization

  this->InterfaceCustomizationFrame = 0;
  this->ResetDragAndDropButton = 0;

  // Toolbar settings

  this->ToolbarSettingsFrame = 0;
  this->FlatFrameCheckButton = 0;
  this->FlatButtonsCheckButton = 0;

  // Print settings

  this->PrintSettingsFrame = 0;
  this->DPIOptionMenu = 0;
}

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface::~vtkKWApplicationSettingsInterface()
{
  this->SetWindow(NULL);

  // Interface settings

  if (this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame->Delete();
    this->InterfaceSettingsFrame = NULL;
    }

  if (this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton->Delete();
    this->ConfirmExitCheckButton = NULL;
    }

  if (this->SaveUserInterfaceGeometryCheckButton)
    {
    this->SaveUserInterfaceGeometryCheckButton->Delete();
    this->SaveUserInterfaceGeometryCheckButton = NULL;
    }

  if (this->ShowSplashScreenCheckButton)
    {
    this->ShowSplashScreenCheckButton->Delete();
    this->ShowSplashScreenCheckButton = NULL;
    }

  if (this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton->Delete();
    this->ShowBalloonHelpCheckButton = NULL;
    }

  // Interface customization

  if (this->InterfaceCustomizationFrame)
    {
    this->InterfaceCustomizationFrame->Delete();
    this->InterfaceCustomizationFrame = NULL;
    }

  if (this->ResetDragAndDropButton)
    {
    this->ResetDragAndDropButton->Delete();
    this->ResetDragAndDropButton = NULL;
    }

  // Toolbar settings

  if (this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame->Delete();
    this->ToolbarSettingsFrame = NULL;
    }

  if (this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->Delete();
    this->FlatFrameCheckButton = NULL;
    }

  if (this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->Delete();
    this->FlatButtonsCheckButton = NULL;
    }

  // Print settings

  if (this->PrintSettingsFrame)
    {
    this->PrintSettingsFrame->Delete();
    this->PrintSettingsFrame = NULL;
    }

  if (this->DPIOptionMenu)
    {
    this->DPIOptionMenu->Delete();
    this->DPIOptionMenu = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SetWindow(vtkKWWindow *arg)
{
  if (this->Window == arg)
    {
    return;
    }
  this->Window = arg;
  this->Modified();

  this->Update();
}

// ---------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create(app);

  ostrstream tk_cmd;
  vtkKWWidget *page;
  vtkKWFrame *frame;

  // --------------------------------------------------------------
  // Add a "Preferences" page

  this->AddPage(this->GetName(), "Change the application settings");
  page = this->GetPageWidget(this->GetName());
  
  // --------------------------------------------------------------
  // Interface settings : main frame

  if (!this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame = vtkKWFrameLabeled::New();
    }

  this->InterfaceSettingsFrame->SetParent(this->GetPagesParentWidget());
  this->InterfaceSettingsFrame->ShowHideFrameOn();
  this->InterfaceSettingsFrame->Create(app, 0);
  this->InterfaceSettingsFrame->SetLabelText("Interface Settings");
    
  tk_cmd << "pack " << this->InterfaceSettingsFrame->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -padx 2 -pady 2 " 
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->InterfaceSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Interface settings : Confirm on exit ?

  if (!this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton = vtkKWCheckButton::New();
    }

  this->ConfirmExitCheckButton->SetParent(frame);
  this->ConfirmExitCheckButton->Create(app, 0);
  this->ConfirmExitCheckButton->SetText("Confirm on exit");
  this->ConfirmExitCheckButton->SetCommand(this, "ConfirmExitCallback");
  this->ConfirmExitCheckButton->SetBalloonHelpString(
    "A confirmation dialog will be presented to the user on exit.");

  tk_cmd << "pack " << this->ConfirmExitCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : Save application geometry on exit ?

  if (!this->SaveUserInterfaceGeometryCheckButton)
    {
    this->SaveUserInterfaceGeometryCheckButton = vtkKWCheckButton::New();
    }

  this->SaveUserInterfaceGeometryCheckButton->SetParent(frame);
  this->SaveUserInterfaceGeometryCheckButton->Create(app, 0);
  this->SaveUserInterfaceGeometryCheckButton->SetText(
    "Save user interface geometry on exit");
  this->SaveUserInterfaceGeometryCheckButton->SetCommand(
    this, "SaveUserInterfaceGeometryCallback");
  this->SaveUserInterfaceGeometryCheckButton->SetBalloonHelpString(
    "Save the user interface size and location on exit and restore it "
    "on startup.");

  tk_cmd << "pack " 
         << this->SaveUserInterfaceGeometryCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : Show splash screen ?

  if (app->GetHasSplashScreen())
    {
    if (!this->ShowSplashScreenCheckButton)
      {
      this->ShowSplashScreenCheckButton = vtkKWCheckButton::New();
      }

    this->ShowSplashScreenCheckButton->SetParent(frame);
    this->ShowSplashScreenCheckButton->Create(app, 0);
    this->ShowSplashScreenCheckButton->SetText("Show splash screen");
    this->ShowSplashScreenCheckButton->SetCommand(
      this, "ShowSplashScreenCallback");
    this->ShowSplashScreenCheckButton->SetBalloonHelpString(
      "Display the splash information screen at startup.");

    tk_cmd << "pack " << this->ShowSplashScreenCheckButton->GetWidgetName()
           << "  -side top -anchor w -expand no -fill none" << endl;
    }

  // --------------------------------------------------------------
  // Interface settings : Show balloon help ?

  if (!this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton = vtkKWCheckButton::New();
    }

  this->ShowBalloonHelpCheckButton->SetParent(frame);
  this->ShowBalloonHelpCheckButton->Create(app, 0);
  this->ShowBalloonHelpCheckButton->SetText("Show balloon help");
  this->ShowBalloonHelpCheckButton->SetCommand(
    this, "ShowBalloonHelpCallback");
  this->ShowBalloonHelpCheckButton->SetBalloonHelpString(
    "Display help in a yellow popup-box on the screen when you rest the "
    "mouse over an item that supports it.");

  tk_cmd << "pack " << this->ShowBalloonHelpCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface customization : main frame

  if (!this->InterfaceCustomizationFrame)
    {
    this->InterfaceCustomizationFrame = vtkKWFrameLabeled::New();
    }

  this->InterfaceCustomizationFrame->SetParent(this->GetPagesParentWidget());
  this->InterfaceCustomizationFrame->ShowHideFrameOn();
  this->InterfaceCustomizationFrame->Create(app, 0);
  this->InterfaceCustomizationFrame->SetLabelText("Interface Customization");
    
  tk_cmd << "pack " << this->InterfaceCustomizationFrame->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -padx 2 -pady 2 " 
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->InterfaceCustomizationFrame->GetFrame();

  // --------------------------------------------------------------
  // Interface customization : Drag & Drop : Reset

  if (!this->ResetDragAndDropButton)
    {
    this->ResetDragAndDropButton = vtkKWPushButton::New();
    }

  this->ResetDragAndDropButton->SetParent(frame);
  this->ResetDragAndDropButton->Create(app, 0);
  this->ResetDragAndDropButton->SetText("Reset Interface To Default State");
  this->ResetDragAndDropButton->SetCommand(this, "ResetDragAndDropCallback");
  this->ResetDragAndDropButton->SetBalloonHelpString(
    "You can drag & drop elements of the "
    "interface within the same panel or from one panel to the other. "
    "To do so, drag the title of a labeled frame to reposition it within "
    "a panel, or drop it on another tab to move it to a different panel."
    "Press this button to reset the placement of all user interface elements "
    "to their default position. You will need to restart the application for "
    "the interface to be reset.");

  tk_cmd << "pack " << this->ResetDragAndDropButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none -padx 22 -pady 2" 
         << endl;

  // --------------------------------------------------------------
  // Toolbar settings : main frame

  if (!this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame = vtkKWFrameLabeled::New();
    }

  this->ToolbarSettingsFrame->SetParent(this->GetPagesParentWidget());
  this->ToolbarSettingsFrame->ShowHideFrameOn();
  this->ToolbarSettingsFrame->Create(app, 0);
  this->ToolbarSettingsFrame->SetLabelText("Toolbar Settings");
    
  tk_cmd << "pack " << this->ToolbarSettingsFrame->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -padx 2 -pady 2 "  
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->ToolbarSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Toolbar settings : flat frame

  if (!this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton = vtkKWCheckButton::New();
    }

  this->FlatFrameCheckButton->SetParent(frame);
  this->FlatFrameCheckButton->Create(app, 0);
  this->FlatFrameCheckButton->SetText("Flat frame");
  this->FlatFrameCheckButton->SetCommand(this, "FlatFrameCallback");
  this->FlatFrameCheckButton->SetBalloonHelpString(
    "Display the toolbar frames using a flat aspect.");  

  tk_cmd << "pack " << this->FlatFrameCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Toolbar settings : flat buttons

  if (!this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton = vtkKWCheckButton::New();
    }

  this->FlatButtonsCheckButton->SetParent(frame);
  this->FlatButtonsCheckButton->Create(app, 0);
  this->FlatButtonsCheckButton->SetText("Flat buttons");
  this->FlatButtonsCheckButton->SetCommand(this, "FlatButtonsCallback");
  this->FlatButtonsCheckButton->SetBalloonHelpString(
    "Display the toolbar buttons using a flat aspect.");  
  
  tk_cmd << "pack " << this->FlatButtonsCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Print settings : main frame

  if (!this->PrintSettingsFrame)
    {
    this->PrintSettingsFrame = vtkKWFrameLabeled::New();
    }

  this->PrintSettingsFrame->SetParent(this->GetPagesParentWidget());
  this->PrintSettingsFrame->ShowHideFrameOn();
  this->PrintSettingsFrame->Create(app, 0);
  this->PrintSettingsFrame->SetLabelText("Print Settings");
    
  tk_cmd << "pack " << this->PrintSettingsFrame->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -padx 2 -pady 2 "  
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->PrintSettingsFrame->GetFrame();

  // Print settings : DPI

  if (!this->DPIOptionMenu)
    {
    this->DPIOptionMenu = vtkKWOptionMenuLabeled::New();
    }

  this->DPIOptionMenu->SetParent(frame);
  this->DPIOptionMenu->Create(app);

  this->DPIOptionMenu->GetLabel()->SetText("DPI:");

  double dpis[] = { 100.0, 150.0, 300.0, 600.0 };
  for (unsigned int i = 0; i < sizeof(dpis) / sizeof(double); i++)
    {
    char label[128], command[128];
    sprintf(command, "DPICallback %lf", dpis[i]);
    sprintf(label, VTK_KW_APPLICATION_SETTINGS_DPI_FORMAT, dpis[i]);
    this->DPIOptionMenu->GetWidget()->AddEntryWithCommand(
      label, this, command);
    }

  tk_cmd << "pack " << this->DPIOptionMenu->GetWidgetName()
         << " -side top -anchor w -padx 2 -pady 2" << endl;

  // --------------------------------------------------------------
  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::Update()
{
  this->Superclass::Update();

  if (!this->IsCreated() || !this->Window)
    {
    return;
    }

  // Interface settings : Confirm on exit ?

  if (this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton->SetState(
      vtkKWMessageDialog::RestoreMessageDialogResponseFromRegistry(
        this->GetApplication(), vtkKWApplication::ExitDialogName) ? 0 : 1);
    }

  // Interface settings : Save application geometry on exit ?

  if (this->SaveUserInterfaceGeometryCheckButton)
    {
    this->SaveUserInterfaceGeometryCheckButton->SetState(
      this->GetApplication()->GetSaveUserInterfaceGeometry());
    }
  
  // Interface settings : Show splash screen ?

  if (this->ShowSplashScreenCheckButton)
    {
    this->ShowSplashScreenCheckButton->SetState(
      this->GetApplication()->GetShowSplashScreen());
    }

  // Interface settings : Show balloon help ?

  if (this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton->SetState(
      this->GetApplication()->GetBalloonHelpManager()->GetShow());
    }

  // Interface customization : Drag & Drop : Enable

  vtkKWUserInterfaceNotebookManager *uim_nb = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(
      this->Window->GetUserInterfaceManager());
  if (!uim_nb)
    {
    this->ResetDragAndDropButton->SetEnabled(0);
    }

  // Toolbar settings : flat frame

  if (FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->SetState(vtkKWToolbar::GetGlobalFlatAspect());
    }

  // Toolbar settings : flat buttons

  if (FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->SetState(
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    }

  // If there is no toolbars, disable the interface

  if (!this->Window->GetToolbars() ||
      !this->Window->GetToolbars()->GetNumberOfToolbars())
    {
    if (this->FlatFrameCheckButton)
      {
      this->FlatFrameCheckButton->SetEnabled(0);
      }
    if (this->FlatButtonsCheckButton)
      {
      this->FlatButtonsCheckButton->SetEnabled(0);
      }
    }

  // Print settings

  if (this->DPIOptionMenu && this->DPIOptionMenu->GetWidget() && this->Window)
    {
    char buffer[128];
    sprintf(buffer, VTK_KW_APPLICATION_SETTINGS_DPI_FORMAT, 
            this->GetApplication()->GetPrintTargetDPI());
    this->DPIOptionMenu->GetWidget()->SetCurrentEntry(buffer);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ConfirmExitCallback()
{
  if (!this->ConfirmExitCheckButton || 
      !this->ConfirmExitCheckButton->IsCreated())
    {
    return;
    }

  vtkKWMessageDialog::SaveMessageDialogResponseToRegistry(
    this->GetApplication(),
    vtkKWApplication::ExitDialogName, 
    this->ConfirmExitCheckButton->GetState() ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SaveUserInterfaceGeometryCallback()
{
  if (!this->SaveUserInterfaceGeometryCheckButton || 
      !this->SaveUserInterfaceGeometryCheckButton->IsCreated())
    {
    return;
    }
  
  int state = this->SaveUserInterfaceGeometryCheckButton->GetState() ? 1 : 0;
  this->GetApplication()->SetSaveUserInterfaceGeometry(state);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ShowSplashScreenCallback()
{
  if (!this->ShowSplashScreenCheckButton ||
      !this->ShowSplashScreenCheckButton->IsCreated())
    {
    return;
    }

  int state = this->ShowSplashScreenCheckButton->GetState() ? 1 : 0;
  this->GetApplication()->SetShowSplashScreen(state);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ShowBalloonHelpCallback()
{
  if (!this->ShowBalloonHelpCheckButton ||
      !this->ShowBalloonHelpCheckButton->IsCreated())
    {
    return;
    }

  int state = this->ShowBalloonHelpCheckButton->GetState() ? 1 : 0;
  this->GetApplication()->GetBalloonHelpManager()->SetShow(state);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ResetDragAndDropCallback()
{
  if (!this->Window || !this->IsCreated())
    {
    return;
    }

  vtkKWMessageDialog::PopupMessage( 
        this->GetApplication(), this->Window, 
        "Reset Interface", 
        "All Drag & Drop events performed so far will be discarded. "
        "Note that your interface will be reset the next time you "
        "start this application.",
        vtkKWMessageDialog::WarningIcon);

  vtkKWUserInterfaceNotebookManager *uim_nb = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(
      this->Window->GetUserInterfaceManager());
  if (uim_nb)
    {
    uim_nb->DeleteAllDragAndDropEntries();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::FlatFrameCallback()
{
  if (!this->FlatFrameCheckButton ||
      !this->FlatFrameCheckButton->IsCreated())
    {
    return;
    }

  vtkKWToolbar::SetGlobalFlatAspect(
    this->FlatFrameCheckButton->GetState() ? 1 : 0);

  if (this->Window)
    {
    this->Window->UpdateToolbarState();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::FlatButtonsCallback()
{
  if (!this->FlatButtonsCheckButton ||
      !this->FlatButtonsCheckButton->IsCreated())
    {
    return;
    }

  vtkKWToolbar::SetGlobalWidgetsFlatAspect(
    this->FlatButtonsCheckButton->GetState() ? 1 : 0);

  if (this->Window)
    {
    this->Window->UpdateToolbarState();
    }
}

//---------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::DPICallback(double dpi)
{
  if (this->GetApplication())
    {
    this->GetApplication()->SetPrintTargetDPI(dpi);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Interface settings

  if (this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame->SetEnabled(this->GetEnabled());
    }

  if (this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->SaveUserInterfaceGeometryCheckButton)
    {
    this->SaveUserInterfaceGeometryCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->ShowSplashScreenCheckButton)
    {
    this->ShowSplashScreenCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton->SetEnabled(this->GetEnabled());
    }

  // Interface customization

  if (this->InterfaceCustomizationFrame)
    {
    this->InterfaceCustomizationFrame->SetEnabled(this->GetEnabled());
    }

  if (this->ResetDragAndDropButton)
    {
    this->ResetDragAndDropButton->SetEnabled(this->GetEnabled());
    }

  // Toolbar settings

  if (this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame->SetEnabled(this->GetEnabled());
    }

  if (this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->SetEnabled(this->GetEnabled());
    }

  // Print settings

  if (this->PrintSettingsFrame)
    {
    this->PrintSettingsFrame->SetEnabled(this->GetEnabled());
    }

  if (this->DPIOptionMenu)
    {
    this->DPIOptionMenu->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;
}


