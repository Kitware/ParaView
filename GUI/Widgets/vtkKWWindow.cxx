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
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWNotebook.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWUserInterfaceNotebookManager.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkObjectFactory.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWToolbar.h"

#include <vtksys/SystemTools.hxx>

const char *vtkKWWindow::MainPanelSizeRegKey = "MainPanelSize";
const char *vtkKWWindow::MainPanelVisibilityRegKey = "MainPanelVisibility";
const char *vtkKWWindow::MainPanelVisibilityKeyAccelerator = "F5";
const char *vtkKWWindow::HideMainPanelMenuLabel = "Hide Left Panel";
const char *vtkKWWindow::ShowMainPanelMenuLabel = "Show Left Panel";

const char *vtkKWWindow::SecondaryPanelSizeRegKey = "SecondaryPanelSize";
const char *vtkKWWindow::SecondaryPanelVisibilityRegKey = "SecondaryPanelVisibility";
const char *vtkKWWindow::SecondaryPanelVisibilityKeyAccelerator = "F6";
const char *vtkKWWindow::HideSecondaryPanelMenuLabel = "Hide Bottom Panel";
const char *vtkKWWindow::ShowSecondaryPanelMenuLabel = "Show Bottom Panel";
const char *vtkKWWindow::DefaultViewPanelName = "View";
const char *vtkKWWindow::TclInteractorMenuLabel = "Command Prompt";

vtkCxxRevisionMacro(vtkKWWindow, "1.260");

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindow );

//----------------------------------------------------------------------------
vtkKWWindow::vtkKWWindow()
{
  this->MainSplitFrame = vtkKWSplitFrame::New();
  this->MainSplitFrame->SetFrame1MinimumSize(250);

  this->SecondarySplitFrame = vtkKWSplitFrame::New();

  this->PanelLayout = vtkKWWindow::PanelLayoutSecondaryBelowView;

  // Main Panel

  this->MainNotebook = vtkKWNotebook::New();
  this->MainNotebook->PagesCanBePinnedOn();
  this->MainNotebook->EnablePageTabContextMenuOn();
  this->MainNotebook->AlwaysShowTabsOn();

  this->MainUserInterfaceManager = vtkKWUserInterfaceNotebookManager::New();
  this->MainUserInterfaceManager->SetNotebook(this->MainNotebook);
  this->MainUserInterfaceManager->EnableDragAndDropOn();

  // Secondary panel

  this->SecondaryNotebook = vtkKWNotebook::New();
  this->SecondaryNotebook->PagesCanBePinnedOn();
  this->SecondaryNotebook->EnablePageTabContextMenuOn();
  this->SecondaryNotebook->AlwaysShowTabsOn();

  this->SecondaryUserInterfaceManager = 
    vtkKWUserInterfaceNotebookManager::New();
  this->SecondaryUserInterfaceManager->SetNotebook(this->SecondaryNotebook);
  this->SecondaryUserInterfaceManager->EnableDragAndDropOn();

  // View panel

  this->ViewNotebook = vtkKWNotebook::New();
  this->ViewNotebook->PagesCanBePinnedOn();
  this->ViewNotebook->EnablePageTabContextMenuOn();
  this->ViewNotebook->AlwaysShowTabsOff();

  this->ViewUserInterfaceManager = 
    vtkKWUserInterfaceNotebookManager::New();
  this->ViewUserInterfaceManager->SetNotebook(this->ViewNotebook);
  this->ViewUserInterfaceManager->EnableDragAndDropOn();

  // Toolbar set

  this->SecondaryToolbarSet = vtkKWToolbarSet::New();
  
  // Interfaces

  this->ApplicationSettingsInterface = NULL;
}

