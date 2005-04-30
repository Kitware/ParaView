/*=========================================================================

  Module:    vtkKWScalarComponentSelectionWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWScalarComponentSelectionWidget.h"

#include "vtkKWEvent.h"
#include "vtkKWOptionMenuLabeled.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWScalarComponentSelectionWidget, "1.9");
vtkStandardNewMacro(vtkKWScalarComponentSelectionWidget);

//----------------------------------------------------------------------------
vtkKWScalarComponentSelectionWidget::vtkKWScalarComponentSelectionWidget()
{
  this->IndependentComponents           = 1;
  this->NumberOfComponents              = VTK_MAX_VRCOMP;
  this->SelectedComponent               = 0;
  this->AllowComponentSelection         = 1;

  this->SelectedComponentChangedCommand = NULL;

  // GUI

  this->SelectedComponentOptionMenu     = vtkKWOptionMenuLabeled::New();
}

//----------------------------------------------------------------------------
vtkKWScalarComponentSelectionWidget::~vtkKWScalarComponentSelectionWidget()
{
  // Commands

  if (this->SelectedComponentChangedCommand)
    {
    delete [] this->SelectedComponentChangedCommand;
    this->SelectedComponentChangedCommand = NULL;
    }

  // GUI

  if (this->SelectedComponentOptionMenu)
    {
    this->SelectedComponentOptionMenu->Delete();
    this->SelectedComponentOptionMenu = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::Create(
  vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->ConfigureOptions(args);

  // --------------------------------------------------------------
  // Component selection

  this->SelectedComponentOptionMenu->SetParent(this);
  this->SelectedComponentOptionMenu->Create(app);
  this->SelectedComponentOptionMenu->ExpandWidgetOff();
  this->SelectedComponentOptionMenu->GetLabel()->SetText("Component:");
  this->SelectedComponentOptionMenu->SetBalloonHelpString(
    "Select the component this interface will control.");

  // Pack

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->AllowComponentSelection)
    {
    this->Script("pack %s -side top -padx 0 -pady 0 -anchor w",
                 this->SelectedComponentOptionMenu->GetWidgetName());
    }
  else
    {
    this->Script("pack forget %s", 
                 this->SelectedComponentOptionMenu->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // In the dependent case, everything is in the component 0

  if (!this->IndependentComponents ||
      (this->SelectedComponent < 0 ||
       this->SelectedComponent >= this->NumberOfComponents))
    {
    this->SelectedComponent = 0;
    }

  int i;

  // Component selection menu

  if (this->SelectedComponentOptionMenu)
    {
    vtkKWOptionMenu *menu = this->SelectedComponentOptionMenu->GetWidget();

    if (this->SelectedComponentOptionMenu->IsCreated() &&
        menu->GetNumberOfEntries() != this->NumberOfComponents)
      {
      menu->DeleteAllEntries();
      for (i = 0; i < this->NumberOfComponents; ++i)
        {
        ostrstream cmd_name, cmd_method;
        
        cmd_name << i + 1 << ends;
        cmd_method << "SelectedComponentCallback " << i << ends;

        menu->AddEntryWithCommand(cmd_name.str(), this, cmd_method.str());

        cmd_name.rdbuf()->freeze(0);
        cmd_method.rdbuf()->freeze(0);
        }
      }
    
    if (menu->GetNumberOfEntries() && this->IndependentComponents)
      {
      ostrstream v;
      v << this->SelectedComponent + 1 << ends;
      menu->SetValue(v.str());
      v.rdbuf()->freeze(0);
      }
    else
      {
      menu->SetValue("");
      }

    if (!this->IndependentComponents || this->NumberOfComponents <= 1)
      {
      this->SelectedComponentOptionMenu->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->SelectedComponentOptionMenu);
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetAllowComponentSelection(
  int arg)
{
  if (this->AllowComponentSelection == arg)
    {
    return;
    }

  this->AllowComponentSelection = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetIndependentComponents(int arg)
{
  if (this->IndependentComponents == arg)
    {
    return;
    }

  this->IndependentComponents = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetSelectedComponent(int arg)
{
  if (this->SelectedComponent == arg ||
      arg < 0 || arg >= this->NumberOfComponents)
    {
    return;
    }

  this->SelectedComponent = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetNumberOfComponents(int arg)
{
  if (this->NumberOfComponents == arg ||
      arg < 1 || arg > VTK_MAX_VRCOMP)
    {
    return;
    }

  this->NumberOfComponents = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::InvokeSelectedComponentChangedCommand()
{
  if (this->SelectedComponentChangedCommand && 
      *this->SelectedComponentChangedCommand)
    {
    this->Script("eval %s %d", 
                 this->SelectedComponentChangedCommand, 
                 this->SelectedComponent);
    }

  this->InvokeEvent(vtkKWEvent::ScalarComponentChangedEvent, 
                    &this->SelectedComponent);
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetSelectedComponentChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SelectedComponentChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SelectedComponentCallback(int n)
{
  if (this->SelectedComponent == n)
    {
    return;
    }

  this->SelectedComponent = n;
  this->Update();
  this->InvokeSelectedComponentChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "IndependentComponents: "
     << (this->IndependentComponents ? "On" : "Off") << endl;
  os << indent << "SelectedComponent: " 
     << this->SelectedComponent << endl;
  os << indent << "NumberOfComponents: " 
     << this->NumberOfComponents << endl;
  os << indent << "AllowComponentSelection: "
     << (this->AllowComponentSelection ? "On" : "Off") << endl;
  os << indent << "SelectedComponentOptionMenu: " 
     << this->SelectedComponentOptionMenu << endl;
}
