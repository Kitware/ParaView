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
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

#define VTK_KW_APPLICATION_SETTINGS_UIP_LABEL "Application Settings"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkKWApplicationSettingsInterface, "1.26");

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
  this->SaveWindowGeometryCheckButton = 0;
  this->ShowSplashScreenCheckButton = 0;
  this->ShowBalloonHelpCheckButton = 0;
  this->ShowMostRecentPanelsCheckButton = 0;

  // Interface customization

  this->InterfaceCustomizationFrame = 0;
  this->EnableDragAndDropCheckButton = 0;
  this->ResetDragAndDropButton = 0;

  // Toolbar settings

  this->ToolbarSettingsFrame = 0;
  this->FlatFrameCheckButton = 0;
  this->FlatButtonsCheckButton = 0;
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

  if (this->SaveWindowGeometryCheckButton)
    {
    this->SaveWindowGeometryCheckButton->Delete();
    this->SaveWindowGeometryCheckButton = NULL;
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

  if (this->ShowMostRecentPanelsCheckButton)
    {
    this->ShowMostRecentPanelsCheckButton->Delete();
    this->ShowMostRecentPanelsCheckButton = NULL;
    }

  // Interface customization

  if (this->InterfaceCustomizationFrame)
    {
    this->InterfaceCustomizationFrame->Delete();
    this->InterfaceCustomizationFrame = NULL;
    }

  if (this->EnableDragAndDropCheckButton)
    {
    this->EnableDragAndDropCheckButton->Delete();
    this->EnableDragAndDropCheckButton = NULL;
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

  if (!this->SaveWindowGeometryCheckButton)
    {
    this->SaveWindowGeometryCheckButton = vtkKWCheckButton::New();
    }

  this->SaveWindowGeometryCheckButton->SetParent(frame);
  this->SaveWindowGeometryCheckButton->Create(app, 0);
  this->SaveWindowGeometryCheckButton->SetText("Save window geometry on exit");
  this->SaveWindowGeometryCheckButton->SetCommand(this, "SaveWindowGeometryCallback");
  this->SaveWindowGeometryCheckButton->SetBalloonHelpString(
    "Save the window size and location on exit and restore it on startup.");

  tk_cmd << "pack " << this->SaveWindowGeometryCheckButton->GetWidgetName()
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
  this->ShowBalloonHelpCheckButton->SetText("Show tooltips");
  this->ShowBalloonHelpCheckButton->SetCommand(
    this, "ShowBalloonHelpCallback");
  this->ShowBalloonHelpCheckButton->SetBalloonHelpString(
    "Display help in a yellow popup-box on the screen when you rest the "
    "mouse over an item that supports it.");

  tk_cmd << "pack " << this->ShowBalloonHelpCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : show most recent panels

  if (!this->ShowMostRecentPanelsCheckButton)
    {
    this->ShowMostRecentPanelsCheckButton = vtkKWCheckButton::New();
    }

  this->ShowMostRecentPanelsCheckButton->SetParent(frame);
  this->ShowMostRecentPanelsCheckButton->Create(app, 0);
  this->ShowMostRecentPanelsCheckButton->SetText(
    "Show most recent panels");
  this->ShowMostRecentPanelsCheckButton->SetCommand(
    this, "ShowMostRecentPanelsCallback");
  this->ShowMostRecentPanelsCheckButton->SetBalloonHelpString(
    "Show (and maintain) the most recent panels visited by the user.");

  tk_cmd << "pack " << this->ShowMostRecentPanelsCheckButton->GetWidgetName()
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
  // Interface customization : Drag & Drop : Enable

  if (!this->EnableDragAndDropCheckButton)
    {
    this->EnableDragAndDropCheckButton = vtkKWCheckButton::New();
    }

  this->EnableDragAndDropCheckButton->SetParent(frame);
  this->EnableDragAndDropCheckButton->Create(app, 0);
  this->EnableDragAndDropCheckButton->SetText("Enable interface Drag & Drop");
  this->EnableDragAndDropCheckButton->SetCommand(
    this, "EnableDragAndDropCallback");
  this->EnableDragAndDropCheckButton->SetBalloonHelpString(
    "When this option is enabled you can drag & drop elements of the "
    "interface within the same panel or from one panel to the other. "
    "To do so, drag the title of a labeled frame to reposition it within "
    "a panel, or drop it on another tab to move it to a different panel.");

  tk_cmd << "pack " << this->EnableDragAndDropCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

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
    "Reset the placement of all user interface elements, discarding any "
    "Drag & Drop events.");

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
      this->GetApplication()->GetMessageDialogResponse(VTK_KW_EXIT_DIALOG_NAME)
      ? 0 : 1);
    }

  // Interface settings : Save application geometry on exit ?

  if (this->SaveWindowGeometryCheckButton)
    {
    this->SaveWindowGeometryCheckButton->SetState(
      this->GetApplication()->GetSaveWindowGeometry());
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
      this->GetApplication()->GetShowBalloonHelp());
    }

  // Interface settings : show most recent panels

  vtkKWUserInterfaceNotebookManager *uim_nb = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(
      this->Window->GetUserInterfaceManager());

  if (this->ShowMostRecentPanelsCheckButton)
    {
    if (this->GetApplication()->HasRegisteryValue(
          2, "RunTime", VTK_KW_SHOW_MOST_RECENT_PANELS_REG_KEY))
      {
      this->ShowMostRecentPanelsCheckButton->SetState(
        this->GetApplication()->GetIntRegisteryValue(
          2, "RunTime", VTK_KW_SHOW_MOST_RECENT_PANELS_REG_KEY));
      }
    else
      {
      this->ShowMostRecentPanelsCheckButton->SetState(1);
      }
    if (!uim_nb)
      {
      this->ShowMostRecentPanelsCheckButton->SetEnabled(0);
      }
    }

  // Interface customization : Drag & Drop : Enable

  if (this->EnableDragAndDropCheckButton)
    {
    if (uim_nb)
      {
      this->EnableDragAndDropCheckButton->SetState(
        uim_nb->GetEnableDragAndDrop());
      }
    else
      {
      this->EnableDragAndDropCheckButton->SetEnabled(0);
      this->ResetDragAndDropButton->SetEnabled(0);
      }
    }

  // Toolbar settings : flat frame

  if (FlatFrameCheckButton)
    {
    if (this->GetApplication()->HasRegisteryValue(
          2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY))
      {
      this->FlatFrameCheckButton->SetState(
        this->GetApplication()->GetIntRegisteryValue(
          2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY));
      }
    else
      {
      this->FlatFrameCheckButton->SetState(
        vtkKWToolbar::GetGlobalFlatAspect());
      }
    }

  // Toolbar settings : flat buttons

  if (FlatButtonsCheckButton)
    {
    if (this->GetApplication()->HasRegisteryValue(
          2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY))
      {
      this->FlatButtonsCheckButton->SetState(
        this->GetApplication()->GetIntRegisteryValue(
          2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY));
      }
    else
      {
      this->FlatButtonsCheckButton->SetState(
        vtkKWToolbar::GetGlobalWidgetsFlatAspect());
      }
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
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ConfirmExitCallback()
{
  if (!this->ConfirmExitCheckButton || 
      !this->ConfirmExitCheckButton->IsCreated())
    {
    return;
    }

  this->GetApplication()->SetMessageDialogResponse(
    VTK_KW_EXIT_DIALOG_NAME, 
    this->ConfirmExitCheckButton->GetState() ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SaveWindowGeometryCallback()
{
  if (!this->SaveWindowGeometryCheckButton || 
      !this->SaveWindowGeometryCheckButton->IsCreated())
    {
    return;
    }
  
  int state = this->SaveWindowGeometryCheckButton->GetState() ? 1 : 0;
  
  this->GetApplication()->SetRegisteryValue(
    2, "Geometry", VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY, "%d", state);
  
  this->GetApplication()->SetSaveWindowGeometry(state);
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
 
  this->GetApplication()->SetRegisteryValue(
    2, "RunTime", VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY, "%d", state);

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

  this->GetApplication()->SetRegisteryValue(
    2, "RunTime", VTK_KW_SHOW_TOOLTIPS_REG_KEY, "%d", state);

  this->GetApplication()->SetShowBalloonHelp(state);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ShowMostRecentPanelsCallback()
{
  if (this->IsCreated())
    {
    int flag = this->ShowMostRecentPanelsCheckButton->GetState() ? 1 : 0;
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_KW_SHOW_MOST_RECENT_PANELS_REG_KEY, "%d", flag);
    if (this->Window)
      {
      this->Window->ShowMostRecentPanels(flag);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::EnableDragAndDropCallback()
{
  if (this->IsCreated())
    {
    int flag = this->EnableDragAndDropCheckButton->GetState() ? 1 : 0;
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_KW_ENABLE_GUI_DRAG_AND_DROP_REG_KEY, "%d", flag);
    vtkKWUserInterfaceNotebookManager *uim_nb = 
      vtkKWUserInterfaceNotebookManager::SafeDownCast(
        this->Window->GetUserInterfaceManager());
    if (uim_nb)
      {
      uim_nb->SetEnableDragAndDrop(flag);
      }
    }
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

  this->GetApplication()->SetRegisteryValue(
    2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY, "%d", 
    this->FlatFrameCheckButton->GetState() ? 1 : 0);

  if (this->Window)
    {
    this->Window->UpdateToolbarAspect();
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

  this->GetApplication()->SetRegisteryValue(
    2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY, "%d", 
    this->FlatButtonsCheckButton->GetState() ? 1 : 0); 

  if (this->Window)
    {
    this->Window->UpdateToolbarAspect();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Interface settings

  if (this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame->SetEnabled(this->Enabled);
    }

  if (this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton->SetEnabled(this->Enabled);
    }

  if (this->SaveWindowGeometryCheckButton)
    {
    this->SaveWindowGeometryCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ShowSplashScreenCheckButton)
    {
    this->ShowSplashScreenCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ShowMostRecentPanelsCheckButton)
    {
    this->ShowMostRecentPanelsCheckButton->SetEnabled(this->Enabled);
    }

  // Interface customization

  if (this->InterfaceCustomizationFrame)
    {
    this->InterfaceCustomizationFrame->SetEnabled(this->Enabled);
    }

  if (this->EnableDragAndDropCheckButton)
    {
    this->EnableDragAndDropCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ResetDragAndDropButton)
    {
    this->ResetDragAndDropButton->SetEnabled(this->Enabled);
    }

  // Toolbar settings

  if (this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame->SetEnabled(this->Enabled);
    }

  if (this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->SetEnabled(this->Enabled);
    }

  if (this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;
}


