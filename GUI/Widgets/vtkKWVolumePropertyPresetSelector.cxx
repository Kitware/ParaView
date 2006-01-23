/*=========================================================================

  Module:    vtkKWVolumePropertyPresetSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWVolumePropertyPresetSelector.h"

#include "vtkVolumeProperty.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"

#define VTK_KW_WLPS_TOLERANCE 0.005

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWVolumePropertyPresetSelector);
vtkCxxRevisionMacro(vtkKWVolumePropertyPresetSelector, "1.4");

//----------------------------------------------------------------------------
vtkKWVolumePropertyPresetSelector::~vtkKWVolumePropertyPresetSelector()
{
  // Delete all presets

  // We do not have much choice here but to call DeleteAllPresets(), even
  // though it is done in the destructor of the superclass too. The problem
  // with this code is that we override the virtual function DeAllocatePreset()
  // which is used by DeleteAllPresets(). At the time it is called by
  // the superclass, the virtual table of the subclass is gone, and
  // our DeAllocatePreset() is never called.

  this->DeleteAllPresets();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyPresetSelector::DeAllocatePreset(int id)
{
  this->Superclass::DeAllocatePreset(id);

  vtkVolumeProperty *ptr = this->GetPresetVolumeProperty(id);
  if (ptr)
    {
    ptr->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkKWVolumePropertyPresetSelector::SetPresetVolumeProperty(
  int id, vtkVolumeProperty *prop)
{
  if (this->HasPreset(id))
    {
    vtkVolumeProperty *ptr = this->GetPresetVolumeProperty(id);
    if (prop)
      {
      if (!ptr)
        {
        ptr = vtkVolumeProperty::New();
        }
      this->DeepCopyVolumeProperty(ptr, prop);
      }
    else
      {
      if (ptr)
        {
        ptr->Delete();
        ptr = NULL;
        }
      }
    this->SetPresetUserSlotAsPointer(id, "VolumeProperty", ptr);
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkVolumeProperty* vtkKWVolumePropertyPresetSelector::GetPresetVolumeProperty(
  int id)
{
  return (vtkVolumeProperty*)
    this->GetPresetUserSlotAsPointer(id, "VolumeProperty");
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyPresetSelector::DeepCopyVolumeProperty(
    vtkVolumeProperty *target, vtkVolumeProperty *source)
{
  if (!target || !source)
    {
    return;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  target->DeepCopy(source);
#else
  target->SetIndependentComponents(source->GetIndependentComponents());

  target->SetInterpolationType(source->GetInterpolationType());

  for (int i = 0; i < VTK_MAX_VRCOMP; i++)
    {
    target->SetComponentWeight(i, source->GetComponentWeight(i));
    
    // Force ColorChannels to the right value and/or create a default tfunc
    // then DeepCopy all the points

    if (source->GetColorChannels(i) > 1)
      {
      target->SetColor(i, target->GetRGBTransferFunction(i));
      target->GetRGBTransferFunction(i)->DeepCopy(
        source->GetRGBTransferFunction(i));
      }
    else
      {
      target->SetColor(i, target->GetGrayTransferFunction(i));
      target->GetGrayTransferFunction(i)->DeepCopy(
        source->GetGrayTransferFunction(i));
      }

    target->GetScalarOpacity(i)->DeepCopy(source->GetScalarOpacity(i));

    target->SetScalarOpacityUnitDistance(
      i, source->GetScalarOpacityUnitDistance(i));

    target->GetGradientOpacity(i)->DeepCopy(source->GetGradientOpacity(i));

    target->SetDisableGradientOpacity(i, source->GetDisableGradientOpacity(i));

    target->SetShade(i, source->GetShade(i));
    target->SetAmbient(i, source->GetAmbient(i));
    target->SetDiffuse(i, source->GetDiffuse(i));
    target->SetSpecular(i, source->GetSpecular(i));
    target->SetSpecularPower(i, source->GetSpecularPower(i));
    }
#endif
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
