/*=========================================================================

  Module:    vtkKWVolumeMaterialPropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWVolumeMaterialPropertyWidget.h"

#include "vtkKWCheckButton.h"
#include "vtkKWCheckButtonWithLabel.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWScalarComponentSelectionWidget.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkObjectFactory.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWVolumeMaterialPropertyWidget);
vtkCxxRevisionMacro(vtkKWVolumeMaterialPropertyWidget, "1.21");

//----------------------------------------------------------------------------
vtkKWVolumeMaterialPropertyWidget::vtkKWVolumeMaterialPropertyWidget()
{
  this->PropertyChangedEvent = 
    vtkKWEvent::VolumeMaterialPropertyChangedEvent;

  this->PropertyChangingEvent = 
    vtkKWEvent::VolumeMaterialPropertyChangingEvent;

  this->VolumeProperty          = NULL;

  this->SelectedComponent       = 0;
  this->NumberOfComponents      = VTK_MAX_VRCOMP;
  this->AllowEnableShading      = 1;

  // UI

  this->ComponentSelectionWidget = 
    vtkKWScalarComponentSelectionWidget::New();

  this->EnableShadingCheckButton = vtkKWCheckButtonWithLabel::New();
}

//----------------------------------------------------------------------------
vtkKWVolumeMaterialPropertyWidget::~vtkKWVolumeMaterialPropertyWidget()
{
   if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->Delete();
    this->ComponentSelectionWidget = NULL;
    }

  if (this->EnableShadingCheckButton)
    {
    this->EnableShadingCheckButton->Delete();
    this->EnableShadingCheckButton = NULL;
    }

  if (this->VolumeProperty)
    {
    this->VolumeProperty->Delete();
    this->VolumeProperty = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::SetVolumeProperty(
  vtkVolumeProperty *arg)
{
  if (this->VolumeProperty == arg)
    {
    return;
    }

  if (this->VolumeProperty)
    {
    this->VolumeProperty->UnRegister(this);
    }
    
  this->VolumeProperty = arg;

  if (this->VolumeProperty)
    {
    this->VolumeProperty->Register(this);
    }

  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("widget already created " << this->GetClassName());
    return;
    }

  // Call superclass

  this->Superclass::Create();

  // --------------------------------------------------------------
  // Material frame

  int label_width = this->AmbientScale->GetLabel()->GetWidth();

  // --------------------------------------------------------------
  // Component selection

  this->ComponentSelectionWidget->SetParent(this->ControlFrame);
  this->ComponentSelectionWidget->Create();
  this->ComponentSelectionWidget->SetSelectedComponentChangedCommand(
    this, "SelectedComponentCallback");

  vtkKWMenuButtonWithLabel *menubuttonwl = 
    this->ComponentSelectionWidget->GetSelectedComponentOptionMenu();
  menubuttonwl->SetLabelWidth(label_width);

  this->Script("pack %s -side top -padx 2 -pady 2 -anchor w",
               this->ComponentSelectionWidget->GetWidgetName());
  
  // --------------------------------------------------------------
  // Enable Shading

  this->EnableShadingCheckButton->SetParent(this->ControlFrame); 
  this->EnableShadingCheckButton->Create();
  this->EnableShadingCheckButton->GetLabel()->SetText("Enable Shading");
  this->EnableShadingCheckButton->SetLabelWidth(label_width);
  this->EnableShadingCheckButton->GetWidget()->SetText("");
  this->EnableShadingCheckButton->GetWidget()->SetCommand(
    this, "EnableShadingCallback");

  // Pack

  this->Pack();

  // Update according to the current property

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::Pack()
{
  this->Superclass::Pack();

  if (!this->IsCreated())
    {
    return;
    }

  if (this->EnableShadingCheckButton && 
      this->EnableShadingCheckButton->IsCreated())
    {
    if (this->AllowEnableShading)
      {
      this->Script("pack %s -side top -padx 2 -pady 2 -anchor w",
                   this->EnableShadingCheckButton->GetWidgetName());
      }
    else
      {
      this->Script("pack forget %s", 
                   this->EnableShadingCheckButton->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::Update()
{
  // Call superclass

  this->Superclass::Update();

  if (!this->IsCreated())
    {
    return;
    }

  // Component selection menu

  if (this->ComponentSelectionWidget)
    {
    if (this->VolumeProperty)
      {
      this->ComponentSelectionWidget->SetIndependentComponents(
        this->VolumeProperty->GetIndependentComponents());
      }
    this->ComponentSelectionWidget->SetNumberOfComponents(
      this->NumberOfComponents);
    this->ComponentSelectionWidget->SetSelectedComponent(
      this->SelectedComponent);
    this->ComponentSelectionWidget->SetEnabled(
      this->VolumeProperty ? 0 : this->GetEnabled());
    }
  
  // Shading ?

  if (this->EnableShadingCheckButton)
    {
    if (this->VolumeProperty)
      {
      this->EnableShadingCheckButton->GetWidget()->SetSelectedState(
        this->VolumeProperty->GetShade(this->SelectedComponent));
      }
    else
      {
      this->EnableShadingCheckButton->SetEnabled(0);
      }
    }

  // Parameters

  if (this->VolumeProperty)
    {
    double ambient = 
      this->VolumeProperty->GetAmbient(this->SelectedComponent) * 100.0;
    double diffuse = 
      this->VolumeProperty->GetDiffuse(this->SelectedComponent) * 100.0;
    double specular = 
      this->VolumeProperty->GetSpecular(this->SelectedComponent) * 100.0;
    double specular_power = 
      this->VolumeProperty->GetSpecularPower(this->SelectedComponent);
    this->UpdateScales(ambient, diffuse, specular, specular_power);
    }

  // Update the image

  this->UpdatePreview();
}

//----------------------------------------------------------------------------
int vtkKWVolumeMaterialPropertyWidget::AreControlsEnabled()
{
  return this->VolumeProperty && 
    this->VolumeProperty->GetShade(this->SelectedComponent);
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ComponentSelectionWidget);
  this->PropagateEnableState(this->EnableShadingCheckButton);
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::SetSelectedComponent(int arg)
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
void vtkKWVolumeMaterialPropertyWidget::SetNumberOfComponents(int arg)
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
void vtkKWVolumeMaterialPropertyWidget::SetAllowEnableShading(int arg)
{
  if (this->AllowEnableShading == arg)
    {
    return;
    }

  this->AllowEnableShading = arg;
  this->Modified();

  this->Pack();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::SelectedComponentCallback(int n)
{
  this->SelectedComponent = n;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::EnableShadingCallback(int state)
{
  if (this->VolumeProperty &&
      this->VolumeProperty->GetShade(this->SelectedComponent) != state)
    {
    this->VolumeProperty->SetShade(this->SelectedComponent, state);

    float args[2];
    args[0] = this->SelectedComponent;
    args[1] = (float)state;
    this->InvokeEvent(vtkKWEvent::EnableShadingEvent, args);

    this->InvokePropertyChangedCommand();
    this->SendStateEvent(this->PropertyChangedEvent);
    }
  
  this->Update(); // some controls are enabled/disabled when shading is not on
}

//----------------------------------------------------------------------------
int vtkKWVolumeMaterialPropertyWidget::UpdatePropertyFromInterface()
{
  if (!this->VolumeProperty || !this->IsCreated())
    {
    return 0;
    }

  unsigned long mtime = this->VolumeProperty->GetMTime();

  this->VolumeProperty->SetAmbient(
    this->SelectedComponent, this->AmbientScale->GetValue() / 100.0);

  this->VolumeProperty->SetDiffuse(
    this->SelectedComponent, this->DiffuseScale->GetValue() / 100.0);

  this->VolumeProperty->SetSpecular(
    this->SelectedComponent, this->SpecularScale->GetValue() / 100.0);

  this->VolumeProperty->SetSpecularPower(
    this->SelectedComponent, this->SpecularPowerScale->GetValue());

  return (this->VolumeProperty->GetMTime() > mtime);
}

//----------------------------------------------------------------------------
int vtkKWVolumeMaterialPropertyWidget::UpdatePropertyFromPreset(
  const Preset *preset)
{
  if (!this->VolumeProperty || !preset)
    {
    return 0;
    }

  unsigned long mtime = this->VolumeProperty->GetMTime();

  this->VolumeProperty->SetAmbient(
    this->SelectedComponent, preset->Ambient);

  this->VolumeProperty->SetDiffuse(
    this->SelectedComponent, preset->Diffuse);

  this->VolumeProperty->SetSpecular(
    this->SelectedComponent, preset->Specular);

  this->VolumeProperty->SetSpecularPower(
    this->SelectedComponent, preset->SpecularPower);

  return (this->VolumeProperty->GetMTime() > mtime);
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::SendStateEvent(int event)
{
  if (!this->VolumeProperty)
    {
    return;
    }
  
  this->InvokeEvent(event, NULL);
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SelectedComponent: " 
     << this->SelectedComponent << endl;
  os << indent << "NumberOfComponents: " 
     << this->NumberOfComponents << endl;
  os << indent << "AllowEnableShading: "
     << (this->AllowEnableShading ? "On" : "Off") << endl;

  os << indent << "VolumeProperty: ";
  if (this->VolumeProperty)
    {
    os << endl;
    this->VolumeProperty->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ComponentSelectionWidget: ";
  if (this->ComponentSelectionWidget)
    {
    os << endl;
    this->ComponentSelectionWidget->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
