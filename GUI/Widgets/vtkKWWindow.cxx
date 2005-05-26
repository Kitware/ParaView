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
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

const char *vtkKWWindow::MainPanelSizeRegKey = "MainPanelSize";
const char *vtkKWWindow::HideMainPanelMenuLabel = "Hide Left Panel";
const char *vtkKWWindow::ShowMainPanelMenuLabel = "Show Left Panel";

vtkCxxRevisionMacro(vtkKWWindow, "1.242");

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindow );

int vtkKWWindowCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWWindow::vtkKWWindow()
{
  this->MainSplitFrame        = vtkKWSplitFrame::New();

  this->MainNotebook          = vtkKWNotebook::New();

  this->CommandFunction       = vtkKWWindowCommand;
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
}

//----------------------------------------------------------------------------
void vtkKWWindow::Create(vtkKWApplication *app, const char *args)
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
  
  // Menu : Window

  this->GetWindowMenu()->AddCommand(
    vtkKWWindow::HideMainPanelMenuLabel, this, "MainPanelVisibilityCallback", 1);

  // Split frame

  this->MainSplitFrame->SetParent(this->Superclass::GetViewFrame());
  this->MainSplitFrame->Create(app);

  this->Script("pack %s -side top -fill both -expand t",
               this->MainSplitFrame->GetWidgetName());

  // Create the notebook

  this->MainNotebook->SetParent(this->GetMainPanelFrame());
  this->MainNotebook->Create(app, NULL);

  this->Script("pack %s -pady 0 -padx 0 -fill both -expand yes -anchor n",
               this->MainNotebook->GetWidgetName());

  this->MainNotebook->AlwaysShowTabsOn();

  // If we have a User Interface Manager, it's time to create it

  vtkKWUserInterfaceManager *uim = this->GetUserInterfaceManager();
  if (uim && !uim->IsCreated())
    {
    uim->Create(app);
    }

  // Udpate the enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWWindow::GetMainPanelFrame()
{
  return this->MainSplitFrame ? this->MainSplitFrame->GetFrame1() : NULL;
}

//----------------------------------------------------------------------------
vtkKWFrame* vtkKWWindow::GetViewFrame()
{
  return this->MainSplitFrame ? this->MainSplitFrame->GetFrame2() : NULL;
}

//----------------------------------------------------------------------------
void vtkKWWindow::SaveWindowGeometryToRegistry()
{
  this->Superclass::SaveWindowGeometryToRegistry();

  if (!this->IsCreated())
    {
    return;
    }

  this->GetApplication()->SetRegistryValue(
    2, "Geometry", vtkKWWindow::MainPanelSizeRegKey, "%d", 
    this->MainSplitFrame->GetFrame1Size());
}

//----------------------------------------------------------------------------
void vtkKWWindow::RestoreWindowGeometryFromRegistry()
{
  this->Superclass::RestoreWindowGeometryFromRegistry();

  if (!this->IsCreated())
    {
    return;
    }

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
}

//----------------------------------------------------------------------------
void vtkKWWindow::Render()
{
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
void vtkKWWindow::Update()
{
  this->Superclass::Update();

  // Update the whole interface

  if (this->GetUserInterfaceManager())
    {
    // Redundant Update() here, since we call UpdateEnableState(), which as 
    // a side effect will update each panel (see UpdateEnableState())
    // this->GetUserInterfaceManager()->Update();
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Update the notebook

  this->PropagateEnableState(this->MainNotebook);

  // Update all the user interface panels

  if (this->GetUserInterfaceManager())
    {
    this->GetUserInterfaceManager()->SetEnabled(this->GetEnabled());
    // As a side effect, SetEnabled() call an Update() on the panel, 
    // which will call an UpdateEnableState() too,
    // this->GetUserInterfaceManager()->UpdateEnableState();
    // this->GetUserInterfaceManager()->Update();
    }

  // Update the window element

  this->PropagateEnableState(this->MainSplitFrame);
}

//----------------------------------------------------------------------------
void vtkKWWindow::UpdateMenuState()
{
  this->Superclass::UpdateMenuState();

  // Update the main panel visibility label

  if (this->WindowMenu)
    {
    kwsys_stl::string label("-label {");
    label += this->GetMainPanelVisibility()
      ? vtkKWWindow::HideMainPanelMenuLabel : vtkKWWindow::ShowMainPanelMenuLabel;
    label += "}";
    this->WindowMenu->ConfigureItem(0, label.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MainNotebook: " << this->GetMainNotebook() << endl;
  os << indent << "MainSplitFrame: " << this->GetMainSplitFrame() << endl;
}


