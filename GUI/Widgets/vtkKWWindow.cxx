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

#include "vtkKWOptions.h"
#include "vtkKWApplication.h"
#include "vtkKWApplicationSettingsInterface.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWRegistryHelper.h"
#include "vtkKWSeparator.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWToolbar.h"
#include "vtkKWToolbarSet.h"
#include "vtkKWUserInterfaceManagerDialog.h"
#include "vtkKWUserInterfaceManagerNotebook.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkKWWindow, "1.279");

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

  this->MainNotebook = NULL;
  this->MainUserInterfaceManager = NULL;

  // Secondary panel

  this->SecondaryNotebook = NULL;
  this->SecondaryUserInterfaceManager = NULL;

  // View panel

  this->ViewNotebook = NULL;
  this->ViewUserInterfaceManager = NULL;

  // Toolbar set

  this->SecondaryToolbarSet = NULL;
  
  // Application Settings

  this->ApplicationSettingsInterface = NULL;
  this->ApplicationSettingsUserInterfaceManager = NULL;

  this->StatusFramePosition = vtkKWWindow::StatusFramePositionWindow;

  // Some constants

  this->HideMainPanelMenuLabel = 
    vtksys::SystemTools::DuplicateString(
      ks_("Menu|Window|Hide &Main Panel"));
  this->ShowMainPanelMenuLabel = 
    vtksys::SystemTools::DuplicateString(
      ks_("Menu|Window|Show &Main Panel"));
  this->HideSecondaryPanelMenuLabel = 
    vtksys::SystemTools::DuplicateString(
      ks_("Menu|Window|Hide &Bottom Panel"));
  this->ShowSecondaryPanelMenuLabel = 
    vtksys::SystemTools::DuplicateString(
      ks_("Menu|Window|Show &Bottom Panel"));
  this->TclInteractorMenuLabel = 
    vtksys::SystemTools::DuplicateString(
      ks_("Menu|Window|&Tcl Interactor"));

  this->DefaultViewPanelName = 
    vtksys::SystemTools::DuplicateString("View");
  this->MainPanelSizeRegKey = 
    vtksys::SystemTools::DuplicateString("MainPanelSize");
  this->MainPanelVisibilityRegKey = 
    vtksys::SystemTools::DuplicateString("MainPanelVisibility");
  this->MainPanelVisibilityKeyAccelerator = 
    vtksys::SystemTools::DuplicateString("F5");
  this->SecondaryPanelSizeRegKey = 
    vtksys::SystemTools::DuplicateString("SecondaryPanelSize");
  this->SecondaryPanelVisibilityRegKey = 
    vtksys::SystemTools::DuplicateString("SecondaryPanelVisibility");
  this->SecondaryPanelVisibilityKeyAccelerator = 
    vtksys::SystemTools::DuplicateString("F6");
  this->ViewPanelPositionRegKey = 
    vtksys::SystemTools::DuplicateString("ViewPanelPosition");
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

  if (this->ApplicationSettingsUserInterfaceManager)
    {
    this->ApplicationSettingsUserInterfaceManager->Delete();
    this->ApplicationSettingsUserInterfaceManager = NULL;
    }

  if (this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface->Delete();
    this->ApplicationSettingsInterface = NULL;
    }

  this->SetMainPanelSizeRegKey(NULL);
  this->SetMainPanelVisibilityRegKey(NULL);
  this->SetMainPanelVisibilityKeyAccelerator(NULL);
  this->SetHideMainPanelMenuLabel(NULL);
  this->SetShowMainPanelMenuLabel(NULL);
  this->SetSecondaryPanelSizeRegKey(NULL);
  this->SetSecondaryPanelVisibilityRegKey(NULL);
  this->SetSecondaryPanelVisibilityKeyAccelerator(NULL);
  this->SetHideSecondaryPanelMenuLabel(NULL);
  this->SetShowSecondaryPanelMenuLabel(NULL);
  this->SetDefaultViewPanelName(NULL);
  this->SetTclInteractorMenuLabel(NULL);
  this->SetViewPanelPositionRegKey(NULL);
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

  if (this->ApplicationSettingsUserInterfaceManager)
    {
    this->ApplicationSettingsUserInterfaceManager->RemoveAllPanels();
    }

  if (this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet->SetToolbarVisibilityChangedCommand(
      NULL, NULL);
    this->SecondaryToolbarSet->SetNumberOfToolbarsChangedCommand(
      NULL, NULL);
    this->SecondaryToolbarSet->RemoveAllToolbars();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("class already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  vtksys_stl::string cmd, event;
  vtkKWMenu *menu = NULL;
  int idx;

  // Main and Secondary split frames

  this->SecondarySplitFrame->SetSeparatorSize(
    this->MainSplitFrame->GetSeparatorSize());
  this->SecondarySplitFrame->SetOrientationToVertical();

  this->MainSplitFrame->SetExpandableFrameToFrame2();

  if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView)
    {
    this->MainSplitFrame->SetParent(this->Superclass::GetViewFrame());
    this->MainSplitFrame->Create();
    this->SecondarySplitFrame->SetParent(this->MainSplitFrame->GetFrame2());
    this->SecondarySplitFrame->Create();
    }
  else if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain)
    {
    this->MainSplitFrame->SetParent(this->Superclass::GetViewFrame());
    this->MainSplitFrame->Create();
    this->SecondarySplitFrame->SetParent(this->MainSplitFrame->GetFrame1());
    this->SecondarySplitFrame->Create();
    }
  else
    {
    this->SecondarySplitFrame->SetParent(this->Superclass::GetViewFrame());
    this->SecondarySplitFrame->Create();
    this->MainSplitFrame->SetParent(this->SecondarySplitFrame->GetFrame2());
    this->MainSplitFrame->Create();
    }

  this->Script("pack %s -side top -fill both -expand t",
               this->MainSplitFrame->GetWidgetName());
  this->Script("pack %s -side top -fill both -expand t",
               this->SecondarySplitFrame->GetWidgetName());

  // Menu : Window

  menu = this->GetWindowMenu();
  idx = menu->AddCommand(this->GetHideMainPanelMenuLabel(), 
                         this, "MainPanelVisibilityCallback");
  menu->SetItemAccelerator(
    idx, this->GetMainPanelVisibilityKeyAccelerator());

  // Menu : Window

  menu = this->GetWindowMenu();
  idx = menu->AddCommand(this->GetHideSecondaryPanelMenuLabel(), 
                         this, "SecondaryPanelVisibilityCallback");
  menu->SetItemAccelerator(
    idx, this->GetSecondaryPanelVisibilityKeyAccelerator());

  // Menu : View : Application Settings

  menu = this->GetViewMenu();
  idx = this->GetViewMenuInsertPosition();
  menu->InsertSeparator(idx++);
  cmd = "ShowApplicationSettingsUserInterface {";
  cmd += this->GetApplicationSettingsInterface()->GetName();
  cmd += "}";
  menu->InsertCommand(
    idx++, this->GetApplicationSettingsInterface()->GetName(), 
    this, cmd.c_str());

  // Menu : Window : Tcl Interactor

  this->GetWindowMenu()->AddSeparator();

  idx = this->GetWindowMenu()->AddCommand(
    this->GetTclInteractorMenuLabel(), 
    this, "DisplayTclInteractor");
  this->GetWindowMenu()->SetItemHelpString(
    idx, k_("Display a prompt to interact with the Tcl engine"));
  this->GetWindowMenu()->SetItemAccelerator(idx, "Ctrl+T");

  // Udpate the enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWindow::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Superclass::Pack();

  // Status frame and status frame separator
  // Override the parent 

  if (this->StatusFrame && this->StatusFrame->IsCreated())
    {
    vtkKWWidget *in = this;
    switch (this->StatusFramePosition)
      {
      case vtkKWWindow::StatusFramePositionMainPanel:
        in = this->GetMainPanelFrame();
        break;
      case vtkKWWindow::StatusFramePositionSecondaryPanel:
        in = this->GetSecondaryPanelFrame();
        break;
      case vtkKWWindow::StatusFramePositionViewPanel:
        in = this->GetViewPanelFrame();
        break;
      case vtkKWWindow::StatusFramePositionLeftOfDivider:
        if (this->MainSplitFrame)
          {
          in = this->MainSplitFrame->GetFrame1();
          }
        break;
      case vtkKWWindow::StatusFramePositionRightOfDivider:
        if (this->MainSplitFrame)
          {
          in = this->MainSplitFrame->GetFrame2();
          }
        break;
      case vtkKWWindow::StatusFramePositionWindow:
      default:
        break;
      }
    if (this->StatusFrameVisibility && in && in->IsCreated())
      {
      this->Script("pack %s -side bottom -fill x -pady 0 -in %s",
                   this->StatusFrame->GetWidgetName(),
                   in->GetWidgetName());

      if (this->StatusFrameSeparator && 
          this->StatusFrameSeparator->IsCreated())
        {
        this->Script("pack %s -side bottom -fill x -pady 2 -in %s",
                     this->StatusFrameSeparator->GetWidgetName(),
                     in->GetWidgetName());
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkKWToolbarSet* vtkKWWindow::GetSecondaryToolbarSet()
{
  if (!this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet = vtkKWToolbarSet::New();
    }

  if (!this->SecondaryToolbarSet->IsCreated() && this->IsCreated())
    {
    this->SecondaryToolbarSet->SetParent(this->MainSplitFrame->GetFrame2());
    this->SecondaryToolbarSet->Create();
    this->SecondaryToolbarSet->TopSeparatorVisibilityOn();
    this->SecondaryToolbarSet->BottomSeparatorVisibilityOff();
    this->SecondaryToolbarSet->SynchronizeToolbarsVisibilityWithRegistryOn();
    this->SecondaryToolbarSet->SetToolbarVisibilityChangedCommand(
      this, "ToolbarVisibilityChangedCallback");
    this->SecondaryToolbarSet->SetNumberOfToolbarsChangedCommand(
      this, "NumberOfToolbarsChangedCallback");

    vtksys_stl::string after;
    
    if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView)
      {
      after = "-after ";
      after += this->SecondarySplitFrame->GetWidgetName();
      }
    else if (this->ViewNotebook)
      {
      after = "-after ";
      after += this->ViewNotebook->GetWidgetName();
      }

    this->Script(
      "pack %s -padx 0 -pady 0 -side bottom -fill x -expand no %s",
      this->SecondaryToolbarSet->GetWidgetName(), after.c_str());
    }

  return this->SecondaryToolbarSet;
}

//----------------------------------------------------------------------------
void vtkKWWindow::SetViewPanelPosition(int val)
{
  if (val < vtkKWWindow::ViewPanelPositionLeft)
    {
    val = vtkKWWindow::ViewPanelPositionLeft;
    }
  if (val > vtkKWWindow::ViewPanelPositionRight)
    {
    val = vtkKWWindow::ViewPanelPositionRight;
    }

  if (this->GetViewPanelPosition() == val)
    {
    return;
    }

  if (this->MainSplitFrame)
    {
    if (val == vtkKWWindow::ViewPanelPositionRight)
      {
      this->MainSplitFrame->SetFrameLayoutToDefault();
      }
    else
      {
      this->MainSplitFrame->SetFrameLayoutToSwapped();
      }
    }

  // Update the UI (the app settings reflect this change)

  this->Update();
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetViewPanelPosition()
{
  if (this->MainSplitFrame)
    {
    if (this->MainSplitFrame->GetFrameLayout() == 
        vtkKWSplitFrame::FrameLayoutDefault)
      {
      return vtkKWWindow::ViewPanelPositionRight;
      }
    else
      {
      return vtkKWWindow::ViewPanelPositionLeft;
      }
    }

  return vtkKWWindow::ViewPanelPositionRight;
}

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface* 
vtkKWWindow::GetApplicationSettingsInterface()
{
  // If not created, create the application settings interface, connect it
  // to the current window, and manage it with the app settings
  // interface manager.

  // Subclasses that will add more settings will likely to create a subclass
  // of vtkKWApplicationSettingsInterface and override this function so that
  // it instantiates that subclass instead vtkKWApplicationSettingsInterface.

  if (!this->ApplicationSettingsInterface)
    {
    this->ApplicationSettingsInterface = 
      vtkKWApplicationSettingsInterface::New();
    this->ApplicationSettingsInterface->SetWindow(this);
    this->ApplicationSettingsInterface->SetUserInterfaceManager(
      this->GetApplicationSettingsUserInterfaceManager());
    }
  return this->ApplicationSettingsInterface;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* 
vtkKWWindow::GetApplicationSettingsUserInterfaceManager()
{
  if (!this->ApplicationSettingsUserInterfaceManager)
    {
    this->ApplicationSettingsUserInterfaceManager = 
      vtkKWUserInterfaceManagerDialog::New();
    vtkKWTopLevel *toplevel = 
      this->ApplicationSettingsUserInterfaceManager->GetTopLevel();
    toplevel->SetMasterWindow(this);
    toplevel->SetTitle(
      ks_("Application Settings|Title|Application Settings"));
    this->ApplicationSettingsUserInterfaceManager->PageNodeVisibilityOff();
    }

  if  (this->IsCreated() && 
       !this->ApplicationSettingsUserInterfaceManager->IsCreated())
    {
    this->ApplicationSettingsUserInterfaceManager->Create();
    }
  
  return this->ApplicationSettingsUserInterfaceManager;
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowApplicationSettingsUserInterface(const char *name)
{
  if (this->GetApplicationSettingsUserInterfaceManager())
    {
    this->ShowApplicationSettingsUserInterface(
      this->GetApplicationSettingsUserInterfaceManager()->GetPanel(name));
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowApplicationSettingsUserInterface(
  vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    return;
    }

  vtkKWUserInterfaceManager *uim = 
    this->GetApplicationSettingsUserInterfaceManager();
  if (!uim || !uim->HasPanel(panel))
    {
    vtkErrorMacro(
      "Sorry, the user interface panel you are trying to display ("
      << panel->GetName() << ") is not managed by the Application Settings "
      "User Interface Manager");
    return;
    }
  
  panel->Raise();
}

//----------------------------------------------------------------------------
vtkKWNotebook* vtkKWWindow::GetMainNotebook()
{
  if (!this->MainNotebook)
    {
    this->MainNotebook = vtkKWNotebook::New();
    this->MainNotebook->PagesCanBePinnedOn();
    this->MainNotebook->EnablePageTabContextMenuOn();
    this->MainNotebook->AlwaysShowTabsOn();
    }

  if (!this->MainNotebook->IsCreated() && this->IsCreated())
    {
    this->MainNotebook->SetParent(this->GetMainPanelFrame());
    this->MainNotebook->Create();
    this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
                 this->MainNotebook->GetWidgetName());
    }

  return this->MainNotebook;
}

//----------------------------------------------------------------------------
int vtkKWWindow::HasMainUserInterfaceManager()
{
  return this->MainUserInterfaceManager ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* vtkKWWindow::GetMainUserInterfaceManager()
{
  if (!this->MainUserInterfaceManager)
    {
    this->MainUserInterfaceManager = vtkKWUserInterfaceManagerNotebook::New();
    this->MainUserInterfaceManager->SetNotebook(this->GetMainNotebook());
    this->MainUserInterfaceManager->EnableDragAndDropOn();
    }

  if (!this->MainUserInterfaceManager->IsCreated() && this->IsCreated())
    {
    this->MainUserInterfaceManager->Create();
    }
                        
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
    vtkErrorMacro(
      "Sorry, the user interface panel you are trying to display ("
      << panel->GetName() << ") is not managed by the Main "
      "User Interface Manager");
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
  if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowView ||
      this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMainAndView)
    {
    if (this->MainSplitFrame)
      {
      return this->MainSplitFrame->GetFrame1();
      }
    }
  else if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain)
    {
    if (this->SecondarySplitFrame)
      {
      return this->SecondarySplitFrame->GetFrame2();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWWindow::GetMainPanelVisibility()
{
  if (this->MainSplitFrame)
    { 
    return this->MainSplitFrame->GetFrame1Visibility();
    }
  return 0;
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
  this->UpdateMenuState();
}

//----------------------------------------------------------------------------
vtkKWNotebook* vtkKWWindow::GetSecondaryNotebook()
{
  if (!this->SecondaryNotebook)
    {
    this->SecondaryNotebook = vtkKWNotebook::New();
    this->SecondaryNotebook->PagesCanBePinnedOn();
    this->SecondaryNotebook->EnablePageTabContextMenuOn();
    this->SecondaryNotebook->AlwaysShowTabsOn();
    }

  if (!this->SecondaryNotebook->IsCreated() && this->IsCreated())
    {
    this->SecondaryNotebook->SetParent(this->GetSecondaryPanelFrame());
    this->SecondaryNotebook->Create();
    this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
                 this->SecondaryNotebook->GetWidgetName());
    }

  return this->SecondaryNotebook;
}

//----------------------------------------------------------------------------
int vtkKWWindow::HasSecondaryUserInterfaceManager()
{
  return this->SecondaryUserInterfaceManager ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* vtkKWWindow::GetSecondaryUserInterfaceManager()
{
  if (!this->SecondaryUserInterfaceManager)
    {
    this->SecondaryUserInterfaceManager = 
      vtkKWUserInterfaceManagerNotebook::New();
    this->SecondaryUserInterfaceManager->SetNotebook(
      this->GetSecondaryNotebook());
    this->SecondaryUserInterfaceManager->EnableDragAndDropOn();
    }

  if (!this->SecondaryUserInterfaceManager->IsCreated() && this->IsCreated())
    {
    this->SecondaryUserInterfaceManager->Create();
    }
                        
  return this->SecondaryUserInterfaceManager;
}

//----------------------------------------------------------------------------
void vtkKWWindow::ShowSecondaryUserInterface(const char *name)
{
  if (this->HasSecondaryUserInterfaceManager())
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
    vtkErrorMacro(
      "Sorry, the user interface panel you are trying to display ("
      << panel->GetName() << ") is not managed by the Secondary "
      "User Interface Manager");
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
  vtkKWUserInterfaceManager *uim = this->GetViewUserInterfaceManager();
  if (uim)
    {
    vtkKWUserInterfacePanel *panel = 
      uim->GetPanel(this->GetDefaultViewPanelName());
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
  else 
    {
    if (this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain ||
        this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMainAndView)
      {
      return this->MainSplitFrame->GetFrame2();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkKWNotebook* vtkKWWindow::GetViewNotebook()
{
  if (!this->ViewNotebook)
    {
    this->ViewNotebook = vtkKWNotebook::New();
    this->ViewNotebook->PagesCanBePinnedOn();
    this->ViewNotebook->EnablePageTabContextMenuOn();
    this->ViewNotebook->AlwaysShowTabsOff();
    }

  if (!this->ViewNotebook->IsCreated() && this->IsCreated())
    {
    this->ViewNotebook->SetParent(this->GetViewPanelFrame());
    this->ViewNotebook->Create();
    this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
                 this->ViewNotebook->GetWidgetName());
    }

  return this->ViewNotebook;
}

//----------------------------------------------------------------------------
int vtkKWWindow::HasViewUserInterfaceManager()
{
  return this->ViewUserInterfaceManager ? 1 : 0;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager* vtkKWWindow::GetViewUserInterfaceManager()
{
  if (!this->ViewUserInterfaceManager)
    {
    this->ViewUserInterfaceManager = 
      vtkKWUserInterfaceManagerNotebook::New();
    this->ViewUserInterfaceManager->SetNotebook(
      this->GetViewNotebook());
    this->ViewUserInterfaceManager->EnableDragAndDropOn();
    }

  if (!this->ViewUserInterfaceManager->IsCreated() && this->IsCreated())
    {
    this->ViewUserInterfaceManager->Create();

    // Also create a default page for the view

    vtkKWUserInterfacePanel *panel = vtkKWUserInterfacePanel::New();
    panel->SetName(this->GetDefaultViewPanelName());
    panel->SetUserInterfaceManager(this->ViewUserInterfaceManager);
    panel->Create();
    panel->Delete();
    panel->AddPage(panel->GetName(), NULL);
    }
                        
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
    vtkErrorMacro(
      "Sorry, the user interface panel you are trying to display ("
      << panel->GetName() << ") is not managed by the View "
      "User Interface Manager");
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

//----------------------------------------------------------------------------
void vtkKWWindow::SetStatusFramePosition(int s)
{
  if (s < vtkKWWindow::StatusFramePositionWindow)
    {
    s = vtkKWWindow::StatusFramePositionWindow;
    }
  else if (s > vtkKWWindow::StatusFramePositionRightOfDivider) 
    {
    s = vtkKWWindow::StatusFramePositionRightOfDivider;
    }
  if (s == this->StatusFramePosition)
    {
    return;
    }

  this->StatusFramePosition = s;
  this->Modified();
  this->Pack();
}

//-----------------------------------------------------------------------------
void vtkKWWindow::PrintSettingsCallback()
{
  vtkKWApplicationSettingsInterface *app_settings = 
    this->GetApplicationSettingsInterface();

  vtkKWUserInterfaceManagerDialog *app_settings_uim =
    vtkKWUserInterfaceManagerDialog::SafeDownCast(
      this->GetApplicationSettingsUserInterfaceManager());

  // If the UIM is a dialog one, try to reach the Print Settings section
  // directly, otherwise just use the regular UIM API to show the panel
  
  if (app_settings && app_settings_uim)
    {
    app_settings_uim->RaiseSection(
      app_settings, 
      NULL, 
      ks_("Application Settings|Page Setup"));
    }
  else
    {
    this->ShowApplicationSettingsUserInterface(app_settings);
    }
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
    2, "Geometry", this->GetMainPanelSizeRegKey(), "%d", 
    this->MainSplitFrame->GetFrame1Size());

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", this->GetMainPanelVisibilityRegKey(), "%d", 
    this->GetMainPanelVisibility());

  // Secondary panel

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", this->GetSecondaryPanelSizeRegKey(), "%d", 
    this->SecondarySplitFrame->GetFrame1Size());

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", this->GetSecondaryPanelVisibilityRegKey(), "%d", 
    this->GetSecondaryPanelVisibility());

  // View panel

  const char *pos = NULL;
  if (this->GetViewPanelPosition() == vtkKWWindow::ViewPanelPositionLeft)
    {
    pos = "Left";
    }
  else if (this->GetViewPanelPosition() == vtkKWWindow::ViewPanelPositionRight)
    {
    pos = "Right";
    }
  if (pos)
    {
    this->GetApplication()->SetRegistryValue(
      2, "Geometry", this->GetViewPanelPositionRegKey(), "%s", pos);
    }
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
        2, "Geometry", this->GetMainPanelSizeRegKey()))
    {
    int reg_size = this->GetApplication()->GetIntRegistryValue(
      2, "Geometry", this->GetMainPanelSizeRegKey());
    if (reg_size >= this->MainSplitFrame->GetFrame1MinimumSize())
      {
      this->MainSplitFrame->SetFrame1Size(reg_size);
      }
    }

  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", this->GetMainPanelVisibilityRegKey()))
    {
    this->SetMainPanelVisibility(
      this->GetApplication()->GetIntRegistryValue(
        2, "Geometry", this->GetMainPanelVisibilityRegKey()));
    }

  // Secondary panel

  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", this->GetSecondaryPanelSizeRegKey()))
    {
    int reg_size = this->GetApplication()->GetIntRegistryValue(
      2, "Geometry", this->GetSecondaryPanelSizeRegKey());
    if (reg_size >= this->SecondarySplitFrame->GetFrame1MinimumSize())
      {
      this->SecondarySplitFrame->SetFrame1Size(reg_size);
      }
    }
  
  if (this->GetApplication()->HasRegistryValue(
        2, "Geometry", this->GetSecondaryPanelVisibilityRegKey()))
    {
    this->SetSecondaryPanelVisibility(
      this->GetApplication()->GetIntRegistryValue(
        2, "Geometry", this->GetSecondaryPanelVisibilityRegKey()));
    }

  // View panel

  char pos[vtkKWRegistryHelper::RegistryKeyValueSizeMax];
  if (this->GetApplication()->GetRegistryValue(
        2, "Geometry", this->GetViewPanelPositionRegKey(), pos))
    {
    if (!strcmp(pos, "Left"))
      {
      this->SetViewPanelPositionToLeft();
      }
    else if (!strcmp(pos, "Right"))
      {
      this->SetViewPanelPositionToRight();
      }
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

  if (this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet->PopulateToolbarsVisibilityMenu(
      this->GetToolbarsVisibilityMenu());
    }
}
  
//----------------------------------------------------------------------------
void vtkKWWindow::ToolbarVisibilityChangedCallback(vtkKWToolbar *toolbar)
{
  this->Superclass::ToolbarVisibilityChangedCallback(toolbar);

  if (this->SecondaryToolbarSet)
    {
    this->SecondaryToolbarSet->UpdateToolbarsVisibilityMenu(
      this->GetToolbarsVisibilityMenu());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::Update()
{
  this->Superclass::Update();

  // Update the whole interface

  if (this->HasMainUserInterfaceManager())
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

  if (this->HasMainUserInterfaceManager())
    {
    this->GetMainUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetMainUserInterfaceManager()->UpdateEnableState();
    // this->GetMainUserInterfaceManager()->Update();
    }

  if (this->HasSecondaryUserInterfaceManager())
    {
    this->GetSecondaryUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetSecondaryUserInterfaceManager()->UpdateEnableState();
    // this->GetSecondaryUserInterfaceManager()->Update();
    }

  if (this->HasViewUserInterfaceManager())
    {
    this->GetViewUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetViewUserInterfaceManager()->UpdateEnableState();
    // this->GetViewUserInterfaceManager()->Update();
    }

  if (this->GetApplicationSettingsUserInterfaceManager())
    {
    this->GetApplicationSettingsUserInterfaceManager()->SetEnabled(
      this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetApplicationSettingsUserInterfaceManager()->UpdateEnableState();
    // this->GetApplicationSettingsUserInterfaceManager()->Update();
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
    if (this->WindowMenu->HasItem(this->GetHideMainPanelMenuLabel()))
      {
      idx = this->WindowMenu->GetIndexOfItem(
        this->GetHideMainPanelMenuLabel());
      }
    else if (this->WindowMenu->HasItem(this->GetShowMainPanelMenuLabel()))
      {
      idx = this->WindowMenu->GetIndexOfItem(
        this->GetShowMainPanelMenuLabel());
      }
    if (idx >= 0)
      {
      vtksys_stl::string label;
      label += this->GetMainPanelVisibility()
        ? this->GetHideMainPanelMenuLabel() 
        : this->GetShowMainPanelMenuLabel();
      this->WindowMenu->SetItemLabel(idx, label.c_str());
      }
    }

  // Update the secondary panel visibility label

  if (this->WindowMenu)
    {
    int idx = -1;
    if (this->WindowMenu->HasItem(this->GetHideSecondaryPanelMenuLabel()))
      {
      idx = this->WindowMenu->GetIndexOfItem(
        this->GetHideSecondaryPanelMenuLabel());
      }
    else if (this->WindowMenu->HasItem(
               this->GetShowSecondaryPanelMenuLabel()))
      {
      idx = this->WindowMenu->GetIndexOfItem(
        this->GetShowSecondaryPanelMenuLabel());
      }
    if (idx >= 0)
      {
      vtksys_stl::string label;
      label += this->GetSecondaryPanelVisibility()
        ? this->GetHideSecondaryPanelMenuLabel() 
        : this->GetShowSecondaryPanelMenuLabel();
      this->WindowMenu->SetItemLabel(idx, label.c_str());
      this->WindowMenu->SetItemState(
        idx, 
        ((this->PanelLayout == vtkKWWindow::PanelLayoutSecondaryBelowMain &&
          !this->GetMainPanelVisibility()) ?
         vtkKWOptions::StateDisabled : this->WindowMenu->GetEnabled()));
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
  os << indent << "StatusFramePosition: " << this->StatusFramePosition << endl;
}
