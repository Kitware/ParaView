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

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButtonLabeled.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWOptionMenuLabeled.h"
#include "vtkKWScalarComponentSelectionWidget.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWVolumeMaterialPropertyWidget);
vtkCxxRevisionMacro(vtkKWVolumeMaterialPropertyWidget, "1.5");

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

  this->EnableShadingCheckButton = vtkKWCheckButtonLabeled::New();
}

//----------------------------------------------------------------------------
vtkKWVolumeMaterialPropertyWidget::~vtkKWVolumeMaterialPropertyWidget()
{
  this->SetVolumeProperty(NULL);

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
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::SetVolumeProperty(
  vtkVolumeProperty *prop)
{
  if (this->VolumeProperty == prop)
    {
    return;
    }

  this->VolumeProperty = prop;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::Create(vtkKWApplication *app,
                                         const char *args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("widget already created " << this->GetClassName());
    return;
    }

  // Call superclass

  this->Superclass::Create(app, args);

  // --------------------------------------------------------------
  // Material frame

  vtkKWFrame *frame = this->MaterialPropertiesFrame->GetFrame();

  int label_width = this->AmbientScale->GetLabel()->GetWidth();

  // --------------------------------------------------------------
  // Component selection

  this->ComponentSelectionWidget->SetParent(frame);
  this->ComponentSelectionWidget->Create(app, 0);
  this->ComponentSelectionWidget->SetSelectedComponentChangedCommand(
    this, "SelectedComponentCallback");

  vtkKWOptionMenuLabeled *omenu = 
    this->ComponentSelectionWidget->GetSelectedComponentOptionMenu();
  omenu->SetLabelWidth(label_width);
  
  // --------------------------------------------------------------
  // Enable Shading

  this->EnableShadingCheckButton->SetParent(frame); 
  this->EnableShadingCheckButton->Create(app, 0);
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
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("pack %s -side top -before %s -padx 2 -pady 2 -anchor w",
               this->ComponentSelectionWidget->GetWidgetName(),
               this->LightingFrame->GetWidgetName());

  if (this->AllowEnableShading)
    {
    this->Script("pack %s -side top -before %s -padx 2 -pady 2 -anchor w",
                 this->EnableShadingCheckButton->GetWidgetName(),
                 this->LightingFrame->GetWidgetName());
    }
  else
    {
    this->Script("pack forget %s", 
                 this->EnableShadingCheckButton->GetWidgetName());
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

  // From here, we need the vol prop

  if (!this->VolumeProperty)
    {
    return;
    }

  // Component selection menu

  if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->SetIndependentComponents(
      this->VolumeProperty->GetIndependentComponents());
    this->ComponentSelectionWidget->SetNumberOfComponents(
      this->NumberOfComponents);
    this->ComponentSelectionWidget->SetSelectedComponent(
      this->SelectedComponent);
    }
  
  // Shading ?

  if (this->EnableShadingCheckButton)
    {
    this->EnableShadingCheckButton->GetWidget()->SetState(
      this->VolumeProperty->GetShade(this->SelectedComponent));
    }

  // Ambient

  if (this->AmbientScale)
    { 
    float ambient = 
      this->VolumeProperty->GetAmbient(this->SelectedComponent) * 100.0;
    if (this->AmbientScale->GetValue() != ambient)
      {
      int old_disable = this->AmbientScale->GetDisableCommands();
      this->AmbientScale->SetDisableCommands(1);
      this->AmbientScale->SetValue(ambient);
      this->AmbientScale->SetDisableCommands(old_disable);
      }
    }

  // Diffuse

  if (this->DiffuseScale)
    {
    float diffuse = 
      this->VolumeProperty->GetDiffuse(this->SelectedComponent) * 100.0;
    if (this->DiffuseScale->GetValue() != diffuse)
      {
      int old_disable = this->DiffuseScale->GetDisableCommands();
      this->DiffuseScale->SetDisableCommands(1);
      this->DiffuseScale->SetValue(diffuse);
      this->DiffuseScale->SetDisableCommands(old_disable);
      }
    }

  // Specular

  if (this->SpecularScale)
    {
    float specular = 
      this->VolumeProperty->GetSpecular(this->SelectedComponent) * 100.0;
    if (this->SpecularScale->GetValue() != specular)
      {
      int old_disable = this->SpecularScale->GetDisableCommands();
      this->SpecularScale->SetDisableCommands(1);
      this->SpecularScale->SetValue(specular);
      this->SpecularScale->SetDisableCommands(old_disable);
      }
    }

  // Specular power

  if (this->SpecularPowerScale)
    {
    float specular_power = 
      this->VolumeProperty->GetSpecularPower(this->SelectedComponent);
    if (this->SpecularPowerScale->GetValue() != specular_power)
      {
      int old_disable = this->SpecularPowerScale->GetDisableCommands();
      this->SpecularPowerScale->SetDisableCommands(1);
      this->SpecularPowerScale->SetValue(specular_power);
      this->SpecularPowerScale->SetDisableCommands(old_disable);
      }
    }

  // Update the image

  this->UpdatePreview();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->ComponentSelectionWidget)
    {
    this->ComponentSelectionWidget->SetEnabled(this->Enabled);
    }

  if (this->EnableShadingCheckButton)
    {
    this->EnableShadingCheckButton->SetEnabled(this->Enabled);
    }
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
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::SelectedComponentCallback(int n)
{
  this->SelectedComponent = n;
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWVolumeMaterialPropertyWidget::EnableShadingCallback()
{
  if (!this->IsCreated())
    {
    return;
    }

  int state = this->EnableShadingCheckButton->GetWidget()->GetState();

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
