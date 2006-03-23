/*=========================================================================

  Module:    vtkKWWindowBase.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWWindowBase.h"

#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWMostRecentFilesManager.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWSeparator.h"
#include "vtkKWTkcon.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkObjectFactory.h"
#include "vtkKWInternationalization.h"

#include <vtksys/SystemTools.hxx>

const char *vtkKWWindowBase::PrintOptionsMenuLabel = "Print Settings...";
const char *vtkKWWindowBase::FileMenuLabel = "File";
const char *vtkKWWindowBase::FileCloseMenuLabel = "Close";
const char *vtkKWWindowBase::FileExitMenuLabel = "Exit";
const char *vtkKWWindowBase::EditMenuLabel = "Edit";
const char *vtkKWWindowBase::ViewMenuLabel = "View";
const char *vtkKWWindowBase::WindowMenuLabel = "Window";
const char *vtkKWWindowBase::HelpMenuLabel = "Help";
const char *vtkKWWindowBase::HelpTopicsMenuLabel = "Help Topics";
const char *vtkKWWindowBase::HelpCheckForUpdatesMenuLabel = "Check for Updates";
const char *vtkKWWindowBase::ToolbarsVisibilityMenuLabel = "Toolbars";

const char *vtkKWWindowBase::OpenRecentFileMenuLabel = "Open Recent File";
const char *vtkKWWindowBase::WindowGeometryRegKey = "WindowGeometry";

const char *vtkKWWindowBase::DefaultGeometry = "900x700+0+0";

vtkCxxRevisionMacro(vtkKWWindowBase, "1.41");

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindowBase );

//----------------------------------------------------------------------------
vtkKWWindowBase::vtkKWWindowBase()
{
  // Menus

  this->FileMenu              = NULL;
  this->HelpMenu              = NULL;
  this->EditMenu              = NULL;
  this->ViewMenu              = NULL;
  this->WindowMenu            = NULL;

  // Separator

  this->MenuBarSeparator  = vtkKWSeparator::New();

  // Toolbars

  this->MainToolbarSet        = vtkKWToolbarSet::New();
  this->ToolbarsVisibilityMenu = NULL; 
  this->StatusToolbar         = NULL;

  // Main Frame

  this->MainFrame              = vtkKWFrame::New();

  // Status frame

  this->StatusFrameSeparator  = vtkKWSeparator::New();
  this->StatusFrame           = vtkKWFrame::New();
  this->StatusLabel           = vtkKWLabel::New();
  this->StatusImage           = NULL;

  this->ProgressGauge         = vtkKWProgressGauge::New();
  this->ProgressGaugePosition = 
    vtkKWWindowBase::ProgressGaugePositionStatusFrame;

  this->TrayFrame             = vtkKWFrame::New();
  this->TrayImageError        = vtkKWLabel::New();
  this->TrayFramePosition = 
    vtkKWWindowBase::TrayFramePositionStatusFrame;

  this->TclInteractor         = NULL;

  this->SupportHelp           = 0;
  this->SupportPrint          = 0;
  this->PromptBeforeClose     = 0;
  this->StatusFrameVisibility   = 1;

  this->MostRecentFilesManager = vtkKWMostRecentFilesManager::New();

  this->SetWindowClass("KitwareWidget");

  this->ScriptExtension       = NULL;
  this->SetScriptExtension(".tcl");

  this->ScriptType            = NULL;
  this->SetScriptType("Tcl");
}

//----------------------------------------------------------------------------
vtkKWWindowBase::~vtkKWWindowBase()
{
  this->PrepareForDelete();

  if (this->TclInteractor)
    {
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
    }

  if (this->FileMenu)
    {
    this->FileMenu->Delete();
    this->FileMenu = NULL;
    }

  if (this->HelpMenu)
    {
    this->HelpMenu->Delete();
    this->HelpMenu = NULL;
    }

  if (this->MainToolbarSet)
    {
    this->MainToolbarSet->Delete();
    this->MainToolbarSet = NULL;
    }

  if (this->MenuBarSeparator)
    {
    this->MenuBarSeparator->Delete();
    this->MenuBarSeparator = NULL;
    }

  if (this->StatusToolbar)
    {
    this->StatusToolbar->Delete();
    this->StatusToolbar = NULL;
    }

  if (this->MainFrame)
    {
    this->MainFrame->Delete();
    this->MainFrame = NULL;
    }

  if (this->StatusFrameSeparator)
    {
    this->StatusFrameSeparator->Delete();
    this->StatusFrameSeparator = NULL;
    }

  if (this->StatusFrame)
    {
    this->StatusFrame->Delete();
    this->StatusFrame = NULL;
    }

  if (this->StatusImage)
    {
    this->StatusImage->Delete();
    this->StatusImage = NULL;
    }

  if (this->StatusLabel)
    {
    this->StatusLabel->Delete();
    this->StatusLabel = NULL;
    }

  if (this->ProgressGauge)
    {
    this->ProgressGauge->Delete();
    this->ProgressGauge = NULL;
    }

  if (this->TrayFrame)
    {
    this->TrayFrame->Delete();
    this->TrayFrame = NULL;
    }

  if (this->TrayImageError)
    {
    this->TrayImageError->Delete();
    this->TrayImageError = NULL;
    }

  if (this->EditMenu)
    {
    this->EditMenu->Delete();
    this->EditMenu = NULL;
    }

  if (this->ViewMenu)
    {
    this->ViewMenu->Delete();
    this->ViewMenu = NULL;
    }

  if (this->WindowMenu)
    {
    this->WindowMenu->Delete();
    this->WindowMenu = NULL;
    }

  if (this->ToolbarsVisibilityMenu)
    {
    this->ToolbarsVisibilityMenu->Delete();
    this->ToolbarsVisibilityMenu = NULL;
    }

  if (this->MostRecentFilesManager)
    {
    this->MostRecentFilesManager->Delete();
    this->MostRecentFilesManager = NULL;
    }

  this->SetScriptExtension(NULL);
  this->SetScriptType(NULL);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::PrepareForDelete()
{
  // Have reference to this object:
  // this->Menu
  // this->MenuBarSeparator
  // this->MainToolbarSet
  // this->MainFrame
  // this->StatusFrameSeparator
  // this->StatusFrame

  if (this->TclInteractor )
    {
    this->TclInteractor->SetMasterWindow(NULL);
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
    }

  if (this->MainToolbarSet)
    {
    this->MainToolbarSet->SetToolbarVisibilityChangedCommand(NULL, NULL);
    this->MainToolbarSet->SetNumberOfToolbarsChangedCommand(NULL, NULL);
    this->MainToolbarSet->RemoveAllToolbars();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("class already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  vtkKWApplication *app = this->GetApplication();

  // Restore Window Geometry
  // Yes, it has to be done now

  if (app->GetSaveUserInterfaceGeometry())
    {
    this->RestoreWindowGeometryFromRegistry();
    }
  else
    {
    this->SetGeometry(vtkKWWindowBase::DefaultGeometry);
    }

  vtksys_stl::string cmd;
  vtksys_stl::string label;
  vtkKWMenu *menu = NULL;
  char buffer[512];

  this->SetIconName(app->GetPrettyName());

  // Menu : File

  menu = this->GetFileMenu();

  if (this->SupportPrint)
    {
    menu->AddCommand(
      vtkKWWindowBase::PrintOptionsMenuLabel, this, "PrintSettingsCallback",4);
    menu->AddSeparator();
    }

  menu->AddCommand(
    vtkKWWindowBase::FileCloseMenuLabel, this, "Close", 0);
  menu->AddCommand(
    vtkKWWindowBase::FileExitMenuLabel, app, "Exit", 1);

  this->MostRecentFilesManager->SetApplication(app);

  // Menu : Help

  menu = this->GetHelpMenu();

  if (this->SupportHelp)
    {
    cmd = "DisplayHelpDialog ";
    cmd += this->GetTclName();
    menu->AddCommand(
      vtkKWWindowBase::HelpTopicsMenuLabel, app, cmd.c_str(), 0);
    }

  if (app->HasCheckForUpdates())
    {
    menu->AddCommand(
      vtkKWWindowBase::HelpCheckForUpdatesMenuLabel,app,"CheckForUpdates",0);
    }
  
  menu->AddSeparator();
  sprintf(buffer, ks_("Menu|Help|About %s"), app->GetPrettyName());
  cmd = "DisplayAboutDialog ";
  cmd += this->GetTclName();
  menu->AddCommand(buffer, this->GetApplication(), cmd.c_str(), 0);

  // Menubar separator

  this->MenuBarSeparator->SetParent(this);  
  this->MenuBarSeparator->Create();
  this->MenuBarSeparator->SetOrientationToHorizontal();

  // Toolbars

  this->MainToolbarSet->SetParent(this);
  this->MainToolbarSet->Create();
  this->MainToolbarSet->TopSeparatorVisibilityOff();
  this->MainToolbarSet->BottomSeparatorVisibilityOn();
  this->MainToolbarSet->SynchronizeToolbarsVisibilityWithRegistryOn();
  this->MainToolbarSet->SetToolbarVisibilityChangedCommand(
    this, "ToolbarVisibilityChangedCallback");
  this->MainToolbarSet->SetNumberOfToolbarsChangedCommand(
    this, "NumberOfToolbarsChangedCallback");

  // Main frame

  this->MainFrame->SetParent(this);
  this->MainFrame->Create();

  // Status frame

  this->StatusFrame->SetParent(this);
  this->StatusFrame->Create();
  
  // Status frame separator

  this->StatusFrameSeparator->SetParent(this);
  this->StatusFrameSeparator->Create();
  this->StatusFrameSeparator->SetOrientationToHorizontal();

  // Status frame : image

  this->UpdateStatusImage();

  // Status frame : label

  this->StatusLabel->SetParent(this->StatusFrame);
  this->StatusLabel->Create();
  this->StatusLabel->SetReliefToSunken();
  this->StatusLabel->SetBorderWidth(0);
  this->StatusLabel->SetPadX(3);
  this->StatusLabel->SetAnchorToWest();

  // Progress gauge

  this->ProgressGauge->SetParent(this); 
  this->ProgressGauge->SetWidth(150);
  this->ProgressGauge->ExpandHeightOn();
  this->ProgressGauge->Create();
  this->ProgressGauge->SetBorderWidth(1);
  this->ProgressGauge->SetPadX(2);
  this->ProgressGauge->SetPadY(2);
  this->ProgressGauge->SetReliefToSunken();

  // Tray frame

  this->TrayFrame->SetParent(this);
  this->TrayFrame->Create();
  this->TrayFrame->SetBorderWidth(1);
  this->TrayFrame->SetReliefToSunken();

  // Tray frame : error image

  this->TrayImageError->SetParent(this->TrayFrame);
  this->TrayImageError->Create();
  this->TrayImageError->SetBorderWidth(1);
  this->TrayImageError->SetImageToPredefinedIcon(vtkKWIcon::IconEmpty16x16);

  this->Script("pack %s -fill both -ipadx 2 -ipady 0 -pady 0", 
               this->TrayImageError->GetWidgetName());

  // Pack and restore geometry

  this->Pack();

  // Update the enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->UnpackChildren();

  // Menubar separator
  
  if (this->MenuBarSeparator && this->MenuBarSeparator->IsCreated())
    {
    this->Script("pack %s -side top -fill x -pady 2",
                 this->MenuBarSeparator->GetWidgetName());
    }

  // Toolbars

  if (this->MainToolbarSet && this->MainToolbarSet->IsCreated())
    {
    vtksys_stl::string after;
    if (this->MenuBarSeparator && 
        this->MenuBarSeparator->IsCreated())
      {
      after = " -after ";
      after += this->MenuBarSeparator->GetWidgetName();
      }
    this->Script(
      "pack %s -padx 0 -pady 0 -side top -fill x -expand no %s",
      this->MainToolbarSet->GetWidgetName(), after.c_str());
    }

  // Main frame

  if (this->MainFrame && this->MainFrame->IsCreated())
    {
    this->Script("pack %s -side top -fill both -expand t",
                 this->MainFrame->GetWidgetName());
    }

  // Status frame and status frame separator

  if (this->StatusFrame && this->StatusFrame->IsCreated())
    {
    if (this->StatusFrameVisibility)
      {
      this->Script("pack %s -side bottom -fill x -pady 0",
                   this->StatusFrame->GetWidgetName());

      if (this->StatusFrameSeparator && 
          this->StatusFrameSeparator->IsCreated())
        {
        this->Script("pack %s -side bottom -fill x -pady 2",
                     this->StatusFrameSeparator->GetWidgetName());
        }
      }

    this->StatusFrame->UnpackChildren();

    // Status image (application logo)

    if (this->StatusImage && this->StatusImage->IsCreated())
      {
      this->StatusImage->Script(
        "pack %s -side left -anchor c -ipadx 1 -ipady 1 -fill y", 
        this->StatusImage->GetWidgetName());
      }

    // Status label (display status text)

    if (this->StatusLabel && this->StatusLabel)
      {
      this->Script("pack %s -side left -padx 1 -expand yes -fill both",
                   this->StatusLabel->GetWidgetName());
      }

    // Progress gauge

    if (this->ProgressGauge && this->ProgressGauge->IsCreated() &&
        this->ProgressGaugePosition == 
        vtkKWWindowBase::ProgressGaugePositionStatusFrame)
      {
      this->Script("pack %s -side left -padx 0 -pady 0 -fill y -in %s", 
                   this->ProgressGauge->GetWidgetName(),
                   this->StatusFrame->GetWidgetName());
      }

    // Tray frame (error icon, etc.)

    if (this->TrayFrame && this->TrayFrame->IsCreated() &&
        this->TrayFramePosition == 
        vtkKWWindowBase::TrayFramePositionStatusFrame)
      {
      this->Script(
      "pack %s -side left -ipadx 0 -ipady 0 -padx 0 -pady 0 -fill both -in %s",
        this->TrayFrame->GetWidgetName(), 
        this->StatusFrame->GetWidgetName());
      }
    }

  // Take care of placing the progress gauge and the tray frame in
  // a toolbar, if this is how they have been set

  if (this->MainToolbarSet)
    {
    int need_progress_in = 
      (this->ProgressGauge && 
       this->ProgressGauge->IsCreated() && 
       (this->ProgressGaugePosition == 
        vtkKWWindowBase::ProgressGaugePositionToolbar));

    int need_tray_in = 
      (this->TrayFrame && 
       this->TrayFrame->IsCreated() && 
       (this->TrayFramePosition == 
        vtkKWWindowBase::TrayFramePositionToolbar));

    // We need a toolbar for any of them (progress or tray)

    if (need_progress_in || need_tray_in)
      {
      if (!this->StatusToolbar)
        {
        this->StatusToolbar = vtkKWToolbar::New();
        this->StatusToolbar->SetName("Status");
        }
      if (!this->StatusToolbar->IsCreated() && this->IsCreated())
        {
        this->StatusToolbar->SetParent(
          this->MainToolbarSet->GetToolbarsFrame());
        this->StatusToolbar->Create();
        }
      }

    if (this->StatusToolbar)
      {
      // Add the progress, if not here already, or remove it if it is placed
      // somewhere else (say, in the status frame)

      int has_progress_in = 
        this->StatusToolbar->HasWidget(this->ProgressGauge);
      if (need_progress_in)
        {
        if (!has_progress_in)
          {
          this->StatusToolbar->AddWidget(this->ProgressGauge);
          }
        }
      else
        {
        if (has_progress_in)
          {
          this->StatusToolbar->RemoveWidget(this->ProgressGauge);
          }
        }

      // Add the tray frame, if not here already, or remove it if it is placed
      // somewhere else (say, in the status frame)

      int has_tray_in = 
        this->StatusToolbar->HasWidget(this->TrayFrame);
      if (need_tray_in)
        {
        if (!has_tray_in)
          {
          this->StatusToolbar->AddWidget(this->TrayFrame);
          }
        }
      else
        {
        if (has_tray_in)
          {
          this->StatusToolbar->RemoveWidget(this->TrayFrame);
          }
        }

      // Now do we really need that toolbar anymore ? If it is empty, just
      // remove it, otherwise make sure it is added to the main toolbar set
      // but anchored to the east side, so that it does not interfere
      // too much with the regular ones

      int has_toolbar = 
        this->MainToolbarSet->HasToolbar(this->StatusToolbar);
      if (this->StatusToolbar->GetNumberOfWidgets())
        {
        if (!has_toolbar)
          {
          this->MainToolbarSet->AddToolbar(this->StatusToolbar);
          this->MainToolbarSet->SetToolbarAnchorToEast(
            this->StatusToolbar);
          }
        }
      else
        {
        if (has_toolbar)
          {
          this->MainToolbarSet->RemoveToolbar(this->StatusToolbar);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWWindowBase::GetViewFrame()
{
  return this->MainFrame;
}

//----------------------------------------------------------------------------
int vtkKWWindowBase::DisplayCloseDialog()
{
  vtkKWMessageDialog *dialog = vtkKWMessageDialog::New();
  dialog->SetStyleToYesNo();
  dialog->SetMasterWindow(this);
  dialog->SetOptions(
    vtkKWMessageDialog::QuestionIcon | 
    vtkKWMessageDialog::Beep | 
    vtkKWMessageDialog::YesDefault);
  dialog->Create();
  dialog->SetText("Are you sure you want to close this window?");
  dialog->SetTitle(vtkKWWindowBase::FileCloseMenuLabel);
  int ret = dialog->Invoke();
  dialog->Delete();
  return ret;
}

//----------------------------------------------------------------------------
int vtkKWWindowBase::Close()
{
  // If a dialog is still up, complain and bail
  // This should be fixed, since we don't know here if the dialog that is
  // up is something that was created by this instance, and not by another
  // window instance (in the later case, it would be safe to close)

  if (this->GetApplication()->IsDialogUp())
    {
    vtkKWTkUtilities::Bell(this->GetApplication());
    return 0;
    }

  // Prompt confirmation if needed

  if (this->PromptBeforeClose && !this->DisplayCloseDialog())
    {
    return 0;
    }

  // Save its geometry

  if (this->GetApplication()->GetSaveUserInterfaceGeometry())
    {
    this->SaveWindowGeometryToRegistry();
    }

  // Just in case this was not done properly so far

  if (this->MostRecentFilesManager)
    {
    this->MostRecentFilesManager->SaveFilesToRegistry();
    }

  // Remove this window from the application. 
  // It is likely that the application will exit if there are no more windows.

  return this->GetApplication()->RemoveWindow(this);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SaveWindowGeometryToRegistry()
{
  if (!this->IsCreated())
    {
    return;
    }

  vtksys_stl::string geometry = this->GetGeometry();
  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindowBase::WindowGeometryRegKey, "%s", 
    geometry.c_str());
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::RestoreWindowGeometryFromRegistry()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", vtkKWWindowBase::WindowGeometryRegKey))
    {
    char geometry[40];
    if (this->GetApplication()->GetRegistryValue(
          2, "Geometry", vtkKWWindowBase::WindowGeometryRegKey, geometry))
      {
      this->SetGeometry(geometry);
      }
    }
  else
    {
    this->SetGeometry(vtkKWWindowBase::DefaultGeometry);
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SetStatusText(const char *text)
{
  this->StatusLabel->SetText(text);
}

//----------------------------------------------------------------------------
const char *vtkKWWindowBase::GetStatusText()
{
  return this->StatusLabel->GetText();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SetStatusFrameVisibility(int flag)
{
  if (this->StatusFrameVisibility == flag)
    {
    return;
    }

  this->StatusFrameVisibility = flag;
  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindowBase::GetFileMenu()
{
  if (!this->FileMenu)
    {
    this->FileMenu = vtkKWMenu::New();
    }

  if (!this->FileMenu->IsCreated() && this->GetMenu() && this->IsCreated())
    {
    this->FileMenu->SetParent(this->GetMenu());
    this->FileMenu->SetTearOff(0);
    this->FileMenu->Create();
    this->GetMenu()->InsertCascade(
      0, vtkKWWindowBase::FileMenuLabel, this->FileMenu, 0);
    }
  
  return this->FileMenu;
}

//----------------------------------------------------------------------------
int vtkKWWindowBase::GetFileMenuInsertPosition()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // First find the print-related menu commands

  if (this->GetFileMenu()->HasItem(vtkKWWindowBase::PrintOptionsMenuLabel))
    {
    return this->GetFileMenu()->GetIndexOfItem(
      vtkKWWindowBase::PrintOptionsMenuLabel);
    }

  // Otherwise find Close or Exit if Close was removed

  if (this->GetFileMenu()->HasItem(vtkKWWindowBase::FileCloseMenuLabel))
    {
    return this->GetFileMenu()->GetIndexOfItem(
      vtkKWWindowBase::FileCloseMenuLabel);  
    }

  if (this->GetFileMenu()->HasItem(vtkKWWindowBase::FileExitMenuLabel))
    {
    return this->GetFileMenu()->GetIndexOfItem(vtkKWWindowBase::FileExitMenuLabel);  
    }

  return this->GetFileMenu()->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindowBase::GetEditMenu()
{
  if (!this->EditMenu)
    {
    this->EditMenu = vtkKWMenu::New();
    }

  if (!this->EditMenu->IsCreated() && this->GetMenu() && this->IsCreated())
    {
    this->EditMenu->SetParent(this->GetMenu());
    this->EditMenu->SetTearOff(0);
    this->EditMenu->Create();
    // Usually after the File Menu (i.e., pos 1)
    this->GetMenu()->InsertCascade(
      1, vtkKWWindowBase::EditMenuLabel, this->EditMenu, 0);
    }
  
  return this->EditMenu;
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindowBase::GetViewMenu()
{
  if (!this->ViewMenu)
    {
    this->ViewMenu = vtkKWMenu::New();
    }

  if (!this->ViewMenu->IsCreated() && this->GetMenu() && this->IsCreated())
    {
    this->ViewMenu->SetParent(this->GetMenu());
    this->ViewMenu->SetTearOff(0);
    this->ViewMenu->Create();
    // Usually after the Edit Menu (do not use GetEditMenu() here)
    this->GetMenu()->InsertCascade(
      1 + (this->EditMenu ? 1 : 0), 
      vtkKWWindowBase::ViewMenuLabel, this->ViewMenu, 0);
    }

  return this->ViewMenu;
}

//----------------------------------------------------------------------------
int vtkKWWindowBase::GetViewMenuInsertPosition()
{
  return 0;
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindowBase::GetWindowMenu()
{
  if (!this->WindowMenu)
    {
    this->WindowMenu = vtkKWMenu::New();
    }

  if (!this->WindowMenu->IsCreated() && this->GetMenu() && this->IsCreated())
    {
    this->WindowMenu->SetParent(this->GetMenu());
    this->WindowMenu->SetTearOff(0);
    this->WindowMenu->Create();
    // Usually after View Menu (do not use GetEditMenu()/GetViewMenu() here)
    this->GetMenu()->InsertCascade(
      1 + (this->EditMenu ? 1 : 0) + (this->ViewMenu ? 1 : 0), 
      vtkKWWindowBase::WindowMenuLabel, this->WindowMenu, 0);
    }
  
  return this->WindowMenu;
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindowBase::GetHelpMenu()
{
  if (!this->HelpMenu)
    {
    this->HelpMenu = vtkKWMenu::New();
    }

  if (!this->HelpMenu->IsCreated() && this->GetMenu() && this->IsCreated())
    {
    this->HelpMenu->SetParent(this->GetMenu());
    this->HelpMenu->SetTearOff(0);
    this->HelpMenu->Create();
    // Usually at the end
    this->GetMenu()->AddCascade(
      vtkKWWindowBase::HelpMenuLabel, this->HelpMenu, 0);
    }
  
  return this->HelpMenu;
}

//----------------------------------------------------------------------------
int vtkKWWindowBase::GetHelpMenuInsertPosition()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // Find about

  if (this->GetHelpMenu()->HasItem("About*"))
    {
    return this->GetHelpMenu()->GetIndexOfItem("About*") - 1;
    }

  return this->GetHelpMenu()->GetNumberOfItems();
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindowBase::GetToolbarsVisibilityMenu()
{
  if (!this->ToolbarsVisibilityMenu)
    {
    this->ToolbarsVisibilityMenu = vtkKWMenu::New();
    }

  if (!this->ToolbarsVisibilityMenu->IsCreated() && 
      this->GetWindowMenu() &&
      this->IsCreated())
    {
    this->ToolbarsVisibilityMenu->SetParent(this->GetWindowMenu());
    this->ToolbarsVisibilityMenu->SetTearOff(0);
    this->ToolbarsVisibilityMenu->Create();
    this->GetWindowMenu()->InsertCascade(
      2, vtkKWWindowBase::ToolbarsVisibilityMenuLabel, 
      this->ToolbarsVisibilityMenu, 1, "Set Toolbars Visibility");
    }
  
  return this->ToolbarsVisibilityMenu;
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::InsertRecentFilesMenu(
  int pos, vtkObject *target)
{
  if (!this->IsCreated() || !this->MostRecentFilesManager)
    {
    return;
    }

  // Create the sub-menu if not done already

  vtkKWMenu *mrf_menu = this->MostRecentFilesManager->GetMenu();
  if (!mrf_menu->IsCreated())
    {
    mrf_menu->SetParent(this->GetFileMenu());
    mrf_menu->SetTearOff(0);
    mrf_menu->Create();
    }

  // Remove the menu if already there (in case that function was used to
  // move the menu)

  if (this->GetFileMenu()->HasItem(vtkKWWindowBase::OpenRecentFileMenuLabel))
    {
    this->GetFileMenu()->DeleteMenuItem(
      vtkKWWindowBase::OpenRecentFileMenuLabel);
    }

  this->GetFileMenu()->InsertCascade(
    pos, vtkKWWindowBase::OpenRecentFileMenuLabel, mrf_menu, 6);

  // Fill the recent files vector with recent files stored in registry
  // this will also update the menu

  this->MostRecentFilesManager->SetDefaultTargetObject(target);
  this->MostRecentFilesManager->RestoreFilesListFromRegistry();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::AddRecentFile(const char *name, 
                                    vtkObject *target,
                                    const char *command)
{  
  if (this->MostRecentFilesManager)
    {
    this->MostRecentFilesManager->AddFile(name, target, command);
    this->MostRecentFilesManager->SaveFilesToRegistry();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::NumberOfToolbarsChangedCallback()
{
  if (this->MainToolbarSet)
    {
    this->MainToolbarSet->PopulateToolbarsVisibilityMenu(
      this->GetToolbarsVisibilityMenu());
    }

  this->UpdateToolbarState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::ToolbarVisibilityChangedCallback(vtkKWToolbar*)
{
  if (this->MainToolbarSet)
    {
    this->MainToolbarSet->UpdateToolbarsVisibilityMenu(
      this->GetToolbarsVisibilityMenu());
    }

  this->UpdateToolbarState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::LoadScript()
{
  vtkKWLoadSaveDialog* load_dialog = vtkKWLoadSaveDialog::New();
  this->GetApplication()->RetrieveDialogLastPathRegistryValue(
    load_dialog, "LoadScriptLastPath");
  load_dialog->SetParent(this);
  load_dialog->Create();
  load_dialog->SaveDialogOff();
  load_dialog->SetTitle("Load Script");
  load_dialog->SetDefaultExtension(this->ScriptExtension);

  vtksys_stl::string filetypes;
  filetypes += "{{";
  filetypes += this->ScriptType;
  filetypes += " Scripts} {";
  filetypes += this->ScriptExtension;
  filetypes += "}} {{All Files} {.*}}";
  load_dialog->SetFileTypes(filetypes.c_str());

  int enabled = this->GetEnabled();
  this->SetEnabled(0);

  if (load_dialog->Invoke() && 
      load_dialog->GetFileName() && 
      strlen(load_dialog->GetFileName()) > 0)
    {
    if (!vtksys::SystemTools::FileExists(load_dialog->GetFileName()))
      {
      vtkWarningMacro("Unable to open script file!");
      }
    else
      {
      this->GetApplication()->SaveDialogLastPathRegistryValue(
        load_dialog, "LoadScriptLastPath");
      this->LoadScript(load_dialog->GetFileName());
      }
    }

  this->SetEnabled(enabled);
  load_dialog->Delete();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::LoadScript(const char *filename)
{
  this->GetApplication()->LoadScript(filename);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::WarningMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "Warning",
    message, vtkKWMessageDialog::WarningIcon);
  this->SetErrorIconToRed();
  this->InvokeEvent(vtkKWEvent::WarningMessageEvent, (void*)message);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::ErrorMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "Error",
    message, vtkKWMessageDialog::ErrorIcon);
  this->SetErrorIconToRed();
  this->InvokeEvent(vtkKWEvent::ErrorMessageEvent, (void*)message);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SetErrorIcon(int s)
{
  if (!this->TrayImageError || !this->TrayImageError->IsCreated())
    {
    return;
    }

  switch (s)
    {
    case vtkKWWindowBase::ErrorIconRed:
      this->TrayImageError->SetImageToPredefinedIcon(
        vtkKWIcon::IconErrorRedMini);
      break;
    case vtkKWWindowBase::ErrorIconBlack:
      this->TrayImageError->SetImageToPredefinedIcon(
        vtkKWIcon::IconErrorMini);
      break;
    default:
      this->TrayImageError->SetImageToPredefinedIcon(
        vtkKWIcon::IconEmpty16x16);
      break;
    }

  if (s == vtkKWWindowBase::ErrorIconNone)
    {
    this->TrayImageError->RemoveBinding("<Button-1>");
    }
  else
    {
    this->TrayImageError->SetBinding("<Button-1>", this, "ErrorIconCallback");
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::ErrorIconCallback()
{
  this->SetErrorIcon(vtkKWWindowBase::ErrorIconBlack);
}

//----------------------------------------------------------------------------
char* vtkKWWindowBase::GetTitle()
{
  if (!this->Title && 
      this->GetApplication() && 
      this->GetApplication()->GetName())
    {
    return this->GetApplication()->GetName();
    }
  return this->Title;
}

//----------------------------------------------------------------------------
vtkKWTclInteractor* vtkKWWindowBase::GetTclInteractor()
{
  if (!this->TclInteractor)
    {
    this->TclInteractor = vtkKWTkcon::New();
    }

  if (!this->TclInteractor->IsCreated() && this->IsCreated())
    {
    this->TclInteractor->SetApplication(this->GetApplication());
    this->TclInteractor->SetMasterWindow(this);
    this->TclInteractor->Create();
    }
  
  return this->TclInteractor;
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::DisplayTclInteractor()
{
  vtkKWTclInteractor *tcl_interactor = this->GetTclInteractor();
  if (tcl_interactor)
    {
    vtksys_stl::string title;
    if (this->GetTitle())
      {
      title += this->GetTitle();
      title += " : ";
      }
    title += "Tcl Interactor";
    tcl_interactor->SetTitle(title.c_str());
    tcl_interactor->Display();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SetProgressGaugePosition(int s)
{
  if (s < vtkKWWindowBase::ProgressGaugePositionStatusFrame)
    {
    s = vtkKWWindowBase::ProgressGaugePositionStatusFrame;
    }
  else if (s > vtkKWWindowBase::ProgressGaugePositionToolbar) 
    {
    s = vtkKWWindowBase::ProgressGaugePositionToolbar;
    }
  if (s == this->ProgressGaugePosition)
    {
    return;
    }

  this->ProgressGaugePosition = s;
  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SetTrayFramePosition(int s)
{
  if (s < vtkKWWindowBase::TrayFramePositionStatusFrame)
    {
    s = vtkKWWindowBase::TrayFramePositionStatusFrame;
    }
  else if (s > vtkKWWindowBase::TrayFramePositionToolbar) 
    {
    s = vtkKWWindowBase::TrayFramePositionToolbar;
    }
  if (s == this->TrayFramePosition)
    {
    return;
    }

  this->TrayFramePosition = s;
  this->Modified();
  this->Pack();
}

//----------------------------------------------------------------------------
vtkKWLabel* vtkKWWindowBase::GetStatusImage()
{
  if (!this->StatusImage)
    {
    this->StatusImage = vtkKWLabel::New();
    }

  if (!this->StatusImage->IsCreated() && 
      this->StatusFrame && this->StatusFrame->IsCreated())
    {
    this->StatusImage->SetParent(this->StatusFrame);
    this->StatusImage->Create();
    this->StatusImage->SetBorderWidth(1);
    this->StatusImage->SetReliefToSunken();
    }

  return this->StatusImage;
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateStatusImage()
{
  // No default image here

  // Subclasses will likely update the StatusImage with a logo of their own.
  // Here is, for example, how, provided that you created or updated
  // the myownlogo image (photo), using vtkKWTkUtilities::UpdatePhoto for ex.
  /*
  vtkKWLabel *status_image = this->GetStatusImage();
  if (status_image && status_image->IsCreated())
    {
    status_image->SetConfigurationOption("-image", "myownlogo");
    }
  */
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::Update()
{
  // Make sure everything is enable/disable accordingly

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateToolbarState()
{
  if (this->MainToolbarSet)
    {
    this->MainToolbarSet->SetToolbarsFlatAspect(
      vtkKWToolbar::GetGlobalFlatAspect());
    this->MainToolbarSet->SetToolbarsWidgetsFlatAspect(
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    this->PropagateEnableState(this->MainToolbarSet);
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Update the toolbars

  this->UpdateToolbarState();

  // Update the Tcl interactor

  this->PropagateEnableState(this->TclInteractor);

  // Update the window element

  this->PropagateEnableState(this->MainFrame);
  this->PropagateEnableState(this->StatusFrame);
  this->PropagateEnableState(this->MenuBarSeparator);

  // Given the state, can we close or not ?

  this->SetDeleteWindowProtocolCommand(
    this, this->GetEnabled() ? 
    vtkKWWindowBase::FileCloseMenuLabel : 
    "SetStatusText \"Can not close while UI is disabled\"");

  // Update the menus

  this->UpdateMenuState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateMenuState()
{
  this->PropagateEnableState(this->Menu);
  this->PropagateEnableState(this->FileMenu);
  this->PropagateEnableState(this->EditMenu);
  this->PropagateEnableState(this->ViewMenu);
  this->PropagateEnableState(this->WindowMenu);
  this->PropagateEnableState(this->HelpMenu);
  this->PropagateEnableState(this->ToolbarsVisibilityMenu);

  // Most Recent Files

  if (this->MostRecentFilesManager)
    {
    this->PropagateEnableState(this->MostRecentFilesManager->GetMenu());
    this->MostRecentFilesManager->UpdateMenuStateInParent();
    }

  // Update the About entry, since the pretty name also depends on the
  // limited edition mode

  if (this->HelpMenu) // do not use GetHelpMenu() here
    {
    vtksys_stl::string about_command = "DisplayAbout ";
    about_command +=  this->GetTclName();
    int pos = this->GetHelpMenu()->GetIndexOfCommand(
      this->GetApplication(), about_command.c_str());
    if (pos >= 0)
      {
      vtksys_stl::string label("-label {About ");
      label += this->GetApplication()->GetPrettyName();
      label += "}";
      this->GetHelpMenu()->ConfigureItem(pos, label.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Menu: " << this->Menu << endl;
  os << indent << "FileMenu: " << this->FileMenu << endl;
  os << indent << "HelpMenu: " << this->HelpMenu << endl;
  os << indent << "MainFrame: " << this->MainFrame << endl;
  os << indent << "ProgressGauge: " << this->GetProgressGauge() << endl;
  os << indent << "PromptBeforeClose: " << this->GetPromptBeforeClose() 
     << endl;
  os << indent << "ScriptExtension: " << this->GetScriptExtension() << endl;
  os << indent << "ScriptType: " << this->GetScriptType() << endl;
  os << indent << "SupportHelp: " << this->GetSupportHelp() << endl;
  os << indent << "SupportPrint: " << this->GetSupportPrint() << endl;
  os << indent << "StatusFrame: " << this->GetStatusFrame() << endl;
  os << indent << "WindowClass: " << this->GetWindowClass() << endl;  
  os << indent << "TclInteractor: " << this->GetTclInteractor() << endl;
  os << indent << "MainToolbarSet: " << this->GetMainToolbarSet() << endl;
  os << indent << "StatusFrameVisibility: " << (this->StatusFrameVisibility ? "On" : "Off") << endl;
  os << indent << "TrayFramePosition: " << this->TrayFramePosition << endl;
}


