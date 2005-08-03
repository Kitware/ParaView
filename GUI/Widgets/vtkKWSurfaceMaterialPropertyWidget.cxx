/*=========================================================================

  Module:    vtkKWSurfaceMaterialPropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSurfaceMaterialPropertyWidget.h"

#include "vtkKWApplication.h"
#include "vtkKWEvent.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkProperty.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWSurfaceMaterialPropertyWidget);
vtkCxxRevisionMacro(vtkKWSurfaceMaterialPropertyWidget, "1.1");

//----------------------------------------------------------------------------
vtkKWSurfaceMaterialPropertyWidget::vtkKWSurfaceMaterialPropertyWidget()
{
  this->PropertyChangedEvent = 
    vtkKWEvent::SurfacePropertyChangedEvent;
  this->PropertyChangingEvent = 
    vtkKWEvent::SurfacePropertyChangingEvent;

  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkKWSurfaceMaterialPropertyWidget::~vtkKWSurfaceMaterialPropertyWidget()
{
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkKWSurfaceMaterialPropertyWidget::SetProperty(vtkProperty *arg)
{
  if (this->Property == arg)
    {
    return;
    }

  if (this->Property)
    {
    this->Property->UnRegister(this);
    }
    
  this->Property = arg;

  if (this->Property)
    {
    this->Property->Register(this);
    }

  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWSurfaceMaterialPropertyWidget::Update()
{
  // Call superclass

  this->Superclass::Update();

  if (!this->IsCreated())
    {
    return;
    }

  // Color

  if (this->Property)
    {
    this->SetMaterialColor(this->Property->GetColor());
    }
  
  // Parameters

  if (this->Property)
    {
    double ambient = this->Property->GetAmbient() * 100.0;
    double diffuse = this->Property->GetDiffuse() * 100.0;
    double specular = this->Property->GetSpecular() * 100.0;
    double specular_power = this->Property->GetSpecularPower();
    this->UpdateScales(ambient, diffuse, specular, specular_power);
    }

  // Update the image

  this->UpdatePreview();
}

//----------------------------------------------------------------------------
int vtkKWSurfaceMaterialPropertyWidget::UpdatePropertyFromInterface()
{
  if (!this->Property || !this->IsCreated())
    {
    return 0;
    }

  unsigned long mtime = this->Property->GetMTime();

  this->Property->SetAmbient(this->AmbientScale->GetValue() / 100.0);

  this->Property->SetDiffuse(this->DiffuseScale->GetValue() / 100.0);

  this->Property->SetSpecular(this->SpecularScale->GetValue() / 100.0);

  this->Property->SetSpecularPower(this->SpecularPowerScale->GetValue());

  return (this->Property->GetMTime() > mtime);
}

//----------------------------------------------------------------------------
int vtkKWSurfaceMaterialPropertyWidget::UpdatePropertyFromPreset(
  const Preset *preset)
{
  if (!this->Property || !preset)
    {
    return 0;
    }

  unsigned long mtime = this->Property->GetMTime();

  this->Property->SetAmbient(preset->Ambient);

  this->Property->SetDiffuse(preset->Diffuse);

  this->Property->SetSpecular(preset->Specular);

  this->Property->SetSpecularPower(preset->SpecularPower);

  return (this->Property->GetMTime() > mtime);
}

//----------------------------------------------------------------------------
void vtkKWSurfaceMaterialPropertyWidget::SendStateEvent(int event)
{
  if (!this->Property)
    {
    return;
    }
  
  this->InvokeEvent(event, NULL);
}

//----------------------------------------------------------------------------
void vtkKWSurfaceMaterialPropertyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Property: " << this->Property << endl;
}
