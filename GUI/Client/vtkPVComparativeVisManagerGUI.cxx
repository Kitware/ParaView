/*=========================================================================

  Module:    vtkPVComparativeVisManagerGUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeVisManagerGUI.h"

#include "vtkCommand.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVisDialog.h"
#include "vtkPVComparativeVisManager.h"
#include "vtkPVComparativeVisProgressDialog.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkSMComparativeVisProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisManagerGUI );
vtkCxxRevisionMacro(vtkPVComparativeVisManagerGUI, "1.15");

class vtkCVProgressObserver : public vtkCommand
{
public:
  static vtkCVProgressObserver* New()
    {
    return new vtkCVProgressObserver;
    }
  virtual void Execute(vtkObject*, unsigned long, void* prog)
    {
      double progress = *((double*)prog);
      if (this->Manager)
        {
        this->Manager->UpdateProgress(progress);
        }
    }

  vtkPVComparativeVisManagerGUI* Manager;

protected:
  vtkCVProgressObserver()
    {
      this->Manager = 0;
    }
};

//----------------------------------------------------------------------------
vtkPVComparativeVisManagerGUI::vtkPVComparativeVisManagerGUI()
{
  this->Manager = vtkPVComparativeVisManager::New();

  this->MainFrame = vtkKWFrame::New();

  this->ListFrame = vtkKWFrameWithLabel::New();
  this->ComparativeVisList = vtkKWListBox::New();

  this->CommandFrame = vtkKWFrame::New();
  this->CreateButton = vtkKWPushButton::New();
  this->EditButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();
  this->ShowButton = vtkKWPushButton::New();
  this->HideButton = vtkKWPushButton::New();
  this->CloseButton = vtkKWPushButton::New();

  this->EditDialog = vtkPVComparativeVisDialog::New();
  this->ProgressDialog = vtkPVComparativeVisProgressDialog::New();

  this->InShow = 0;
  this->VisSelected = 0;

  this->VisBeingGenerated = 0;

  this->ProgressObserver = vtkCVProgressObserver::New();
  this->ProgressObserver->Manager = this;
}

//----------------------------------------------------------------------------
vtkPVComparativeVisManagerGUI::~vtkPVComparativeVisManagerGUI()
{
  this->MainFrame->Delete();

  this->ListFrame->Delete();
  this->ComparativeVisList->Delete();

  this->CommandFrame->Delete();
  this->CreateButton->Delete();
  this->EditButton->Delete();
  this->DeleteButton->Delete();
  this->ShowButton->Delete();
  this->HideButton->Delete();

  this->CloseButton->Delete();

  this->EditDialog->Delete();
  this->ProgressDialog->Delete();

  this->Manager->Delete();

  this->ProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget
  this->Superclass::CreateWidget();

  this->MainFrame->SetParent(this);
  this->MainFrame->Create();
  this->Script("pack %s -padx 5 -pady 5 -expand t -fill both", 
               this->MainFrame->GetWidgetName());

  this->ListFrame->SetParent(this->MainFrame);
  this->ListFrame->Create();
  this->ListFrame->SetLabelText("Current Visualizations");
  this->Script("pack %s -side top -expand t -fill both", 
               this->ListFrame->GetWidgetName());

  this->ComparativeVisList->SetParent(this->ListFrame->GetFrame());
  this->ComparativeVisList->Create();
  this->Script("pack %s -side top -pady 5 -expand t -fill both", 
               this->ComparativeVisList->GetWidgetName());

  this->ComparativeVisList->SetDoubleClickCommand(this, "ShowVisualization");
  this->ComparativeVisList->SetSingleClickCommand(this, "ItemSelected");

  this->CommandFrame->SetParent(this->MainFrame);
  this->CommandFrame->Create();
  this->Script("pack %s -side top -pady 5 -expand t -fill x", 
               this->CommandFrame->GetWidgetName());

  this->CreateButton->SetParent(this->CommandFrame);
  this->CreateButton->SetBalloonHelpString("Create a visualization");
  this->CreateButton->Create();
  this->CreateButton->SetWidth(7);
  this->CreateButton->SetCommand(this, "AddVisualization");
  this->CreateButton->SetText("Create");
  this->Script("pack %s -side left -padx 2", 
               this->CreateButton->GetWidgetName());

  this->DeleteButton->SetParent(this->CommandFrame);
  this->DeleteButton->SetBalloonHelpString("Delete a visualization");
  this->DeleteButton->Create();
  this->DeleteButton->SetWidth(7);
  this->DeleteButton->SetCommand(this, "DeleteVisualization");
  this->DeleteButton->SetText("Delete");
  this->Script("pack %s  -side left -padx 2", 
               this->DeleteButton->GetWidgetName());

  this->EditButton->SetParent(this->CommandFrame);
  this->EditButton->SetBalloonHelpString("Edit a visualization");
  this->EditButton->Create();
  this->EditButton->SetWidth(7);
  this->EditButton->SetCommand(this, "EditVisualization");
  this->EditButton->SetText("Edit");
  this->Script("pack %s -side left -padx 2", this->EditButton->GetWidgetName());

  this->ShowButton->SetParent(this->CommandFrame);
  this->ShowButton->SetBalloonHelpString("Show a visualization");
  this->ShowButton->Create();
  this->ShowButton->SetWidth(7);
  this->ShowButton->SetCommand(this, "ShowVisualization");
  this->ShowButton->SetText("Show");
  this->Script("pack %s -side left -padx 2", this->ShowButton->GetWidgetName());

  this->HideButton->SetParent(this->CommandFrame);
  this->HideButton->SetBalloonHelpString("Hide a visualization");
  this->HideButton->Create();
  this->HideButton->SetWidth(7);
  this->HideButton->SetCommand(this, "HideVisualization");
  this->HideButton->SetText("Hide");
  this->Script("pack %s -side left -padx 2", this->HideButton->GetWidgetName());

  this->CloseButton->SetParent(this->MainFrame);
  this->CloseButton->SetBalloonHelpString("Close the visualization dialog");
  this->CloseButton->Create();
  this->CloseButton->SetCommand(this, "Withdraw");
  this->CloseButton->SetText("Close");
  this->Script("pack %s -side top -expand t -fill x", 
               this->CloseButton->GetWidgetName());

  vtkPVApplication* pvApp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  this->Manager->SetApplication(pvApp);

  this->EditDialog->SetMasterWindow(pvApp->GetMainWindow());
  this->EditDialog->Create();
  this->EditDialog->SetTitle("Edit visualization");

  this->ProgressDialog->SetMasterWindow(pvApp->GetMainWindow());
  this->ProgressDialog->SetTitle("Comparative vis progress");

  this->SetResizable(0, 0);
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::UpdateProgress(double progress)
{
  if (progress <= 0.01)
    {
    return;
    }

  this->ProgressDialog->SetProgress(progress);
  if (this->ProgressDialog->GetAbortFlag())
    {
    this->VisBeingGenerated->SetShouldAbort(1);
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::ItemSelected()
{
  this->VisSelected = 1;
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::PrepareForDelete()
{
  this->HideVisualization();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::AddVisualization()
{
  this->EditDialog->InitializeToDefault();

  if (this->EditDialog->Invoke()) 
    {
    vtkSMProxy* vis = vtkSMObject::GetProxyManager()->NewProxy(
      "ComparativeVisHelpers","ComparativeVis");
    this->EditDialog->CopyToVisualization(
      static_cast<vtkSMComparativeVisProxy*>(vis));
 
    vtkSMComparativeVisProxy* cvp = static_cast<vtkSMComparativeVisProxy*>(vis);
    this->Manager->AddVisualization(cvp);
    if (cvp->GetVisName() && cvp->GetVisName()[0] != '\0')
      {
      this->Manager->SetSelectedVisualizationName(cvp->GetVisName());
      }

    vis->Delete();
    this->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::EditVisualization()
{
  const char* cur = this->ComparativeVisList->GetSelection();
  if (cur)
    {
    vtkSMComparativeVisProxy* vis = this->Manager->GetVisualization(cur);
    if (vis)
      {
      this->EditDialog->CopyFromVisualization(vis);

      if (this->EditDialog->Invoke()) 
        {
        this->EditDialog->CopyToVisualization(vis);
        if (vis->GetVisName() && vis->GetVisName()[0] != '\0')
          {
          this->Manager->SetSelectedVisualizationName(vis->GetVisName());
          }
        this->Update();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::DeleteVisualization()
{
  if (this->ComparativeVisList->GetSelection())
    {
    this->Manager->RemoveVisualization(
      this->ComparativeVisList->GetSelection());
    if (!this->Manager->GetCurrentlyDisplayedVisualization())
      {
      this->InShow = 0;
      }
    this->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::ShowVisualization()
{
  const char* cur = this->ComparativeVisList->GetSelection();
  if (cur)
    {
    vtkSMComparativeVisProxy* vis = this->Manager->GetVisualization(cur);
    if (vis)
      {
      if (!vis->GetIsGenerated())
        {
        vtkPVApplication* app = 
          vtkPVApplication::SafeDownCast(this->GetApplication());
        vtkPVAnimationManager* aMan = 
          app->GetMainWindow()->GetAnimationManager();
        int prevStat = aMan->GetCacheGeometry();
        aMan->SetCacheGeometry(0);
        vis->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);
        this->VisBeingGenerated = vis;
        if (!this->ProgressDialog->IsCreated())
          {
          this->ProgressDialog->Create();
          }
        this->ProgressDialog->Display();
        this->ProgressDialog->SetProgress(0.01);
        this->Manager->GenerateVisualization(vis);
        this->ProgressDialog->Withdraw();
        this->VisBeingGenerated = 0;
        vis->RemoveObserver(this->ProgressObserver);
        aMan->SetCacheGeometry(prevStat);
        }
      if (!this->ProgressDialog->GetAbortFlag())
        {
        this->Manager->SetSelectedVisualizationName(cur);
        if (this->Manager->Show())
          {
          this->InShow = 1;
          }
        }
      else
        {
        vis->RemoveAllCache();
        vtkPVApplication::SafeDownCast(
          this->GetApplication())->GetMainView()->ForceRender();
        }
      this->ProgressDialog->SetAbortFlag(0);
      this->Update();
      }
    }  
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::HideVisualization()
{
  this->Manager->Hide();
  this->InShow = 0;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::Update()
{
  this->ComparativeVisList->DeleteAll();

  int idx=-1;
  // Add all the visualizations to the list
  unsigned int numVis = this->Manager->GetNumberOfVisualizations();
  for (unsigned int i=0; i<numVis; i++)
    {
    vtkSMComparativeVisProxy* vis = this->Manager->GetVisualization(i);
    const char* name = vis->GetVisName();
    if (name && name[0] != '\0')
      {
      this->ComparativeVisList->AppendUnique(name);
      if (this->Manager->GetSelectedVisualizationName() &&
          strcmp(this->Manager->GetSelectedVisualizationName(), name) == 0)
        {
        idx = i;
        }
      }
    }

  // Select the current vis.
  if (idx >= 0)
    {
    this->ComparativeVisList->SetSelectionIndex(idx);
    this->VisSelected = 1;
    }
  else if (this->ComparativeVisList->GetNumberOfItems() > 0)
    {
    this->ComparativeVisList->SetSelectionIndex(0);
    }
  else
    {
    this->VisSelected = 0;
    }

  this->UpdateEnableState();
}

//-----------------------------------------------------------------------------
// This takes care enabling/disabling buttons
void vtkPVComparativeVisManagerGUI::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ComparativeVisList);
  this->PropagateEnableState(this->CreateButton);
  this->PropagateEnableState(this->EditButton);
  this->PropagateEnableState(this->DeleteButton);
  this->PropagateEnableState(this->ShowButton);
  this->PropagateEnableState(this->HideButton);
  this->PropagateEnableState(this->CloseButton);

  if (this->GetEnabled())
    {
    if (!this->InShow)
      {
      this->HideButton->SetEnabled(0);
      this->CreateButton->SetEnabled(1);
      }
    else
      {
      this->HideButton->SetEnabled(1);
      this->CreateButton->SetEnabled(0);
      }
    if (this->ComparativeVisList->GetNumberOfItems() < 1 ||
        !this->VisSelected)
      {
      this->EditButton->SetEnabled(0);
      this->ShowButton->SetEnabled(0);
      this->DeleteButton->SetEnabled(0);
      }
    else
      {
      this->EditButton->SetEnabled(1);
      this->ShowButton->SetEnabled(1);
      this->DeleteButton->SetEnabled(1);
      }
    if (this->InShow)
      {
      this->EditButton->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::SaveState(ofstream *file)
{
  *file << endl;
  *file << "# Comparative visualizations" << endl;

  vtkPVApplication* app = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* mainWin = app->GetMainWindow();
  *file << "set kw(" << this->GetTclName() << ") [$kw(" 
        << mainWin->GetTclName() << ") GetComparativeVisManagerGUI]"
        << endl;
  *file << "set kw(" << this->Manager->GetTclName() << ") [$kw("
        << this->GetTclName() << ") GetManager]" << endl;
  this->Manager->SaveState(file);
 
  *file << "$kw(" << this->GetTclName() << ") Update" << endl;
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Manager: ";
  if (this->Manager)
    {
    this->Manager->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(null)" << endl;
    }
  os << indent << "ComparativeVisList: " << this->GetComparativeVisList() << endl;
}

