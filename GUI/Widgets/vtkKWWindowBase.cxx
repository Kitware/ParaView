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
#include "vtkKWTclInteractor.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

const char *vtkKWWindowBase::PrintOptionsMenuLabel = "Print options...";
const char *vtkKWWindowBase::WindowGeometryRegKey = "WindowGeometry";

const char* vtkKWWindowBase::GetPrintOptionsMenuLabel()
{ return vtkKWWindowBase::PrintOptionsMenuLabel; }

const unsigned int vtkKWWindowBase::DefaultWidth = 900;
const unsigned int vtkKWWindowBase::DefaultHeight = 700;

vtkCxxRevisionMacro(vtkKWWindowBase, "1.1");

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindowBase );

int vtkKWWindowBaseCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWWindowBase::vtkKWWindowBase()
{
  // Menus

  this->FileMenu              = vtkKWMenu::New();
  this->HelpMenu              = vtkKWMenu::New();
  this->EditMenu              = NULL;
  this->ViewMenu              = NULL;
  this->WindowMenu            = NULL;

  // Toolbars

  this->Toolbars               = vtkKWToolbarSet::New();
  this->ToolbarsVisibilityMenu = NULL; 
  this->MenuBarSeparatorFrame  = vtkKWFrame::New();

  // Main Frame

  this->MainFrame              = vtkKWFrame::New();

  // Status frame

  this->StatusFrameSeparator  = vtkKWFrame::New();
  this->StatusFrame           = vtkKWFrame::New();
  this->StatusLabel           = vtkKWLabel::New();
  this->StatusImage           = vtkKWLabel::New();

  this->ProgressFrame         = vtkKWFrame::New();
  this->ProgressGauge         = vtkKWProgressGauge::New();

  this->TrayFrame             = vtkKWFrame::New();
  this->TrayImageError        = vtkKWLabel::New();

  this->TclInteractor         = NULL;

  this->CommandFunction       = vtkKWWindowBaseCommand;

  this->SupportHelp           = 1;
  this->SupportPrint          = 1;
  this->PromptBeforeClose     = 0;

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

  if (this->Toolbars)
    {
    this->Toolbars->Delete();
    this->Toolbars = NULL;
    }

  if (this->MenuBarSeparatorFrame)
    {
    this->MenuBarSeparatorFrame->Delete();
    this->MenuBarSeparatorFrame = NULL;
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

  if (this->ProgressFrame)
    {
    this->ProgressFrame->Delete();
    this->ProgressFrame = NULL;
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
void vtkKWWindowBase::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("class already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app, args);

  kwsys_stl::string cmd;
  kwsys_stl::string label;
  
  this->SetIconName(app->GetPrettyName());

  // Menu : File

  this->FileMenu->SetParent(this->GetMenu());
  this->FileMenu->SetTearOff(0);
  this->FileMenu->Create(app, NULL);

  this->GetMenu()->AddCascade("File", this->FileMenu, 0);

  if (this->SupportPrint)
    {
    this->FileMenu->AddCommand(
      vtkKWWindowBase::PrintOptionsMenuLabel, this, "PrintOptionsCallback", 4);
    this->FileMenu->AddSeparator();
    }

  this->FileMenu->AddCommand("Close", this, "Close", 0);
  this->FileMenu->AddCommand("Exit", this->GetApplication(), "Exit", 1);

  this->MostRecentFilesManager->SetApplication(app);

  // Menu : Help

  this->HelpMenu->SetParent(this->GetMenu());
  this->HelpMenu->SetTearOff(0);
  this->HelpMenu->Create(app, NULL);

  if (this->SupportHelp)
    {
    this->GetMenu()->AddCascade("Help", this->HelpMenu, 0);
    }

  cmd = "DisplayHelpDialog ";
  cmd += this->GetTclName();
  this->HelpMenu->AddCommand(
    "Help", this->GetApplication(), cmd.c_str(), 0);

  if (app->HasCheckForUpdates())
    {
    this->HelpMenu->AddCommand("Check For Updates", app, "CheckForUpdates", 0);
    }

  this->HelpMenu->AddSeparator();
  label = "About ";
  label += app->GetPrettyName();
  cmd = "DisplayAboutDialog ";
  cmd += this->GetTclName();
  this->HelpMenu->AddCommand(
    label.c_str(), this->GetApplication(), cmd.c_str(), 0);

  // Menubar separator

  this->MenuBarSeparatorFrame->SetParent(this);  
#if defined(_WIN32)
  this->MenuBarSeparatorFrame->Create(app, "-height 2 -bd 1 -relief groove");
#else
  this->MenuBarSeparatorFrame->Create(app, "-height 2 -bd 1 -relief sunken");
#endif

  this->Script("pack %s -side top -fill x -pady 2",
               this->MenuBarSeparatorFrame->GetWidgetName());

  // Toolbars

  this->Toolbars->SetParent(this);  
  this->Toolbars->Create(app, NULL);
  this->Toolbars->ShowBottomSeparatorOn();
  this->Toolbars->SynchronizeToolbarsVisibilityWithRegistryOn();
  this->Toolbars->SetToolbarVisibilityChangedCommand(
    this, "ToolbarVisibilityChangedCallback");
  this->Toolbars->SetNumberOfToolbarsChangedCommand(
    this, "NumberOfToolbarsChangedCallback");

  this->Script(
    "pack %s -padx 0 -pady 0 -side top -fill x -expand no",
    this->Toolbars->GetWidgetName());

  // Main frame

  this->MainFrame->SetParent(this);
  this->MainFrame->Create(app, "-bg #223344");

  this->Script("pack %s -side top -fill both -expand t",
               this->MainFrame->GetWidgetName());

  // Restore Window Geometry

  if (app->GetSaveUserInterfaceGeometry())
    {
    this->RestoreWindowGeometryFromRegistry();
    }
  else
    {
    this->SetSize(vtkKWWindowBase::DefaultWidth, vtkKWWindowBase::DefaultHeight);
    }

  // Status frame

  this->StatusFrame->SetParent(this);
  this->StatusFrame->Create(app, NULL);
  
  this->Script("pack %s -side bottom -fill x -pady 0",
               this->StatusFrame->GetWidgetName());

  // Status frame separator

  this->StatusFrameSeparator->SetParent(this);
  this->StatusFrameSeparator->Create(app, "-height 2 -bd 1");
#if defined(_WIN32)
  this->StatusFrameSeparator->ConfigureOptions("-relief groove");
#else
  this->StatusFrameSeparator->ConfigureOptions("-relief sunken");
#endif

  this->Script("pack %s -side bottom -fill x -pady 2",
               this->StatusFrameSeparator->GetWidgetName());
  
  // Status frame : image

  this->StatusImage->SetParent(this->StatusFrame);
  this->StatusImage->Create(app, "-relief sunken -bd 1");

  this->UpdateStatusImage();

  this->Script("pack %s -side left -anchor c -ipadx 1 -ipady 1 -fill y", 
               this->StatusImage->GetWidgetName());

  // Status frame : label

  this->StatusLabel->SetParent(this->StatusFrame);
  this->StatusLabel->Create(app, "-relief sunken -bd 0 -padx 3 -anchor w");

  this->Script("pack %s -side left -padx 1 -expand yes -fill both",
               this->StatusLabel->GetWidgetName());

  // Status frame : progress frame

  this->ProgressFrame->SetParent(this->StatusFrame);
  this->ProgressFrame->Create(app, "-relief sunken -bd 1");

  this->Script("pack %s -side left -padx 0 -fill y", 
               this->ProgressFrame->GetWidgetName());

  // Status frame : progress frame : gauge

  this->ProgressGauge->SetParent(this->ProgressFrame);
  this->ProgressGauge->SetLength(200);
  this->ProgressGauge->SetHeight(
    vtkKWTkUtilities::GetPhotoHeight(this->StatusImage) - 4);
  this->ProgressGauge->Create(app, NULL);

  this->Script("pack %s -side right -padx 2 -pady 2",
               this->ProgressGauge->GetWidgetName());

  // Status frame : tray frame

  this->TrayFrame->SetParent(this->StatusFrame);
  this->TrayFrame->Create(app, "-relief sunken -bd 1");

  this->Script(
   "pack %s -side left -ipadx 0 -ipady 0 -padx 0 -pady 0 -fill both",
   this->TrayFrame->GetWidgetName());

  // Status frame : tray frame : error image

  this->TrayImageError->SetParent(this->TrayFrame);
  this->TrayImageError->Create(app, NULL);

  this->TrayImageError->SetImageOption(vtkKWIcon::ICON_SMALLERRORRED);
  
  this->TrayImageError->SetBind(this, "<Button-1>", "ErrorIconCallback");

  // Udpate the enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::PrepareForDelete()
{
  // Have reference to this object:
  // this->Menu
  // this->MenuBarSeparatorFrame
  // this->Toolbars
  // this->MainFrame
  // this->StatusFrameSeparator
  // this->StatusFrame

  if (this->TclInteractor )
    {
    this->TclInteractor->SetMasterWindow(NULL);
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
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
  dialog->Create(this->GetApplication(), NULL);
  dialog->SetText("Are you sure you want to close this window?");
  dialog->SetTitle("Close");
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

  if (this->GetApplication()->GetDialogUp())
    {
    this->Script("bell");
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

  kwsys_stl::string geometry = this->GetGeometry();
  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindowBase::WindowGeometryRegKey, "%s", geometry.c_str());
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
    this->SetSize(
      vtkKWWindowBase::DefaultWidth, vtkKWWindowBase::DefaultHeight);
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
    this->EditMenu->Create(this->GetApplication(), NULL);
    this->GetMenu()->InsertCascade(1, "Edit", this->EditMenu, 0);
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
    this->ViewMenu->Create(this->GetApplication(), NULL);
    this->GetMenu()->InsertCascade(
      1 + (this->EditMenu ? 1 : 0), "View", this->ViewMenu, 0);
    }

  return this->ViewMenu;
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
    this->WindowMenu->Create(this->GetApplication(), NULL);
    this->GetMenu()->InsertCascade(
      1 + (!this->EditMenu ? 1 : 0), "Window", this->WindowMenu, 0);
    }
  
  return this->WindowMenu;
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
    this->ToolbarsVisibilityMenu->Create(this->GetApplication(), NULL);
    this->GetWindowMenu()->InsertCascade(
      2, " Toolbars", this->ToolbarsVisibilityMenu, 1, 
      "Set Toolbars Visibility");
    }
  
  return this->ToolbarsVisibilityMenu;
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::NumberOfToolbarsChangedCallback()
{
  this->Toolbars->PopulateToolbarsVisibilityMenu(
    this->GetToolbarsVisibilityMenu());
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::ToolbarVisibilityChangedCallback()
{
  this->Toolbars->UpdateToolbarsVisibilityMenu(
    this->GetToolbarsVisibilityMenu());

  this->UpdateToolbarState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::LoadScript()
{
  vtkKWLoadSaveDialog* load_dialog = vtkKWLoadSaveDialog::New();
  this->GetApplication()->RetrieveDialogLastPathRegistryValue(
    load_dialog, "LoadScriptLastPath");
  load_dialog->SetParent(this);
  load_dialog->Create(this->GetApplication(), NULL);
  load_dialog->SaveDialogOff();
  load_dialog->SetTitle("Load Script");
  load_dialog->SetDefaultExtension(this->ScriptExtension);

  kwsys_stl::string filetypes;
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
    if (!kwsys::SystemTools::FileExists(load_dialog->GetFileName()))
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
void vtkKWWindowBase::InsertRecentFilesMenu(
  int pos, vtkKWObject *target)
{
  if (!this->IsCreated() || !this->FileMenu || !this->MostRecentFilesManager)
    {
    return;
    }

  // Create the sub-menu if not done already

  vtkKWMenu *mrf_menu = this->MostRecentFilesManager->GetMenu();
  if (!mrf_menu->IsCreated())
    {
    mrf_menu->SetParent(this->FileMenu);
    mrf_menu->SetTearOff(0);
    mrf_menu->Create(this->GetApplication(), NULL);
    }

  // Remove the menu if already there (in case that function was used to
  // move the menu)

  if (this->FileMenu->HasItem(vtkKWWindowBase::PrintOptionsMenuLabel))
    {
    this->FileMenu->DeleteMenuItem(vtkKWWindowBase::PrintOptionsMenuLabel);
    }

  this->FileMenu->InsertCascade(
    pos, vtkKWWindowBase::PrintOptionsMenuLabel, mrf_menu, 6);

  // Fill the recent files vector with recent files stored in registry
  // this will also update the menu

  this->MostRecentFilesManager->SetDefaultTargetObject(target);
  this->MostRecentFilesManager->RestoreFilesListFromRegistry();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::AddRecentFile(const char *name, 
                                vtkKWObject *target,
                                const char *command)
{  
  if (this->MostRecentFilesManager)
    {
    this->MostRecentFilesManager->AddFile(name, target, command);
    this->MostRecentFilesManager->SaveFilesToRegistry();
    }
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
    return this->GetFileMenu()->GetIndex(vtkKWWindowBase::PrintOptionsMenuLabel);
    }

  // Otherwise find Close or Exit if Close was removed

  if (this->GetFileMenu()->HasItem("Close"))
    {
    return this->GetFileMenu()->GetIndex("Close");  
    }

  if (this->GetFileMenu()->HasItem("Exit"))
    {
    return this->GetFileMenu()->GetIndex("Exit");  
    }

  return this->HelpMenu->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkKWWindowBase::GetHelpMenuInsertPosition()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // Find about

  if (this->HelpMenu->HasItem("About*"))
    {
    return this->HelpMenu->GetIndex("About*") - 1;
    }

  return this->HelpMenu->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::WarningMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "Warning",
    message, vtkKWMessageDialog::WarningIcon);
  this->SetErrorIcon(vtkKWWindowBase::ERROR_ICON_RED);
  this->InvokeEvent(vtkKWEvent::WarningMessageEvent, (void*)message);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::ErrorMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "Error",
    message, vtkKWMessageDialog::ErrorIcon);
  this->SetErrorIcon(vtkKWWindowBase::ERROR_ICON_RED);
  this->InvokeEvent(vtkKWEvent::ErrorMessageEvent, (void*)message);
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::SetErrorIcon(int s)
{
  if (!this->TrayImageError || !this->TrayImageError->IsCreated())
    {
    return;
    }

  if (s > vtkKWWindowBase::ERROR_ICON_NONE) 
    {
    this->Script("pack %s -fill both -ipadx 4 -expand yes", 
                 this->TrayImageError->GetWidgetName());
    if (s == vtkKWWindowBase::ERROR_ICON_RED)
      {
      this->TrayImageError->SetImageOption(vtkKWIcon::ICON_SMALLERRORRED);
      }
    else if (s == vtkKWWindowBase::ERROR_ICON_BLACK)
      {
      this->TrayImageError->SetImageOption(vtkKWIcon::ICON_SMALLERROR);
      }
    }
  else
    {
    this->Script("pack forget %s", this->TrayImageError->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::ErrorIconCallback()
{
  this->SetErrorIcon(vtkKWWindowBase::ERROR_ICON_BLACK);
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
void vtkKWWindowBase::DisplayTclInteractor()
{
  if ( ! this->TclInteractor )
    {
    this->TclInteractor = vtkKWTclInteractor::New();
    kwsys_stl::string title;
    if (this->GetTitle())
      {
      title += this->GetTitle();
      title += " : ";
      }
    title += "Command Prompt";
    this->TclInteractor->SetTitle(title.c_str());
    this->TclInteractor->SetMasterWindow(this);
    this->TclInteractor->Create(this->GetApplication(), NULL);
    }
  
  this->TclInteractor->Display();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateStatusImage()
{
  if (!this->StatusImage || !this->StatusImage->IsCreated())
    {
    return;
    }

  // Create an empty image for now. It will be modified later on

  kwsys_stl::string image_name(
    this->Script("%s cget -image", this->StatusImage->GetWidgetName()));
  if (!image_name.size() || !*image_name.c_str())
    {
    image_name = this->Script("image create photo");
    }

  this->Script("%s configure "
               "-image %s "
               "-fg white -bg white "
               "-highlightbackground white -highlightcolor white "
               "-highlightthickness 0 "
               "-padx 0 -pady 0", 
               this->StatusImage->GetWidgetName(),
               image_name.c_str());
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
  if (this->Toolbars)
    {
    this->Toolbars->SetToolbarsFlatAspect(
      vtkKWToolbar::GetGlobalFlatAspect());
    this->Toolbars->SetToolbarsWidgetsFlatAspect(
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Update the toolbars

  if (this->Toolbars)
    {
    this->PropagateEnableState(this->Toolbars);
    this->UpdateToolbarState();
    }

  // Update the Tcl interactor

  this->PropagateEnableState(this->TclInteractor);

  // Update the window element

  this->PropagateEnableState(this->MainFrame);
  this->PropagateEnableState(this->StatusFrame);
  this->PropagateEnableState(this->MenuBarSeparatorFrame);

  // Given the state, can we close or not ?

  this->SetDeleteWindowProtocolCommand(
    this, this->GetEnabled() ? 
    "Close" : "SetStatusText \"Can not close while UI is disabled\"");

  // Update the menus

  this->UpdateMenuState();
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::UpdateMenuState()
{
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

  if (this->HelpMenu)
    {
    kwsys_stl::string about_command = "DisplayAbout ";
    about_command +=  this->GetTclName();
    int pos = this->HelpMenu->GetIndexOfCommand(
      this->GetApplication(), about_command.c_str());
    if (pos >= 0)
      {
      kwsys_stl::string label("-label {About ");
      label += this->GetApplication()->GetPrettyName();
      label += "}";
      this->HelpMenu->ConfigureItem(pos, label.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindowBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Menu: " << this->GetMenu() << endl;
  os << indent << "FileMenu: " << this->GetFileMenu() << endl;
  os << indent << "HelpMenu: " << this->GetHelpMenu() << endl;
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
  os << indent << "Toolbars: " << this->GetToolbars() << endl;
}


