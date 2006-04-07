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
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWInternationalization.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWScalarComponentSelectionWidget, "1.20");
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

  this->SelectedComponentOptionMenu     = vtkKWMenuButtonWithLabel::New();
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
void vtkKWScalarComponentSelectionWidget::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  // --------------------------------------------------------------
  // Component selection

  this->SelectedComponentOptionMenu->SetParent(this);
  this->SelectedComponentOptionMenu->Create();
  this->SelectedComponentOptionMenu->ExpandWidgetOff();
  this->SelectedComponentOptionMenu->GetLabel()->SetText(
    ks_("Scalar Component|Component:"));
  this->SelectedComponentOptionMenu->SetBalloonHelpString(
    k_("Select the component this interface will control."));

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
    vtkKWMenuButton *menubutton = 
      this->SelectedComponentOptionMenu->GetWidget();
    vtkKWMenu *menu = menubutton->GetMenu();

    if (this->SelectedComponentOptionMenu->IsCreated() &&
        menu->GetNumberOfItems() != this->NumberOfComponents)
      {
      menu->DeleteAllItems();
      for (i = 0; i < this->NumberOfComponents; ++i)
        {
        ostrstream cmd_name, cmd_method;
        
        cmd_name << i + 1 << ends;
        cmd_method << "SelectedComponentCallback " << i << ends;

        menu->AddRadioButton(cmd_name.str(), this, cmd_method.str());

        cmd_name.rdbuf()->freeze(0);
        cmd_method.rdbuf()->freeze(0);
        }
      }
    
    if (menu->GetNumberOfItems() && this->IndependentComponents)
      {
      ostrstream v;
      v << this->SelectedComponent + 1 << ends;
      menubutton->SetValue(v.str());
      v.rdbuf()->freeze(0);
      }
    else
      {
      menubutton->SetValue("");
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
void vtkKWScalarComponentSelectionWidget::SetSelectedComponentChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SelectedComponentChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void 
vtkKWScalarComponentSelectionWidget::InvokeSelectedComponentChangedCommand(
  int comp)
{
  if (this->SelectedComponentChangedCommand && 
      *this->SelectedComponentChangedCommand && 
      this->GetApplication())
    {
    this->Script("%s %d", 
                 this->SelectedComponentChangedCommand, comp);
    }

  this->InvokeEvent(vtkKWEvent::ScalarComponentChangedEvent, &comp);
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
  this->InvokeSelectedComponentChangedCommand(this->SelectedComponent);
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
