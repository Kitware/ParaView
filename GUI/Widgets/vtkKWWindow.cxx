/*=========================================================================

  Module:    vtkKWWindow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWWindow.h"

#include "vtkKWApplication.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWMostRecentFilesHelper.h"
#include "vtkKWNotebook.h"
#include "vtkKWProgressGauge.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWTclInteractor.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWWidgetCollection.h"
#include "vtkKWWindowCollection.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

#define VTK_KW_HIDE_PROPERTIES_LABEL "Hide Left Panel" 
#define VTK_KW_SHOW_PROPERTIES_LABEL "Show Left Panel"
#define VTK_KW_WINDOW_DEFAULT_GEOMETRY "900x700+0+0"

vtkCxxRevisionMacro(vtkKWWindow, "1.217");
vtkCxxSetObjectMacro(vtkKWWindow, PropertiesParent, vtkKWWidget);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindow );

int vtkKWWindowCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWWindow::vtkKWWindow()
{
  this->PropertiesParent      = NULL;

  this->Menu                  = vtkKWMenu::New();
  this->MenuFile              = vtkKWMenu::New();
  this->MenuHelp              = vtkKWMenu::New();
  this->PageMenu              = vtkKWMenu::New();

  this->MenuEdit              = NULL;
  this->MenuView              = NULL;
  this->MenuWindow            = NULL;

  this->Toolbars              = vtkKWToolbarSet::New();
  this->ToolbarsMenu          = NULL; 
  this->MenuBarSeparatorFrame = vtkKWFrame::New();
  this->MiddleFrame           = vtkKWSplitFrame::New();
  this->ViewFrame             = vtkKWFrame::New();

  this->StatusFrameSeparator  = vtkKWFrame::New();
  this->StatusFrame           = vtkKWFrame::New();
  this->StatusLabel           = vtkKWLabel::New();
  this->StatusImage           = vtkKWLabel::New();
  this->StatusImageName       = NULL;

  this->ProgressFrame         = vtkKWFrame::New();
  this->ProgressGauge         = vtkKWProgressGauge::New();

  this->TrayFrame             = vtkKWFrame::New();
  this->TrayImageError        = vtkKWLabel::New();

  this->Notebook              = vtkKWNotebook::New();

  this->ExitDialogWidget      = NULL;

  this->TclInteractor         = NULL;

  this->CommandFunction       = vtkKWWindowCommand;

  this->PrintTargetDPI        = 100;
  this->SupportHelp           = 1;
  this->SupportPrint           = 1;
  this->WindowClass           = NULL;
  this->Title                 = NULL;
  this->PromptBeforeClose     = 1;
  this->ScriptExtension       = 0;
  this->ScriptType            = 0;

  this->InExit                = 0;

  this->MostRecentFilesHelper = vtkKWMostRecentFilesHelper::New();

  this->SetWindowClass("KitwareWidget");
  this->SetScriptExtension(".tcl");
  this->SetScriptType("Tcl");
}

//----------------------------------------------------------------------------
vtkKWWindow::~vtkKWWindow()
{
  if (this->TclInteractor)
    {
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
    }

  this->Notebook->Delete();
  this->SetPropertiesParent(NULL);

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION <= 2)
  // This "hack" is here to get around a Tk bug ( Bug: 3402 )
  // in tkMenu.c
  vtkKWMenu* menuparent = vtkKWMenu::SafeDownCast(this->PageMenu->GetParent());
  if (menuparent)
    {
    menuparent->DeleteMenuItem(VTK_KW_PAGE_SETUP_MENU_LABEL);
    }
#endif

  this->Menu->Delete();
  this->PageMenu->Delete();
  this->MenuFile->Delete();
  this->MenuHelp->Delete();
  this->Toolbars->Delete();
  this->MenuBarSeparatorFrame->Delete();
  this->ViewFrame->Delete();
  this->MiddleFrame->Delete();
  this->StatusFrameSeparator->Delete();
  this->StatusFrame->Delete();
  this->StatusImage->Delete();
  this->StatusLabel->Delete();
  this->ProgressFrame->Delete();
  this->ProgressGauge->Delete();
  this->TrayFrame->Delete();
  this->TrayImageError->Delete();
  
  if (this->MenuEdit)
    {
    this->MenuEdit->Delete();
    }
  if (this->MenuView)
    {
    this->MenuView->Delete();
    }
  if (this->MenuWindow)
    {
    this->MenuWindow->Delete();
    }
  if (this->ToolbarsMenu)
    {
    this->ToolbarsMenu->Delete();
    this->ToolbarsMenu = NULL;
    }
  this->SetStatusImageName(0);
  this->SetWindowClass(0);
  this->SetTitle(0);
  this->SetScriptExtension(0);
  this->SetScriptType(0);
  this->MostRecentFilesHelper->Delete();
}

//----------------------------------------------------------------------------
void vtkKWWindow::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to set the appropriate flags

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  const char *wname = this->GetWidgetName();

  this->Script("toplevel %s -visual best %s -class %s",
               wname, (args ? args : ""), this->WindowClass);

  this->Script("wm title %s {%s}", 
               wname, this->GetTitle());

  this->Script("wm iconname %s {%s}",
               wname, app->GetApplicationPrettyName());

  // Set up standard menus

  this->Menu->SetParent(this);
  this->Menu->SetTearOff(0);
  this->Menu->Create(app, "");

  this->InstallMenu(this->Menu);

  // Menu : File

  this->MenuFile->SetParent(this->Menu);
  this->MenuFile->SetTearOff(0);
  this->MenuFile->Create(app, "");

  // Menu : Print quality

  this->PageMenu->SetParent(this->MenuFile);
  this->PageMenu->SetTearOff(0);
  this->PageMenu->Create(app, "");

  char* rbv = 
    this->PageMenu->CreateRadioButtonVariable(this, "PageSetup");

  this->Script( "set %s 0", rbv );
  this->PageMenu->AddRadioButton(0, "100 DPI", rbv, this, "OnPrint 1 0", 0);
  this->PageMenu->AddRadioButton(1, "150 DPI", rbv, this, "OnPrint 1 1", 1);
  this->PageMenu->AddRadioButton(2, "300 DPI", rbv, this, "OnPrint 1 2", 0);
  delete [] rbv;

  // Menu : File (cont.)

  this->Menu->AddCascade("File", this->MenuFile, 0);

  if (this->SupportPrint)
    {
    this->MenuFile->AddCascade(
      VTK_KW_PAGE_SETUP_MENU_LABEL, this->PageMenu, 8);
    this->MenuFile->AddSeparator();
    }

  this->MenuFile->AddCommand("Close", this, "Close", 0);
  this->MenuFile->AddCommand("Exit", this, "Exit", 1);

  this->MostRecentFilesHelper->SetApplication(app);

  // Menu : Window : Properties panel

  this->GetMenuWindow()->AddCommand(VTK_KW_HIDE_PROPERTIES_LABEL, this,
                                    "TogglePropertiesVisibilityCallback", 1 );

  // Help menu

  this->MenuHelp->SetParent(this->Menu);
  this->MenuHelp->SetTearOff(0);
  this->MenuHelp->Create(app, "");

  if (this->SupportHelp)
    {
    this->Menu->AddCascade("Help", this->MenuHelp, 0);
    }
  this->MenuHelp->AddCommand("Help", this, "DisplayHelp", 0);

  if (app->HasCheckForUpdates())
    {
    this->MenuHelp->AddCommand("Check For Updates", app, "CheckForUpdates", 0);
    }

  this->MenuHelp->AddSeparator();
  ostrstream about_label;
  about_label << "About " << app->GetApplicationPrettyName() << ends;
  this->MenuHelp->AddCommand(about_label.str(), this, "DisplayAbout", 0);
  about_label.rdbuf()->freeze(0);

  // Menubar separator

  this->MenuBarSeparatorFrame->SetParent(this);  
#if defined(_WIN32)
  this->MenuBarSeparatorFrame->Create(app, "-height 2 -bd 1 -relief groove");
#else
  this->MenuBarSeparatorFrame->Create(app, "-height 2 -bd 1 -relief sunken");
#endif

  this->Script("pack %s -side top -fill x -pady 2",
               this->MenuBarSeparatorFrame->GetWidgetName());

  // Toolbar frame

  this->Toolbars->SetParent(this);  
  this->Toolbars->Create(app, "");
  this->Toolbars->ShowBottomSeparatorOn();


  // Split frame
  this->MiddleFrame->SetParent(this);
  this->MiddleFrame->Create(app);

  this->Script("pack %s -side top -fill both -expand t",
               this->MiddleFrame->GetWidgetName());

  // Split frame : view frame

  this->ViewFrame->SetParent(this->MiddleFrame->GetFrame2());
  this->ViewFrame->Create(app, "");

  this->Script("pack %s -side right -fill both -expand yes",
               this->ViewFrame->GetWidgetName());

  // Restore Window Geometry

  if (app->GetSaveWindowGeometry())
    {
    this->RestoreWindowGeometry();
    }
  else
    {
    this->Script("wm geometry %s %s", 
                 this->GetWidgetName(), VTK_KW_WINDOW_DEFAULT_GEOMETRY);
    }

  // Window properties / Application settings (leading to preferences)

  this->CreateDefaultPropertiesParent();

  // Create the notebook

  this->Notebook->SetParent(this->GetPropertiesParent());
  this->Notebook->Create(app, "");
  this->Notebook->AlwaysShowTabsOn();

  // Status frame separator

  this->StatusFrameSeparator->SetParent(this);
  this->StatusFrameSeparator->Create(app, "-height 2 -bd 1");
#if defined(_WIN32)
  this->StatusFrameSeparator->ConfigureOptions("-relief groove");
#else
  this->StatusFrameSeparator->ConfigureOptions("-relief sunken");
#endif

  this->Script("pack %s -side top -fill x -pady 2",
               this->StatusFrameSeparator->GetWidgetName());
  
  // Status frame

  this->StatusFrame->SetParent(this);
  this->StatusFrame->Create(app, NULL);
  
  this->Script("pack %s -side top -fill x -pady 0",
               this->StatusFrame->GetWidgetName());
  
  // Status frame : image

  this->SetStatusImageName(this->Script("image create photo"));
  this->CreateStatusImage();

  this->StatusImage->SetParent(this->StatusFrame);
  this->StatusImage->Create(app, "-relief sunken -bd 1");

  this->Script("%s configure -image %s -fg white -bg white "
               "-highlightbackground white -highlightcolor white "
               "-highlightthickness 0 -padx 0 -pady 0", 
               this->StatusImage->GetWidgetName(),
               this->StatusImageName);

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
    vtkKWTkUtilities::GetPhotoHeight(
      app->GetMainInterp(), this->StatusImageName) - 4);
  this->ProgressGauge->Create(app, "");

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
  this->TrayImageError->Create(app, "");

  this->TrayImageError->SetImageOption(vtkKWIcon::ICON_SMALLERRORRED);
  
  this->TrayImageError->SetBind(this, "<Button-1>", "ProcessErrorClick");

  // If we have a User Interface Manager, it's time to create it

  vtkKWUserInterfaceManager *uim = this->GetUserInterfaceManager();
  if (uim && !uim->IsCreated())
    {
    uim->Create(app);
    }

  if (this->GetApplication()->HasRegistryValue(
        2, "RunTime", VTK_KW_SHOW_MOST_RECENT_PANELS_REG_KEY) &&
      !this->GetIntRegistryValue(
        2, "RunTime", VTK_KW_SHOW_MOST_RECENT_PANELS_REG_KEY))
    {
    this->ShowMostRecentPanels(0);
    }
  else
    {
    this->ShowMostRecentPanels(1);
    }

  vtkKWUserInterfaceNotebookManager *uim_nb = 
    vtkKWUserInterfaceNotebookManager::SafeDownCast(uim);
  if (uim_nb)
    {
    if (this->GetApplication()->HasRegistryValue(
          2, "RunTime", VTK_KW_ENABLE_GUI_DRAG_AND_DROP_REG_KEY))
      {
      uim_nb->SetEnableDragAndDrop(
        this->GetIntRegistryValue(
          2, "RunTime", VTK_KW_ENABLE_GUI_DRAG_AND_DROP_REG_KEY));
      }
    }

  // Udpate the enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWindow::DisplayHelp()
{
  this->GetApplication()->DisplayHelp(this);
}

//----------------------------------------------------------------------------
void vtkKWWindow::CreateDefaultPropertiesParent()
{
  if (!this->PropertiesParent)
    {
    vtkKWWidget *pp = vtkKWWidget::New();
    pp->SetParent(this->MiddleFrame->GetFrame1());
    pp->Create(this->GetApplication(),"frame","-bd 0");
    this->Script("pack %s -side left -fill both -expand t -anchor nw",
                 pp->GetWidgetName());
    this->SetPropertiesParent(pp);
    pp->Delete();
    }
  else
    {
    vtkDebugMacro("Properties Parent already set for Window");
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Exit()
{  
  if (this->GetApplication()->GetDialogUp())
    {
    this->Script("bell");
    return;
    }
  if (!this->InExit)
    {
    this->InExit = 1;
    }
  else
    {
    return;
    }
  if ( this->ExitDialog() )
    {
    this->PromptBeforeClose = 0;
    this->GetApplication()->Exit();
    }
  else
    {
    this->InExit = 0;
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Close()
{
  if (this->GetApplication()->GetDialogUp())
    {
    this->Script("bell");
    return;
    }
  if (this->PromptBeforeClose &&
      this->GetApplication()->GetWindows()->GetNumberOfItems() <= 1)
    {
    if ( !this->ExitDialog() )
      {
      return;
      }
    }
  this->CloseNoPrompt();
}

//----------------------------------------------------------------------------
void vtkKWWindow::CloseNoPrompt()
{
  if (this->TclInteractor )
    {
    this->TclInteractor->SetMasterWindow(NULL);
    this->TclInteractor->Delete();
    this->TclInteractor = NULL;
    }

  // Save its geometry

  if (this->GetApplication()->GetSaveWindowGeometry())
    {
    this->SaveWindowGeometry();
    }

  // Close this window in the application. The
  // application will exit if there are no more windows.
  this->GetApplication()->Close(this);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SaveWindowGeometry()
{
  if (this->IsCreated())
    {
    this->Script("wm geometry %s", this->GetWidgetName());

    this->GetApplication()->SetRegistryValue(
      2, "Geometry", VTK_KW_WINDOW_GEOMETRY_REG_KEY, "%s", 
      this->GetApplication()->GetMainInterp()->result);

    this->GetApplication()->SetRegistryValue(
      2, "Geometry", VTK_KW_WINDOW_FRAME1_SIZE_REG_KEY, "%d", 
      this->MiddleFrame->GetFrame1Size());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::RestoreWindowGeometry()
{
  if (this->IsCreated())
    {
    if (this->GetApplication()->HasRegistryValue(
          2, "Geometry", VTK_KW_WINDOW_GEOMETRY_REG_KEY))
      {
      char geometry[40];
      if (this->GetApplication()->GetRegistryValue(
            2, "Geometry", VTK_KW_WINDOW_GEOMETRY_REG_KEY, geometry))
        {
        this->Script("wm geometry %s %s", this->GetWidgetName(), geometry);
        }
      }
    else
      {
      this->Script("wm geometry %s %s", 
                   this->GetWidgetName(), VTK_KW_WINDOW_DEFAULT_GEOMETRY);
      }

    if (this->GetApplication()->HasRegistryValue(
          2, "Geometry", VTK_KW_WINDOW_FRAME1_SIZE_REG_KEY))
      {
      int reg_size = this->GetApplication()->GetIntRegistryValue(
        2, "Geometry", VTK_KW_WINDOW_FRAME1_SIZE_REG_KEY);
      if (reg_size >= this->MiddleFrame->GetFrame1MinimumSize())
        {
        this->MiddleFrame->SetFrame1Size(reg_size);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Render()
{
}

//----------------------------------------------------------------------------
void vtkKWWindow::DisplayAbout()
{
  this->GetApplication()->DisplayAbout(this);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetStatusText(const char *text)
{
  this->StatusLabel->SetText(text);
}

//----------------------------------------------------------------------------
const char *vtkKWWindow::GetStatusText()
{
  return this->StatusLabel->GetText();
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindow::GetMenuEdit()
{
  if (this->MenuEdit)
    {
    return this->MenuEdit;
    }

  if ( !this->IsCreated() )
    {
    return 0;
    }
  
  this->MenuEdit = vtkKWMenu::New();
  this->MenuEdit->SetParent(this->GetMenu());
  this->MenuEdit->SetTearOff(0);
  this->MenuEdit->Create(this->GetApplication(),"");
  // Make sure Edit menu is next to file menu
  this->Menu->InsertCascade(1, "Edit", this->MenuEdit, 0);
  return this->MenuEdit;
}

//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindow::GetMenuView()
{
  if (this->MenuView)
    {
    return this->MenuView;
    }

  if ( !this->IsCreated() )
    {
    return 0;
    }

  this->MenuView = vtkKWMenu::New();
  this->MenuView->SetParent(this->GetMenu());
  this->MenuView->SetTearOff(0);
  this->MenuView->Create(this->GetApplication(), "");
  // make sure Help menu is on the right
  if (this->MenuEdit)
    { 
    this->Menu->InsertCascade(2, "View", this->MenuView, 0);
    }
  else
    {
    this->Menu->InsertCascade(1, "View", this->MenuView, 0);
    }
  
  return this->MenuView;
}

//----------------------------------------------------------------------------
void vtkKWWindow::AddToolbar(vtkKWToolbar* toolbar, const char* name,
  int visibility /*=1*/)
{
  if (!this->Toolbars->AddToolbar(toolbar))
    {
    return;
    }
  int id = this->Toolbars->GetNumberOfToolbars() - 1; 
  ostrstream command;
  command << "ToggleToolbarVisibility " << id << " " << name << ends;
  this->AddToolbarToMenu(toolbar, name, this, command.str());
  command.rdbuf()->freeze(0);

  // Restore state from registry.
  ostrstream reg_key;
  reg_key << name << "_ToolbarVisibility" << ends;
  if (this->GetApplication()->GetRegistryValue(2, "RunTime", reg_key.str(), 0))
    {
    visibility = this->GetApplication()->GetIntRegistryValue(2, "RunTime", reg_key.str());
    }
  this->SetToolbarVisibility(toolbar, name, visibility);
  reg_key.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWWindow::AddToolbarToMenu(vtkKWToolbar*, const char* name,
  vtkKWWidget* target, const char* command)
{
  if (!this->ToolbarsMenu)
    {
    this->ToolbarsMenu = vtkKWMenu::New();
    this->ToolbarsMenu->SetParent(this->GetMenuWindow());
    this->ToolbarsMenu->SetTearOff(0);
    this->ToolbarsMenu->Create(this->GetApplication(), "");
    this->GetMenuWindow()->InsertCascade(2, " Toolbars", this->ToolbarsMenu, 1,
      "Customize Toolbars");
    }
  char *rbv = this->ToolbarsMenu->CreateCheckButtonVariable(this, name);

  this->ToolbarsMenu->AddCheckButton(
    name, rbv, target, command, "Show/Hide this toolbar");

  delete [] rbv;
  
  this->ToolbarsMenu->CheckCheckButton(this, name, 1); 
}

//----------------------------------------------------------------------------
void vtkKWWindow::HideToolbar(vtkKWToolbar* toolbar, const char* name)
{
  this->SetToolbarVisibility(toolbar, name, 0);
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowToolbar(vtkKWToolbar* toolbar, const char* name)
{
  this->SetToolbarVisibility(toolbar, name, 1);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetToolbarVisibility(vtkKWToolbar* toolbar, const char* name, int flag)
{
  this->Toolbars->SetToolbarVisibility(toolbar, flag);
  this->SetToolbarVisibilityInternal(toolbar, name, flag); 
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetToolbarVisibilityInternal(vtkKWToolbar* ,
  const char* name, int flag)
{
  if (this->ToolbarsMenu)
    {
    this->ToolbarsMenu->CheckCheckButton(this, name, flag);
    }
  ostrstream reg_key;
  reg_key << name << "_ToolbarVisibility" << ends;
  this->GetApplication()->SetRegistryValue(2, "RunTime", reg_key.str(),
    "%d", flag);
  reg_key.rdbuf()->freeze(0); 
  this->UpdateToolbarAspect();
}

//----------------------------------------------------------------------------
void vtkKWWindow::ToggleToolbarVisibility(int id, const char* name)
{
  vtkKWToolbar* toolbar = this->Toolbars->GetToolbar(id);
  if (!toolbar)
    {
    return;
    }
  int new_visibility = (this->Toolbars->IsToolbarVisible(toolbar))? 0 : 1;
  this->SetToolbarVisibility(toolbar, name, new_visibility);
}
//----------------------------------------------------------------------------
vtkKWMenu *vtkKWWindow::GetMenuWindow()
{
  if (this->MenuWindow)
    {
    return this->MenuWindow;
    }

  if ( !this->IsCreated() )
    {
    return 0;
    }

  this->MenuWindow = vtkKWMenu::New();
  this->MenuWindow->SetParent(this->GetMenu());
  this->MenuWindow->SetTearOff(0);
  this->MenuWindow->Create(this->GetApplication(), "");
  // make sure Help menu is on the right
  if (this->MenuEdit)
    { 
    this->Menu->InsertCascade(1, "Window", this->MenuWindow, 0);
    }
  else
    {
    this->Menu->InsertCascade(2, "Window", this->MenuWindow, 0);
    }
  
  return this->MenuWindow;
}

//----------------------------------------------------------------------------
void vtkKWWindow::OnPrint(int propagate, int res)
{
  int dpis[] = { 100, 150, 300 };
  this->PrintTargetDPI = dpis[res];
  if ( propagate )
    {
    float dpi = res;
    this->InvokeEvent(vtkKWEvent::PrinterDPIChangedEvent, &dpi);
    }
  else
    {
    char array[][20] = { "100 DPI", "150 DPI", "300 DPI" };
    this->PageMenu->Invoke( this->PageMenu->GetIndex(array[res]) );
    }
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetPropertiesVisiblity()
{
  return (this->MiddleFrame && this->MiddleFrame->GetFrame1Visibility() ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetPropertiesVisiblity(int arg)
{
  if (arg)
    {
    if (!this->GetPropertiesVisiblity())
      {
      this->MiddleFrame->Frame1VisibilityOn();
      this->Script("%s entryconfigure 0 -label {%s}",
                   this->GetMenuWindow()->GetWidgetName(),
                   VTK_KW_HIDE_PROPERTIES_LABEL);
      }
    }
  else
    {
    if (this->GetPropertiesVisiblity())
      {
      this->MiddleFrame->Frame1VisibilityOff();
      this->Script("%s entryconfigure 0 -label {%s}",
                   this->GetMenuWindow()->GetWidgetName(),
                   VTK_KW_SHOW_PROPERTIES_LABEL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::TogglePropertiesVisibilityCallback()
{
  int arg = !this->GetPropertiesVisiblity();
  this->SetPropertiesVisiblity(arg);
  float farg = arg;
  this->InvokeEvent(vtkKWEvent::UserInterfaceVisibilityChangedEvent, &farg);
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowWindowProperties()
{
  this->ShowProperties();
  
  // Forget current props and pack the notebook

  this->Notebook->UnpackSiblings();

  this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWWindow::ShowApplicationSettingsInterface()
{
  if (this->GetApplicationSettingsInterface())
    {
    this->ShowWindowProperties();
    return this->GetApplicationSettingsInterface()->Raise();
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowMostRecentPanels(int arg)
{
  if (arg)
    {
    this->Notebook->ShowAllPagesWithSameTagOff();
    this->Notebook->ShowOnlyPagesWithSameTagOff();
    this->Notebook->SetNumberOfMostRecentPages(4);
    this->Notebook->ShowOnlyMostRecentPagesOn();
    }
  else
    {
    this->Notebook->ShowAllPagesWithSameTagOff();
    this->Notebook->ShowOnlyMostRecentPagesOff();
    this->Notebook->ShowOnlyPagesWithSameTagOn();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::InstallMenu(vtkKWMenu* menu)
{ 
  this->Script("%s configure -menu %s", this->GetWidgetName(),
               menu->GetWidgetName());  
}

//----------------------------------------------------------------------------
void vtkKWWindow::UnRegister(vtkObjectBase *o)
{
  if (!this->DeletingChildren)
    {
    // delete the children if we are about to be deleted
    if (this->ReferenceCount == 1 +
        (this->HasChildren() ? this->GetChildren()->GetNumberOfItems() : 0))
      {
      if (!((this->HasChildren() && 
             this->GetChildren()->IsItemPresent((vtkKWWidget *)o))))
        {
        vtkKWWidget *child;
        
        this->DeletingChildren = 1;
        if (this->HasChildren())
          {
          vtkKWWidgetCollection *children = this->GetChildren();
          children->InitTraversal();
          while ((child = children->GetNextKWWidget()))
            {
            child->SetParent(NULL);
            }
          }
        this->DeletingChildren = 0;
        }
      }
    }
  
  this->Superclass::UnRegister(o);
}

//----------------------------------------------------------------------------
void vtkKWWindow::LoadScript()
{
  vtkKWLoadSaveDialog* loadScriptDialog = vtkKWLoadSaveDialog::New();
  this->RetrieveLastPath(loadScriptDialog, "LoadScriptLastPath");
  loadScriptDialog->SetParent(this);
  loadScriptDialog->Create(this->GetApplication(),"");
  loadScriptDialog->SaveDialogOff();
  loadScriptDialog->SetTitle("Load Script");
  loadScriptDialog->SetDefaultExtension(this->ScriptExtension);
  ostrstream filetypes;
  filetypes << "{{" << this->ScriptType << " Scripts} {" 
            << this->ScriptExtension << "}} {{All Files} {.*}}" << ends;
  loadScriptDialog->SetFileTypes(filetypes.str());
  filetypes.rdbuf()->freeze(0);

  int enabled = this->GetEnabled();
  this->SetEnabled(0);
  if (loadScriptDialog->Invoke() && 
      loadScriptDialog->GetFileName() && 
      strlen(loadScriptDialog->GetFileName()) > 0)
    {
    FILE *fin = fopen(loadScriptDialog->GetFileName(), "r");
    if (!fin)
      {
      vtkWarningMacro("Unable to open script file!");
      }
    else
      {
      this->SaveLastPath(loadScriptDialog, "LoadScriptLastPath");
      this->LoadScript(loadScriptDialog->GetFileName());
      }
    }
  this->SetEnabled(enabled);
  loadScriptDialog->Delete();
}

//----------------------------------------------------------------------------
void vtkKWWindow::LoadScript(const char *path)
{
  this->Script("set InitialWindow %s", this->GetTclName());
  this->GetApplication()->LoadScript(path);
}

//----------------------------------------------------------------------------
void vtkKWWindow::AddRecentFilesMenu(
  const char *menuEntry, vtkKWObject *target, const char *label, int underline)
{
  if (!this->IsCreated() || !label || !this->MenuFile || 
      !this->MostRecentFilesHelper)
    {
    return;
    }

  // Create the menu if not done already

  vtkKWMenu *mrf_menu = 
    this->MostRecentFilesHelper->GetMostRecentFilesMenu();
  if (!mrf_menu->IsCreated())
    {
    mrf_menu->SetParent(this->MenuFile);
    mrf_menu->SetTearOff(0);
    mrf_menu->Create(this->GetApplication(), NULL);
    }

  // Remove the menu if already there (in case that function was used to
  // move the menu)

  if (this->MenuFile->HasItem(label))
    {
    this->MenuFile->DeleteMenuItem(label);
    }

  // Find where to insert

  int insert_idx;
  if (!menuEntry || !this->MenuFile->HasItem(menuEntry))
    {
    insert_idx = this->GetFileMenuIndex();
    }
  else
    {
    insert_idx = this->MenuFile->GetIndex(menuEntry) - 1;
    }
  this->MenuFile->InsertCascade(insert_idx, label, mrf_menu, underline);

  // Fill the recent files vector with recent files stored in registry
  // this will also update the menu

  this->MostRecentFilesHelper->SetDefaultTargetObject(target);
  this->MostRecentFilesHelper->LoadMostRecentFilesFromRegistry();
}

//----------------------------------------------------------------------------
void vtkKWWindow::AddRecentFile(const char *name, 
                                vtkKWObject *target,
                                const char *command)
{  
  if (this->MostRecentFilesHelper)
    {
    this->MostRecentFilesHelper->AddMostRecentFile(name, target, command);
    this->MostRecentFilesHelper->SaveMostRecentFilesToRegistry();
    }
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetFileMenuIndex()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }

  // First find the print-related menu commands

  if (this->GetMenuFile()->HasItem(VTK_KW_PAGE_SETUP_MENU_LABEL))
    {
    return this->GetMenuFile()->GetIndex(VTK_KW_PAGE_SETUP_MENU_LABEL);
    }

  // Otherwise find Close or Exit if Close was removed

  int clidx;
  if (this->GetMenuFile()->HasItem("Close"))
    {
    clidx = this->GetMenuFile()->GetIndex("Close");  
    }
  else
    {
    clidx = this->GetMenuFile()->GetIndex("Exit");  
    }

  return clidx;  
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetHelpMenuIndex()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // Find about

  if (this->MenuHelp->HasItem("About*"))
    {
    return this->MenuHelp->GetIndex("About*") - 1;
    }

  return this->MenuHelp->GetNumberOfItems();
}

//----------------------------------------------------------------------------
int vtkKWWindow::ExitDialog()
{
  this->GetApplication()->SetBalloonHelpWidget(0);
  if ( this->ExitDialogWidget )
    {
    return 1;
    }
  ostrstream title;
  title << "Exit " << this->GetApplication()->GetApplicationPrettyName() 
        << ends;
  ostrstream str;
  str << "Are you sure you want to exit " 
      << this->GetApplication()->GetApplicationPrettyName() << "?" << ends;
  
  vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
  this->ExitDialogWidget = dlg2;
  dlg2->SetStyleToYesNo();
  dlg2->SetMasterWindow(this);
  dlg2->SetOptions(
     vtkKWMessageDialog::QuestionIcon | vtkKWMessageDialog::RememberYes |
     vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dlg2->SetDialogName(VTK_KW_EXIT_DIALOG_NAME);
  dlg2->Create(this->GetApplication(),"");
  dlg2->SetText( str.str() );
  dlg2->SetTitle( title.str() );
  int ret = dlg2->Invoke();
  this->ExitDialogWidget = 0;
  dlg2->Delete();

  str.rdbuf()->freeze(0);
  title.rdbuf()->freeze(0);
 
  vtkKWApplicationSettingsInterface *asi =  
    this->GetApplicationSettingsInterface();
  if (asi)
    {
    asi->Update();
    }
 
  return ret;
}

//----------------------------------------------------------------------------
float vtkKWWindow::GetFloatRegistryValue(int level, const char* subkey, 
                                          const char* key)
{
  return this->GetApplication()->GetFloatRegistryValue(level, subkey, key);
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetIntRegistryValue(int level, const char* subkey, 
                                      const char* key)
{
  return this->GetApplication()->GetIntRegistryValue(level, subkey, key);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SaveLastPath(vtkKWLoadSaveDialog *dialog, const char* key)
{
  //  "OpenDirectory"
  if ( dialog->GetLastPath() )
    {
    this->GetApplication()->SetRegistryValue(
      1, "RunTime", key, dialog->GetLastPath());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::RetrieveLastPath(vtkKWLoadSaveDialog *dialog, const char* key)
{
  char buffer[1024];
  if ( this->GetApplication()->GetRegistryValue(1, "RunTime", key, buffer) )
    {
    if ( *buffer )
      {
      dialog->SetLastPath( buffer );
      }  
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::SaveColor(int level, const char* key, double rgb[3])
{
  this->GetApplication()->SetRegistryValue(
    level, "Colors", key, "Color: %lf %lf %lf", rgb[0], rgb[1], rgb[2]);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SaveColor(int level, const char* key, float rgb[3])
{
  double drgb[3];
  drgb[0] = rgb[0];
  drgb[1] = rgb[1];
  drgb[2] = rgb[2];
  this->SaveColor(level, key, drgb);
}

//----------------------------------------------------------------------------
int vtkKWWindow::RetrieveColor(int level, const char* key, double rgb[3])
{
  char buffer[1024];
  rgb[0] = -1;
  rgb[1] = -1;
  rgb[2] = -1;

  int ok = 0;
  if (this->GetApplication()->GetRegistryValue(
        level, "Colors", key, buffer) )
    {
    if (*buffer)
      {      
      sscanf(buffer, "Color: %lf %lf %lf", rgb, rgb+1, rgb+2);
      ok = 1;
      }
    }
  return ok;
}

//----------------------------------------------------------------------------
int vtkKWWindow::RetrieveColor(int level, const char* key, float rgb[3])
{
  double drgb[3];
  int res = this->RetrieveColor(level, key, drgb);
  rgb[0] = drgb[0];
  rgb[1] = drgb[1];
  rgb[2] = drgb[2];
  return res;
}

//----------------------------------------------------------------------------
int vtkKWWindow::BooleanRegistryCheck(int level, 
                                       const char* subkey,
                                       const char* key, 
                                       const char* trueval)
{
  return this->GetApplication()->BooleanRegistryCheck(level, subkey, key, trueval);
}


//----------------------------------------------------------------------------
void vtkKWWindow::WarningMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "VTK Warning",
    message, vtkKWMessageDialog::WarningIcon);
  this->SetErrorIcon(2);
  this->InvokeEvent(vtkKWEvent::WarningMessageEvent, (void*)message);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetErrorIcon(int s)
{
  if (s) 
    {
    this->Script("pack %s -fill both -ipadx 4 -expand yes", 
                 this->TrayImageError->GetWidgetName());
    if ( s > 1 )
      {
      this->TrayImageError->SetImageOption(vtkKWIcon::ICON_SMALLERRORRED);
      }
    else
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
void vtkKWWindow::ErrorMessage(const char* message)
{
  vtkKWMessageDialog::PopupMessage(
    this->GetApplication(), this, "VTK Error",
    message, vtkKWMessageDialog::ErrorIcon);
  this->SetErrorIcon(2);
  this->InvokeEvent(vtkKWEvent::ErrorMessageEvent, (void*)message);
}

//----------------------------------------------------------------------------
void vtkKWWindow::ProcessErrorClick()
{
  this->SetErrorIcon(1);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetTitle (const char* _arg)
{
  if (this->Title == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Title && _arg && (!strcmp(this->Title, _arg))) 
    {
    return;
    }

  if (this->Title) 
    { 
    delete [] this->Title; 
    }

  if (_arg)
    {
    this->Title = new char[strlen(_arg)+1];
    strcpy(this->Title, _arg);
    }
  else
    {
    this->Title = NULL;
    }

  this->Modified();

  if (this->IsCreated() && this->Title)
    {
    this->Script("wm title %s {%s}", this->GetWidgetName(), this->GetTitle());
    }
} 

//----------------------------------------------------------------------------
char* vtkKWWindow::GetTitle()
{
  if (!this->Title && 
      this->GetApplication() && 
      this->GetApplication()->GetApplicationName())
    {
    return this->GetApplication()->GetApplicationName();
    }
  return this->Title;
}

//----------------------------------------------------------------------------
void vtkKWWindow::DisplayCommandPrompt()
{
  if ( ! this->TclInteractor )
    {
    this->TclInteractor = vtkKWTclInteractor::New();
    ostrstream title;
    if (this->GetTitle())
      {
      title << this->GetTitle() << " : ";
      }
    title << "Command Prompt" << ends;
    this->TclInteractor->SetTitle(title.str());
    title.rdbuf()->freeze(0);
    this->TclInteractor->SetMasterWindow(this);
    this->TclInteractor->Create(this->GetApplication());
    }
  
  this->TclInteractor->Display();
}

//----------------------------------------------------------------------------
void vtkKWWindow::ProcessEvent( vtkObject* object,
                                unsigned long event, 
                                void *clientdata, void *calldata)
{   
  float *fArgs = 0;
  if (calldata)
    {    
    fArgs = reinterpret_cast<float *>(calldata);
    }

  vtkKWWindow *self = reinterpret_cast<vtkKWWindow *>(clientdata);
  self->InternalProcessEvent(object, event, fArgs, calldata);
}

//----------------------------------------------------------------------------
void vtkKWWindow::InternalProcessEvent(vtkObject*, unsigned long, 
                                       float *, void *)
{
  // No implementation
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateToolbarAspect()
{
  if (!this->Toolbars)
    {
    return;
    }

  int flat_frame;
  if (this->GetApplication()->HasRegistryValue(
    2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY))
    {
    flat_frame = this->GetApplication()->GetIntRegistryValue(
      2, "RunTime", VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY);
    }
  else
    {
    flat_frame = vtkKWToolbar::GetGlobalFlatAspect();
    }

  int flat_buttons;
  if (this->GetApplication()->HasRegistryValue(
    2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY))
    {
    flat_buttons = this->GetApplication()->GetIntRegistryValue(
      2, "RunTime", VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY);
    }
  else
    {
    flat_buttons = vtkKWToolbar::GetGlobalWidgetsFlatAspect();
    }

  this->Toolbars->SetToolbarsFlatAspect(flat_frame);
  this->Toolbars->SetToolbarsWidgetsFlatAspect(flat_buttons);

  // The split frame packing mechanism is so weird that I will have
  // to unpack the toolbar frame myself in case it's empty, otherwise
  // the middle frame won't claim the space used by the toolbar frame

  if (this->Toolbars->IsCreated())
    {
    if (this->Toolbars->GetNumberOfVisibleToolbars())
      {
      this->Script(
        "pack %s -padx 0 -pady 0 -side top -fill x -expand no -after %s",
        this->Toolbars->GetWidgetName(),
        this->MenuBarSeparatorFrame->GetWidgetName());
      this->Toolbars->PackToolbars();
      }
    else
      {
      this->Toolbars->Unpack();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Update the toolbars

  this->PropagateEnableState(this->Toolbars);

  // Update the notebook

  this->PropagateEnableState(this->Notebook);

  // Update the Tcl interactor

  this->PropagateEnableState(this->TclInteractor);

  this->PropagateEnableState(this->MiddleFrame);
  this->PropagateEnableState(this->StatusFrame);
  //this->PropagateEnableState(this->StatusLabel);
  this->PropagateEnableState(this->PropertiesParent);
  this->PropagateEnableState(this->ViewFrame);
  this->PropagateEnableState(this->MenuBarSeparatorFrame);

  // Do not disable the status image, it has not functionality attached 
  // anyway, and is used to display the application logo: disabling it 
  // makes it look ugly.
  //this->PropagateEnableState(this->StatusImage);

  // Given the state, can we close or not ?

  if (this->IsCreated())
    {
    if (this->Enabled)
      {
      this->Script("wm protocol %s WM_DELETE_WINDOW {%s Close}",
                   this->GetWidgetName(), this->GetTclName());
      }
    else
      {
      this->Script("wm protocol %s WM_DELETE_WINDOW "
                   "{%s SetStatusText \"Can not close while UI is disabled\"}",
                   this->GetWidgetName(), this->GetTclName());
      }
    }

  // Update the menus

  this->UpdateMenuState();
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateMenuState()
{
  int menu_enabled = this->Enabled ? vtkKWMenu::Normal : vtkKWMenu::Disabled;

  if (this->Menu)
    {
    this->Menu->SetEnabled(this->Enabled);
    this->Menu->SetState(menu_enabled);
    }

  // Most Recent Files

  if (this->MostRecentFilesHelper)
    {
    this->MostRecentFilesHelper->GetMostRecentFilesMenu()->SetEnabled(
      this->Enabled);
    this->MostRecentFilesHelper->UpdateMostRecentFilesMenuStateInParent();
    }

  // Update the About entry, since the pretty name also depends on the
  // limited edition mode

  if (this->MenuHelp)
    {
    int pos = this->MenuHelp->GetIndexOfCommand(this, "DisplayAbout");
    if (pos >= 0)
      {
      ostrstream label;
      label << "-label {About " 
            << this->GetApplication()->GetApplicationPrettyName() << "}"<<ends;
      this->MenuHelp->ConfigureItem(pos, label.str());
      label.rdbuf()->freeze(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Menu: " << this->GetMenu() << endl;
  os << indent << "MenuFile: " << this->GetMenuFile() << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "PrintTargetDPI: " << this->GetPrintTargetDPI() << endl;
  os << indent << "ProgressGauge: " << this->GetProgressGauge() << endl;
  os << indent << "PromptBeforeClose: " << this->GetPromptBeforeClose() 
     << endl;
  os << indent << "PropertiesParent: " << this->GetPropertiesParent() << endl;
  os << indent << "ScriptExtension: " << this->GetScriptExtension() << endl;
  os << indent << "ScriptType: " << this->GetScriptType() << endl;
  os << indent << "SupportHelp: " << this->GetSupportHelp() << endl;
  os << indent << "SupportPrint: " << this->GetSupportPrint() << endl;
  os << indent << "StatusFrame: " << this->GetStatusFrame() << endl;
  os << indent << "ViewFrame: " << this->GetViewFrame() << endl;
  os << indent << "WindowClass: " << this->GetWindowClass() << endl;  
  os << indent << "TclInteractor: " << this->GetTclInteractor() << endl;
  os << indent << "Title: " 
     << ( this->GetTitle() ? this->GetTitle() : "(none)" ) << endl;  
  os << indent << "Toolbars: " << this->GetToolbars() << endl;
}


