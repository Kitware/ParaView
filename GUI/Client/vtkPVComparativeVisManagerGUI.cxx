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

#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVis.h"
#include "vtkPVComparativeVisDialog.h"
#include "vtkPVComparativeVisManager.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisManagerGUI );
vtkCxxRevisionMacro(vtkPVComparativeVisManagerGUI, "1.1");

int vtkPVComparativeVisManagerGUICommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVComparativeVisManagerGUI::vtkPVComparativeVisManagerGUI()
{
  this->CommandFunction = vtkPVComparativeVisManagerGUICommand;

  this->Manager = vtkPVComparativeVisManager::New();

  this->ComparativeVisList = vtkKWListBox::New();
  this->AddButton = vtkKWPushButton::New();
  this->EditButton = vtkKWPushButton::New();
  this->DeleteButton = vtkKWPushButton::New();
  this->ShowButton = vtkKWPushButton::New();
  this->HideButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVComparativeVisManagerGUI::~vtkPVComparativeVisManagerGUI()
{
  this->ComparativeVisList->Delete();
  this->AddButton->Delete();
  this->EditButton->Delete();
  this->DeleteButton->Delete();
  this->ShowButton->Delete();
  this->HideButton->Delete();

  this->Manager->Delete();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::Create(
  vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget
  this->Superclass::Create(app, args);

  this->ComparativeVisList->SetParent(this);
  this->ComparativeVisList->Create(app, "");
  this->Script("pack %s", this->ComparativeVisList->GetWidgetName());

  this->AddButton->SetParent(this);
  this->AddButton->SetBalloonHelpString("Add a visualization.");
  this->AddButton->Create(app, "");
  this->AddButton->SetCommand(this, "AddVisualization");
  this->AddButton->SetText("Add");
  this->Script("pack %s", this->AddButton->GetWidgetName());

  this->EditButton->SetParent(this);
  this->EditButton->SetBalloonHelpString("Edit a visualization.");
  this->EditButton->Create(app, "");
  this->EditButton->SetCommand(this, "EditVisualization");
  this->EditButton->SetText("Edit");
  this->Script("pack %s", this->EditButton->GetWidgetName());

  this->DeleteButton->SetParent(this);
  this->DeleteButton->SetBalloonHelpString("Delete a visualization.");
  this->DeleteButton->Create(app, "");
  this->DeleteButton->SetCommand(this, "DeleteVisualization");
  this->DeleteButton->SetText("Delete");
  this->Script("pack %s", this->DeleteButton->GetWidgetName());

  this->ShowButton->SetParent(this);
  this->ShowButton->SetBalloonHelpString("Show a visualization.");
  this->ShowButton->Create(app, "");
  this->ShowButton->SetCommand(this, "ShowVisualization");
  this->ShowButton->SetText("Show");
  this->Script("pack %s", this->ShowButton->GetWidgetName());

  this->HideButton->SetParent(this);
  this->HideButton->SetBalloonHelpString("Hide a visualization.");
  this->HideButton->Create(app, "");
  this->HideButton->SetCommand(this, "HideVisualization");
  this->HideButton->SetText("Hide");
  this->Script("pack %s", this->HideButton->GetWidgetName());

  this->Manager->SetApplication(vtkPVApplication::SafeDownCast(app));
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::AddVisualization()
{
  vtkPVComparativeVisDialog* mydialog = vtkPVComparativeVisDialog::New();
  mydialog->InitializeToDefault();
  mydialog->Create(this->GetApplication(), 0);
  mydialog->SetMasterWindow(this);
  mydialog->SetTitle("Add a visualization");

  if (mydialog->Invoke()) 
    {
    vtkPVComparativeVis* vis = vtkPVComparativeVis::New();
    vis->SetName("My first vis");
    mydialog->CopyToVisualization(vis);
    
    this->Manager->AddVisualization(vis);
    vis->Delete();
    this->Update();
    }
  
  mydialog->Delete();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::EditVisualization()
{
  const char* cur = this->ComparativeVisList->GetSelection();
  if (cur)
    {
    vtkPVComparativeVis* vis = this->Manager->GetVisualization(cur);
    if (vis)
      {
      vtkPVComparativeVisDialog* mydialog = vtkPVComparativeVisDialog::New();
      mydialog->InitializeFromVisualization(vis);
      mydialog->Create(this->GetApplication(), 0);
      mydialog->SetMasterWindow(this);
      mydialog->SetTitle("Edit visualization");
      mydialog->CopyFromVisualization(vis);

      if (mydialog->Invoke()) 
        {
        mydialog->CopyToVisualization(vis);
        this->Update();
        }
  
      mydialog->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::DeleteVisualization()
{
  if (this->ComparativeVisList->GetSelection())
    {
    this->Manager->RemoveVisualization(this->ComparativeVisList->GetSelection());
    this->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::ShowVisualization()
{
  const char* cur = this->ComparativeVisList->GetSelection();
  if (cur)
    {
    vtkPVComparativeVis* vis = this->Manager->GetVisualization(cur);
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
        vis->Generate();
        aMan->SetCacheGeometry(prevStat);
        }
      this->Manager->SetCurrentVisualization(cur);
      this->Manager->Show();
      }
    }  
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::HideVisualization()
{
  this->Manager->Hide();
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::Update()
{
  this->ComparativeVisList->DeleteAll();

  unsigned int numVis = this->Manager->GetNumberOfVisualizations();
  for (unsigned int i=0; i<numVis; i++)
    {
    vtkPVComparativeVis* vis = this->Manager->GetVisualization(i);
    this->ComparativeVisList->AppendUnique(vis->GetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisManagerGUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