//----------------------------------------------------------------------------
vtkKWWindow::~vtkKWWindow()
{
  this->PrepareForDelete();

  if (this->MainSplitFrame)
    {
    this->MainSplitFrame->Delete();
    this->MainSplitFrame = NULL;
    }

  if (this->MainNotebook)
    {
    this->MainNotebook->Delete();
    this->MainNotebook = NULL;
    }

  if (this->MainUserInterfaceManager)
    {
    this->MainUserInterfaceManager->Delete();
    this->MainUserInterfaceManager = NULL;
    }

  if (this->SecondarySplitFrame)
    {
    this->SecondarySplitFrame->Delete();
    this->SecondarySplitFrame = NULL;
    }

  if (this->SecondaryNotebook)
    {
    this->SecondaryNotebook->Delete();
    this->SecondaryNotebook = NULL;
    }

  if (this->SecondaryUserInterfaceManager)
    {
    this->SecondaryUserInterfaceManager->Delete();
    this->SecondaryUserInterfaceManager = NULL;
    }

  if (this->ViewNotebook)
    {
    this->ViewNotebook->Delete();
    this->ViewNotebook = NULL;
    }

  if (this->ViewUserInterfaceManager)
    {
    this->ViewUserInterfaceManager->Delete();
    this->ViewUserInterfaceManager = NULL;
    }

  if (this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet->Delete();
    this->SecondaryToolbarSet = NULL;
    }

  if (this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface->Delete();
    this->ApplicationSettingsInterface = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::PrepareForDelete()
{
  if (this->MainUserInterfaceManager)
    {
    this->MainUserInterfaceManager->RemoveAllPanels();
    }

  if (this->SecondaryUserInterfaceManager)
    {
    this->SecondaryUserInterfaceManager->RemoveAllPanels();
    }

  if (this->ViewUserInterfaceManager)
    {
    this->ViewUserInterfaceManager->RemoveAllPanels();
    }

  if (this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet->SetToolbarVisibilityChangedCommand(NULL, NULL);
    this->SecondaryToolbarSet->SetNumberOfToolbarsChangedCommand(NULL, NULL);
    this->SecondaryToolbarSet->RemoveAllToolbars();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("class already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  vtksys_stl::string cmd, event;
  vtkKWMenu *menu = NULL;
  int idx;
  vtkKWUserInterfaceManager *uim;

  // Main and Secondary split frames

  this->SecondarySplitFrame->SetSeparatorSize(
    this->MainSplitFrame->GetSeparatorSize());
  this->SecondarySplitFrame->SetOrientationToVertical();

  if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView)
    {
    this->MainSplitFrame->SetParent(this->Superclass::GetViewFrame());
    this->MainSplitFrame->Create(app);
    this->SecondarySplitFrame->SetParent(this->MainSplitFrame->GetFrame2());
    this->SecondarySplitFrame->Create(app);
    }
  else if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain)
    {
    this->MainSplitFrame->SetParent(this->Superclass::GetViewFrame());
    this->MainSplitFrame->Create(app);
    this->SecondarySplitFrame->SetParent(this->MainSplitFrame->GetFrame1());
    this->SecondarySplitFrame->Create(app);
    }
  else
    {
    this->SecondarySplitFrame->SetParent(this->Superclass::GetViewFrame());
    this->SecondarySplitFrame->Create(app);
    this->MainSplitFrame->SetParent(this->SecondarySplitFrame->GetFrame2());
    this->MainSplitFrame->Create(app);
    }

  this->Script("pack %s -side top -fill both -expand t",
               this->MainSplitFrame->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
               this->SecondarySplitFrame->GetWidgetName());

  // Create the view notebook

  this->ViewNotebook->SetParent(this->GetViewPanelFrame());
  this->ViewNotebook->Create(app);

  this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
               this->ViewNotebook->GetWidgetName());

  // If we have a view User Interface Manager, it's time to create it
  // Also create a default page for the view

  uim = this->GetViewUserInterfaceManager();
  if (uim)
    {
    if  (!uim->IsCreated())
      {
      uim->Create(app);
      }
    
    vtkKWUserInterfacePanel *panel = vtkKWUserInterfacePanel::New();
    panel->SetName(vtkKWWindow::DefaultViewPanelName);
    panel->SetUserInterfaceManager(this->GetViewUserInterfaceManager());
    panel->Create(app);
    panel->Delete();
    panel->AddPage(panel->GetName(), NULL);
    }

  // Menu : Window

  menu = this->GetWindowMenu();
  menu->AddCommand(vtkKWWindow::HideMainPanelMenuLabel, 
                   this, "MainPanelVisibilityCallback", 5);
  menu->SetItemAccelerator(
    vtkKWWindow::HideMainPanelMenuLabel,
    vtkKWWindow::MainPanelVisibilityKeyAccelerator);
  event = "<Key-";
  event += vtkKWWindow::MainPanelVisibilityKeyAccelerator;
  event += ">";
  this->AddBinding(event.c_str(), this, "MainPanelVisibilityCallback");

  // Create the main notebook

  this->MainNotebook->SetParent(this->GetMainPanelFrame());
  this->MainNotebook->Create(app);

  this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
               this->MainNotebook->GetWidgetName());

  // If we have a main User Interface Manager, it's time to create it

  uim = this->GetMainUserInterfaceManager();
  if (uim && !uim->IsCreated())
    {
    uim->Create(app);
    }

  // Create the secondary notebook

  this->SecondaryNotebook->SetParent(this->GetSecondaryPanelFrame());
  this->SecondaryNotebook->Create(app);

  this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
               this->SecondaryNotebook->GetWidgetName());

  // If we have a secondary User Interface Manager, it's time to create it

  uim = this->GetSecondaryUserInterfaceManager();
  if (uim && !uim->IsCreated())
    {
    uim->Create(app);
    }

  // Menu : Window

  menu = this->GetWindowMenu();
  menu->AddCommand(vtkKWWindow::HideSecondaryPanelMenuLabel, 
                   this, "SecondaryPanelVisibilityCallback", 5);
  menu->SetItemAccelerator(
    vtkKWWindow::HideSecondaryPanelMenuLabel,
    vtkKWWindow::SecondaryPanelVisibilityKeyAccelerator);
  event = "<Key-";
  event += vtkKWWindow::SecondaryPanelVisibilityKeyAccelerator;
  event += ">";
  this->AddBinding(event.c_str(), this, "SecondaryPanelVisibilityCallback");

  // Menu : View : Application Settings

  menu = this->GetViewMenu();
  idx = this->GetViewMenuInsertPosition();
  menu->InsertSeparator(idx++);
  cmd = "ShowMainUserInterface {";
  cmd += this->GetApplicationSettingsInterface()->GetName();
  cmd += "}";
  menu->InsertCommand(
    idx++, this->GetApplicationSettingsInterface()->GetName(), 
    this, cmd.c_str(), 0);

  // Menu : Window : Tcl Interactor

  this->GetWindowMenu()->AddSeparator();

  this->GetWindowMenu()->AddCommand(
    vtkKWWindow::TclInteractorMenuLabel, 
    this, "DisplayTclInteractor", 8, 
    "Display a prompt to interact with the Tcl engine");

  // Secondary toolbar

  this->SecondaryToolbarSet->SetParent(this->MainSplitFrame->GetFrame2());
  this->SecondaryToolbarSet->Create(app);
  this->SecondaryToolbarSet->ShowTopSeparatorOn();
  this->SecondaryToolbarSet->ShowBottomSeparatorOff();
  this->SecondaryToolbarSet->SynchronizeToolbarsVisibilityWithRegistryOn();
  this->SecondaryToolbarSet->SetToolbarVisibilityChangedCommand(
    this, "ToolbarVisibilityChangedCallback");
  this->SecondaryToolbarSet->SetNumberOfToolbarsChangedCommand(
    this, "NumberOfToolbarsChangedCallback");

  this->Script(
    "pack %s -padx 0 -pady 0 -side bottom -fill x -expand no -after %s",
    this->SecondaryToolbarSet->GetWidgetName(),
    (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView ?
     this->SecondarySplitFrame->GetWidgetName() :
     this->ViewNotebook->GetWidgetName())); // important

  // Udpate the enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface* 
