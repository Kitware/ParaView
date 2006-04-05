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
#include "vtkKWFrameWithLabel.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWPushButton.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceManagerNotebook.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

const char *vtkKWApplicationSettingsInterface::PrintSettingsLabel = "Page Setup";

#define VTK_KW_APPLICATION_SETTINGS_UIP_LABEL "Application Settings"
#define VTK_KW_APPLICATION_SETTINGS_DPI_FORMAT "%.1lf"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkKWApplicationSettingsInterface, "1.55");

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface::vtkKWApplicationSettingsInterface()
{
  this->SetName(
    ks_("Application Settings Panel|Title|Application Settings"));

  this->Window = 0;

  // Interface settings

  this->InterfaceSettingsFrame = 0;
  this->ConfirmExitCheckButton = 0;
  this->SaveUserInterfaceGeometryCheckButton = 0;
  this->SplashScreenVisibilityCheckButton = 0;
  this->BalloonHelpVisibilityCheckButton = 0;
  this->ViewPanelPositionOptionMenu = 0;

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

  if (this->SplashScreenVisibilityCheckButton)
    {
    this->SplashScreenVisibilityCheckButton->Delete();
    this->SplashScreenVisibilityCheckButton = NULL;
    }

  if (this->BalloonHelpVisibilityCheckButton)
    {
    this->BalloonHelpVisibilityCheckButton->Delete();
    this->BalloonHelpVisibilityCheckButton = NULL;
    }

  if (this->ViewPanelPositionOptionMenu)
    {
    this->ViewPanelPositionOptionMenu->Delete();
    this->ViewPanelPositionOptionMenu = NULL;
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
void vtkKWApplicationSettingsInterface::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create();

  ostrstream tk_cmd;
  vtkKWWidget *page;
  vtkKWFrame *frame;
  vtkKWMenu *menu;

  // --------------------------------------------------------------
  // Add a "Preferences" page

  this->AddPage(this->GetName(), "Change the application settings");
  page = this->GetPageWidget(this->GetName());
  
  // --------------------------------------------------------------
  // Interface settings : main frame

  if (!this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame = vtkKWFrameWithLabel::New();
    }

  this->InterfaceSettingsFrame->SetParent(this->GetPagesParentWidget());
  this->InterfaceSettingsFrame->Create();
  this->InterfaceSettingsFrame->SetLabelText("Interface Settings");
    
  tk_cmd << "pack " << this->InterfaceSettingsFrame->GetWidgetName()
         << " -side top -anchor nw -fill x -padx 2 -pady 2 " 
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->InterfaceSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Interface settings : Confirm on exit ?

  if (!this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton = vtkKWCheckButton::New();
    }

  this->ConfirmExitCheckButton->SetParent(frame);
  this->ConfirmExitCheckButton->Create();
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
  this->SaveUserInterfaceGeometryCheckButton->Create();
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

  if (this->GetApplication()->GetSupportSplashScreen())
    {
    if (!this->SplashScreenVisibilityCheckButton)
      {
      this->SplashScreenVisibilityCheckButton = vtkKWCheckButton::New();
      }

    this->SplashScreenVisibilityCheckButton->SetParent(frame);
    this->SplashScreenVisibilityCheckButton->Create();
    this->SplashScreenVisibilityCheckButton->SetText("Show splash screen");
    this->SplashScreenVisibilityCheckButton->SetCommand(
      this, "SplashScreenVisibilityCallback");
    this->SplashScreenVisibilityCheckButton->SetBalloonHelpString(
      "Display the splash information screen at startup.");

    tk_cmd << "pack " << this->SplashScreenVisibilityCheckButton->GetWidgetName()
           << "  -side top -anchor w -expand no -fill none" << endl;
    }

  // --------------------------------------------------------------
  // Interface settings : Show balloon help ?

  if (!this->BalloonHelpVisibilityCheckButton)
    {
    this->BalloonHelpVisibilityCheckButton = vtkKWCheckButton::New();
    }

  this->BalloonHelpVisibilityCheckButton->SetParent(frame);
  this->BalloonHelpVisibilityCheckButton->Create();
  this->BalloonHelpVisibilityCheckButton->SetText("Show balloon help");
  this->BalloonHelpVisibilityCheckButton->SetCommand(
    this, "BalloonHelpVisibilityCallback");
  this->BalloonHelpVisibilityCheckButton->SetBalloonHelpString(
    "Display help in a yellow popup-box on the screen when you rest the "
    "mouse over an item that supports it.");

  tk_cmd << "pack " << this->BalloonHelpVisibilityCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : View Panel Position

  if (!this->ViewPanelPositionOptionMenu)
    {
    this->ViewPanelPositionOptionMenu = vtkKWMenuButtonWithLabel::New();
    }

  this->ViewPanelPositionOptionMenu->SetParent(frame);
  this->ViewPanelPositionOptionMenu->Create();

  this->ViewPanelPositionOptionMenu->GetLabel()->SetText(
    "View panel position:");

  menu = this->ViewPanelPositionOptionMenu->GetWidget()->GetMenu();
  menu->AddRadioButton("Left", this, "ViewPanelPositionCallback");
  menu->AddRadioButton("Right", this, "ViewPanelPositionCallback");

  tk_cmd << "pack " << this->ViewPanelPositionOptionMenu->GetWidgetName()
         << " -side top -anchor w -padx 2 -pady 2" << endl;

  // --------------------------------------------------------------
  // Interface customization : main frame

  if (!this->InterfaceCustomizationFrame)
    {
    this->InterfaceCustomizationFrame = vtkKWFrameWithLabel::New();
    }

  this->InterfaceCustomizationFrame->SetParent(this->GetPagesParentWidget());
  this->InterfaceCustomizationFrame->Create();
  this->InterfaceCustomizationFrame->SetLabelText("Drag & Drop Settings");
    
  tk_cmd << "pack " << this->InterfaceCustomizationFrame->GetWidgetName()
         << " -side top -anchor nw -fill x -padx 2 -pady 2 " 
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->InterfaceCustomizationFrame->GetFrame();

  // --------------------------------------------------------------
  // Interface customization : Drag & Drop : Reset

  if (!this->ResetDragAndDropButton)
    {
    this->ResetDragAndDropButton = vtkKWPushButton::New();
    }

  this->ResetDragAndDropButton->SetParent(frame);
  this->ResetDragAndDropButton->Create();
  this->ResetDragAndDropButton->SetText("Reset Interface To Default State");
  this->ResetDragAndDropButton->SetCommand(this, "ResetDragAndDropCallback");
  this->ResetDragAndDropButton->SetBalloonHelpString(
    "You can drag & drop elements of the "
    "interface within the same panel or from one panel to the other. "
    "To do so, drag the title of a labeled frame to reposition it within "
    "a panel, or drop it on another tab to move it to a different panel. "
    "Press this button to reset the placement of all user interface elements "
    "to their default position. You will need to restart the application for "
    "the interface to be reset.");

  tk_cmd << "pack " << this->ResetDragAndDropButton->GetWidgetName()
         << "  -side top -anchor w -expand y -fill x -padx 2 -pady 2" 
         << endl;

  // --------------------------------------------------------------
  // Toolbar settings : main frame

  if (!this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame = vtkKWFrameWithLabel::New();
    }

  this->ToolbarSettingsFrame->SetParent(this->GetPagesParentWidget());
  this->ToolbarSettingsFrame->Create();
  this->ToolbarSettingsFrame->SetLabelText("Toolbar Settings");
    
  tk_cmd << "pack " << this->ToolbarSettingsFrame->GetWidgetName()
         << " -side top -anchor nw -fill x -padx 2 -pady 2 "  
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->ToolbarSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Toolbar settings : flat frame

  if (!this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton = vtkKWCheckButton::New();
    }

  this->FlatFrameCheckButton->SetParent(frame);
  this->FlatFrameCheckButton->Create();
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
  this->FlatButtonsCheckButton->Create();
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
    this->PrintSettingsFrame = vtkKWFrameWithLabel::New();
    }

  this->PrintSettingsFrame->SetParent(this->GetPagesParentWidget());
  this->PrintSettingsFrame->Create();
  this->PrintSettingsFrame->SetLabelText(
    vtkKWApplicationSettingsInterface::PrintSettingsLabel);
    
  tk_cmd << "pack " << this->PrintSettingsFrame->GetWidgetName()
         << " -side top -anchor nw -fill x -padx 2 -pady 2 "  
         << " -in " << page->GetWidgetName() << endl;
  
  frame = this->PrintSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Print settings : DPI

  if (!this->DPIOptionMenu)
    {
    this->DPIOptionMenu = vtkKWMenuButtonWithLabel::New();
    }

  this->DPIOptionMenu->SetParent(frame);
  this->DPIOptionMenu->Create();

  this->DPIOptionMenu->GetLabel()->SetText("DPI:");

  double dpis[] = { 100.0, 150.0, 300.0, 600.0 };
  for (unsigned int i = 0; i < sizeof(dpis) / sizeof(double); i++)
    {
    char label[128], command[128];
    sprintf(command, "DPICallback %lf", dpis[i]);
    sprintf(label, VTK_KW_APPLICATION_SETTINGS_DPI_FORMAT, dpis[i]);
    this->DPIOptionMenu->GetWidget()->GetMenu()->AddRadioButton(
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
    this->ConfirmExitCheckButton->SetSelectedState(
      vtkKWMessageDialog::RestoreMessageDialogResponseFromRegistry(
        this->GetApplication(), vtkKWApplication::ExitDialogName) ? 0 : 1);
    }

  // Interface settings : Save application geometry on exit ?

  if (this->SaveUserInterfaceGeometryCheckButton)
    {
    this->SaveUserInterfaceGeometryCheckButton->SetSelectedState(
      this->GetApplication()->GetSaveUserInterfaceGeometry());
    }
  
  // Interface settings : Show splash screen ?

  if (this->SplashScreenVisibilityCheckButton)
    {
    this->SplashScreenVisibilityCheckButton->SetSelectedState(
      this->GetApplication()->GetSplashScreenVisibility());
    }

  // Interface settings : Show balloon help ?

  if (this->BalloonHelpVisibilityCheckButton)
    {
    vtkKWBalloonHelpManager *mgr = 
      this->GetApplication()->GetBalloonHelpManager();
    if (mgr)
      {
      this->BalloonHelpVisibilityCheckButton->SetSelectedState(
        mgr->GetVisibility());
      }
    }

  // Interface settings : View Panel Position

  if (this->ViewPanelPositionOptionMenu && 
      this->ViewPanelPositionOptionMenu->GetWidget() && this->Window)
    {
    if (this->Window->GetViewPanelPosition() == 
        vtkKWWindow::ViewPanelPositionLeft)
      {
      this->ViewPanelPositionOptionMenu->GetWidget()->SetValue("Left");
      }
    else if (this->Window->GetViewPanelPosition() == 
             vtkKWWindow::ViewPanelPositionRight)
      {
      this->ViewPanelPositionOptionMenu->GetWidget()->SetValue("Right");
      }
    }

  // Interface customization : Drag & Drop : Enable

  vtkKWUserInterfaceManagerNotebook *uim_nb = NULL;
  if (this->Window->HasMainUserInterfaceManager())
    {
    uim_nb = vtkKWUserInterfaceManagerNotebook::SafeDownCast(
      this->Window->GetMainUserInterfaceManager());
    }
  if (this->ResetDragAndDropButton && !uim_nb)
    {
    this->ResetDragAndDropButton->SetEnabled(0);
    }

  // Toolbar settings : flat frame

  if (FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->SetSelectedState(
      vtkKWToolbar::GetGlobalFlatAspect());
    }

  // Toolbar settings : flat buttons

  if (FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->SetSelectedState(
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    }

  // Print settings

  if (this->DPIOptionMenu && this->DPIOptionMenu->GetWidget() && this->Window)
    {
    char buffer[128];
    sprintf(buffer, VTK_KW_APPLICATION_SETTINGS_DPI_FORMAT, 
            this->GetApplication()->GetPrintTargetDPI());
    this->DPIOptionMenu->GetWidget()->SetValue(buffer);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ConfirmExitCallback(int state)
{
  vtkKWMessageDialog::SaveMessageDialogResponseToRegistry(
    this->GetApplication(),
    vtkKWApplication::ExitDialogName, 
    state ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SaveUserInterfaceGeometryCallback(
  int state)
{
  this->GetApplication()->SetSaveUserInterfaceGeometry(state ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SplashScreenVisibilityCallback(
  int state)
{
  this->GetApplication()->SetSplashScreenVisibility(state ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::BalloonHelpVisibilityCallback(
  int state)
{
  vtkKWBalloonHelpManager *mgr = 
    this->GetApplication()->GetBalloonHelpManager();
  if (mgr)
    {
    mgr->SetVisibility(state ? 1 : 0);
    }
}

//---------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ViewPanelPositionCallback()
{
  if (this->ViewPanelPositionOptionMenu && 
      this->ViewPanelPositionOptionMenu->GetWidget() && this->Window)
    {
    const char *pos = 
      this->ViewPanelPositionOptionMenu->GetWidget()->GetValue();
    if (pos)
      {
      if (!strcmp(pos, "Left"))
        {
        this->Window->SetViewPanelPositionToLeft();
        }
      else if (!strcmp(pos, "Right"))
        {
        this->Window->SetViewPanelPositionToRight();
        }
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
        "Note that the interface will be reset the next time you "
        "start this application.",
        vtkKWMessageDialog::WarningIcon);

  if (this->Window->HasMainUserInterfaceManager())
    {
    vtkKWUserInterfaceManagerNotebook *uim_nb = 
      vtkKWUserInterfaceManagerNotebook::SafeDownCast(
        this->Window->GetMainUserInterfaceManager());
    if (uim_nb)
      {
      uim_nb->DeleteAllDragAndDropEntries();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::FlatFrameCallback(int state)
{
  vtkKWToolbar::SetGlobalFlatAspect(state ? 1 : 0);
  if (this->Window)
    {
    this->Window->UpdateToolbarState();
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::FlatButtonsCallback(int state)
{
  vtkKWToolbar::SetGlobalWidgetsFlatAspect(state ? 1 : 0);
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

  if (this->SplashScreenVisibilityCheckButton)
    {
    this->SplashScreenVisibilityCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->BalloonHelpVisibilityCheckButton)
    {
    this->BalloonHelpVisibilityCheckButton->SetEnabled(this->GetEnabled());
    }

  if (this->ViewPanelPositionOptionMenu)
    {
    this->ViewPanelPositionOptionMenu->SetEnabled(this->GetEnabled());
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