vtkKWWindow::GetApplicationSettingsInterface()
{
  // If not created, create the application settings interface, connect it
  // to the current window, and manage it with the current interface manager.

  // Subclasses that will add more settings will likely to create a subclass
  // of vtkKWApplicationSettingsInterface and override this function so that
  // it instantiates that subclass instead vtkKWApplicationSettingsInterface.

  if (!this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface = 
      vtkKWApplicationSettingsInterface::New();
    this->ApplicationSettingsInterface->SetWindow(this);
    this->ApplicationSettingsInterface->SetUserInterfaceManager(
      this->GetMainUserInterfaceManager());
    }
  return this->ApplicationSettingsInterface;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* vtkKWWindow::GetMainUserInterfaceManager()
{
  return this->MainUserInterfaceManager;
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowMainUserInterface(const char *name)
{
  if (this->GetMainUserInterfaceManager())
    {
    this->ShowMainUserInterface(
      this->GetMainUserInterfaceManager()->GetPanel(name));
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowMainUserInterface(vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    return;
    }

  vtkKWUserInterfaceManager *uim = this->GetMainUserInterfaceManager();
  if (!uim || !uim->HasPanel(panel))
    {
    return;
    }

  this->SetMainPanelVisibility(1);

  if (!panel->Raise())
    {
    ostrstream msg;
    msg << "The panel you are trying to access could not be displayed "
        << "properly. Please make sure there is enough room in the notebook "
        << "to bring up this part of the interface.";
    if (this->MainNotebook && 
        this->MainNotebook->GetShowOnlyMostRecentPages() &&
        this->MainNotebook->GetPagesCanBePinned())
      {
      msg << " This may happen if you displayed " 
          << this->MainNotebook->GetNumberOfMostRecentPages() 
          << " notebook pages "
          << "at the same time and pinned/locked all of them. In that case, "
          << "try to hide or unlock a notebook page first.";
      }
    msg << ends;
    vtkKWMessageDialog::PopupMessage( 
      this->GetApplication(), this, "User Interface Warning", msg.str(),
      vtkKWMessageDialog::WarningIcon);
    msg.rdbuf()->freeze(0);
    }
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWWindow::GetMainPanelFrame()
{
  if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView)
    {
    return this->MainSplitFrame ? this->MainSplitFrame->GetFrame1() : NULL;
    }
  else if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain)
    {
    return this->SecondarySplitFrame 
      ? this->SecondarySplitFrame->GetFrame2() : NULL;
    }
  else
    {
    return this->MainSplitFrame ? this->MainSplitFrame->GetFrame1() : NULL;
    }
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetMainPanelVisibility()
{
  return (this->MainSplitFrame && 
          this->MainSplitFrame->GetFrame1Visibility() ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetMainPanelVisibility(int arg)
{
  if (arg == this->GetMainPanelVisibility())
    {
    return;
    }

  if (this->MainSplitFrame)
    {
    this->MainSplitFrame->SetFrame1Visibility(arg);
    }

  this->UpdateMenuState();
}

//----------------------------------------------------------------------------
void vtkKWWindow::MainPanelVisibilityCallback()
{
  this->SetMainPanelVisibility(!this->GetMainPanelVisibility());
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* vtkKWWindow::GetSecondaryUserInterfaceManager()
{
  return this->SecondaryUserInterfaceManager;
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowSecondaryUserInterface(const char *name)
{
  if (this->GetSecondaryUserInterfaceManager())
    {
    this->ShowSecondaryUserInterface(
      this->GetSecondaryUserInterfaceManager()->GetPanel(name));
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowSecondaryUserInterface(vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    return;
    }

  vtkKWUserInterfaceManager *uim = this->GetSecondaryUserInterfaceManager();
  if (!uim || !uim->HasPanel(panel))
    {
    return;
    }

  this->SetSecondaryPanelVisibility(1);

  if (!panel->Raise())
    {
    vtksys_stl::string msg;
    msg = "The panel you are trying to access could not be displayed "
      "properly. Please make sure there is enough room in the notebook "
      "to bring up this part of the interface.";
    if (this->SecondaryNotebook && 
        this->SecondaryNotebook->GetShowOnlyMostRecentPages() &&
        this->SecondaryNotebook->GetPagesCanBePinned())
      {
      msg += " This may happen if you displayed ";
      msg += this->SecondaryNotebook->GetNumberOfMostRecentPages();
      msg += " notebook pages "
        "at the same time and pinned/locked all of them. In that case, "
        "try to hide or unlock a notebook page first.";
      }
    vtkKWMessageDialog::PopupMessage( 
      this->GetApplication(), this, "User Interface Warning", msg.c_str(),
      vtkKWMessageDialog::WarningIcon);
    }
}

//---------------------------------------------------------------------------
vtkKWFrame* vtkKWWindow::GetSecondaryPanelFrame()
{
  return this->SecondarySplitFrame ? 
    this->SecondarySplitFrame->GetFrame1() : NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetSecondaryPanelVisibility()
{
  return (this->SecondarySplitFrame && 
          this->SecondarySplitFrame->GetFrame1Visibility() ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetSecondaryPanelVisibility(int arg)
{
  if (arg == this->GetSecondaryPanelVisibility())
    {
    return;
    }

  if (this->SecondarySplitFrame)
    {
    this->SecondarySplitFrame->SetFrame1Visibility(arg);
    }

  this->UpdateMenuState();
}

//----------------------------------------------------------------------------
void vtkKWWindow::SecondaryPanelVisibilityCallback()
{
  this->SetSecondaryPanelVisibility(!this->GetSecondaryPanelVisibility());
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWWindow::GetViewFrame()
{
  if (this->ViewUserInterfaceManager)
    {
    vtkKWUserInterfacePanel *panel = 
      this->ViewUserInterfaceManager->GetPanel(
        vtkKWWindow::DefaultViewPanelName);
    if (panel)
      {
      return vtkKWFrame::SafeDownCast(panel->GetPageWidget(panel->GetName()));
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWWindow::GetViewPanelFrame()
{
  if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView)
    {
    return this->SecondarySplitFrame->GetFrame2();
    }
  else if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain)
    {
    return this->MainSplitFrame->GetFrame2();
    }
  else
    {
    return this->MainSplitFrame->GetFrame2();
    }
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* vtkKWWindow::GetViewUserInterfaceManager()
{
  return this->ViewUserInterfaceManager;
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowViewUserInterface(const char *name)
{
  if (this->GetViewUserInterfaceManager())
    {
    this->ShowViewUserInterface(
      this->GetViewUserInterfaceManager()->GetPanel(name));
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowViewUserInterface(vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    return;
    }

  vtkKWUserInterfaceManager *uim = this->GetViewUserInterfaceManager();
  if (!uim || !uim->HasPanel(panel))
    {
    return;
    }

  this->SetSecondaryPanelVisibility(1);

  if (!panel->Raise())
    {
    vtksys_stl::string msg;
    msg = "The panel you are trying to access could not be displayed "
      "properly. Please make sure there is enough room in the notebook "
      "to bring up this part of the interface.";
    if (this->ViewNotebook && 
        this->ViewNotebook->GetShowOnlyMostRecentPages() &&
        this->ViewNotebook->GetPagesCanBePinned())
      {
      msg += " This may happen if you displayed ";
      msg += this->ViewNotebook->GetNumberOfMostRecentPages();
      msg += " notebook pages "
        "at the same time and pinned/locked all of them. In that case, "
        "try to hide or unlock a notebook page first.";
      }
    vtkKWMessageDialog::PopupMessage( 
      this->GetApplication(), this, "User Interface Warning", msg.c_str(),
      vtkKWMessageDialog::WarningIcon);
    }
}

//-----------------------------------------------------------------------------
void vtkKWWindow::PrintOptionsCallback()
{
  this->ShowMainUserInterface(this->GetApplicationSettingsInterface());
}

//----------------------------------------------------------------------------
void vtkKWWindow::SaveWindowGeometryToRegistry()
{
  this->Superclass::SaveWindowGeometryToRegistry();

  if (!this->IsCreated())
    {
    return;
    }

  // Main panel

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindow::MainPanelSizeRegKey, "%d", 
    this->MainSplitFrame->GetFrame1Size());

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindow::MainPanelVisibilityRegKey, "%d", 
    this->GetMainPanelVisibility());

  // Secondary panel

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindow::SecondaryPanelSizeRegKey, "%d", 
    this->SecondarySplitFrame->GetFrame1Size());

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindow::SecondaryPanelVisibilityRegKey, "%d", 
    this->GetSecondaryPanelVisibility());
}

//----------------------------------------------------------------------------
void vtkKWWindow::RestoreWindowGeometryFromRegistry()
{
  this->Superclass::RestoreWindowGeometryFromRegistry();

  if (!this->IsCreated())
    {
    return;
    }

  // Main panel

  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", vtkKWWindow::MainPanelSizeRegKey))
    {
    int reg_size = this->GetApplication()->GetIntRegistryValue(
      2, "Geometry", vtkKWWindow::MainPanelSizeRegKey);
    if (reg_size >= this->MainSplitFrame->GetFrame1MinimumSize())
      {
      this->MainSplitFrame->SetFrame1Size(reg_size);
      }
    }

  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", vtkKWWindow::MainPanelVisibilityRegKey))
    {
    this->SetMainPanelVisibility(
      this->GetApplication()->GetIntRegistryValue(
        2, "Geometry", vtkKWWindow::MainPanelVisibilityRegKey));
    }

  // Secondary panel

  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", vtkKWWindow::SecondaryPanelSizeRegKey))
    {
    int reg_size = this->GetApplication()->GetIntRegistryValue(
      2, "Geometry", vtkKWWindow::SecondaryPanelSizeRegKey);
    if (reg_size >= this->SecondarySplitFrame->GetFrame1MinimumSize())
      {
      this->SecondarySplitFrame->SetFrame1Size(reg_size);
      }
    }
  
  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", vtkKWWindow::SecondaryPanelVisibilityRegKey))
    {
    this->SetSecondaryPanelVisibility(
      this->GetApplication()->GetIntRegistryValue(
        2, "Geometry", vtkKWWindow::SecondaryPanelVisibilityRegKey));
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Render()
{
}

//-----------------------------------------------------------------------------
void vtkKWWindow::NumberOfToolbarsChangedCallback()
{
  this->Superclass::NumberOfToolbarsChangedCallback();

  this->SecondaryToolbarSet->PopulateToolbarsVisibilityMenu(
    this->GetToolbarsVisibilityMenu());
}
  
//----------------------------------------------------------------------------
void vtkKWWindow::ToolbarVisibilityChangedCallback()
{
  this->Superclass::ToolbarVisibilityChangedCallback();

  this->SecondaryToolbarSet->UpdateToolbarsVisibilityMenu(
    this->GetToolbarsVisibilityMenu());
}

//----------------------------------------------------------------------------
void vtkKWWindow::Update()
{
  this->Superclass::Update();

  // Update the whole interface

  if (this->GetMainUserInterfaceManager())
    {
    // Redundant Update() here, since we call UpdateEnableState(), which as 
    // a side effect will update each panel (see UpdateEnableState())
    // this->GetMainUserInterfaceManager()->Update();
    }
}

//-----------------------------------------------------------------------------
void vtkKWWindow::UpdateToolbarState()
{
  this->Superclass::UpdateToolbarState();

  if (this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet->SetToolbarsFlatAspect(
      vtkKWToolbar::GetGlobalFlatAspect());
    this->SecondaryToolbarSet->SetToolbarsWidgetsFlatAspect(
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    this->PropagateEnableState(this->SecondaryToolbarSet);
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Update the notebook

  this->PropagateEnableState(this->MainNotebook);
  this->PropagateEnableState(this->SecondaryNotebook);
  this->PropagateEnableState(this->ViewNotebook);

  // Update all the user interface panels

  if (this->GetMainUserInterfaceManager())
    {
    this->GetMainUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetMainUserInterfaceManager()->UpdateEnableState();
    // this->GetMainUserInterfaceManager()->Update();
    }

  if (this->GetSecondaryUserInterfaceManager())
    {
    this->GetSecondaryUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetSecondaryUserInterfaceManager()->UpdateEnableState();
    // this->GetSecondaryUserInterfaceManager()->Update();
    }

  if (this->GetViewUserInterfaceManager())
    {
    this->GetViewUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetViewUserInterfaceManager()->UpdateEnableState();
    // this->GetViewUserInterfaceManager()->Update();
    }

  // Update the window element

  this->PropagateEnableState(this->MainSplitFrame);
  this->PropagateEnableState(this->SecondarySplitFrame);
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateMenuState()
{
  this->Superclass::UpdateMenuState();

  // Update the main panel visibility label

  if (this->WindowMenu)
    {
    int idx = -1;
    if (this->WindowMenu->HasItem(vtkKWWindow::HideMainPanelMenuLabel))
      {
      idx = this->WindowMenu->GetIndex(vtkKWWindow::HideMainPanelMenuLabel);
      }
    else if (this->WindowMenu->HasItem(vtkKWWindow::ShowMainPanelMenuLabel))
      {
      idx = this->WindowMenu->GetIndex(vtkKWWindow::ShowMainPanelMenuLabel);
      }
    if (idx >= 0)
      {
      vtksys_stl::string label("-label {");
      label += this->GetMainPanelVisibility()
        ? vtkKWWindow::HideMainPanelMenuLabel 
        : vtkKWWindow::ShowMainPanelMenuLabel;
      label += "}";
      this->WindowMenu->ConfigureItem(idx, label.c_str());
      }
    }

  // Update the secondary panel visibility label

  if (this->WindowMenu)
    {
    int idx = -1;
    if (this->WindowMenu->HasItem(vtkKWWindow::HideSecondaryPanelMenuLabel))
      {
      idx = this->WindowMenu->GetIndex(
        vtkKWWindow::HideSecondaryPanelMenuLabel);
      }
    else if (this->WindowMenu->HasItem(
               vtkKWWindow::ShowSecondaryPanelMenuLabel))
      {
      idx = this->WindowMenu->GetIndex(
        vtkKWWindow::ShowSecondaryPanelMenuLabel);
      }
    if (idx >= 0)
      {
      vtksys_stl::string label("-label {");
      label += this->GetSecondaryPanelVisibility()
        ? vtkKWWindow::HideSecondaryPanelMenuLabel 
        : vtkKWWindow::ShowSecondaryPanelMenuLabel;
      label += "}";
      this->WindowMenu->ConfigureItem(idx, label.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MainNotebook: " << this->GetMainNotebook() << endl;
  os << indent << "SecondaryNotebook: " << this->GetSecondaryNotebook() << endl;
  os << indent << "ViewNotebook: " << this->GetViewNotebook() << endl;
  os << indent << "MainSplitFrame: " << this->MainSplitFrame << endl;
  os << indent << "SecondarySplitFrame: " << this->SecondarySplitFrame << endl;
  os << indent << "SecondaryToolbarSet: " << this->SecondaryToolbarSet << endl;
  os << indent << "PanelLayout: " << this->PanelLayout << endl;
}
