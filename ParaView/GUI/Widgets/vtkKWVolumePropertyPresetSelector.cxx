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
vtkCxxRevisionMacro(vtkKWVolumePropertyPresetSelector, "1.1");

//----------------------------------------------------------------------------
vtkKWVolumePropertyPresetSelector::~vtkKWVolumePropertyPresetSelector()
{
  // Remove all presets

  // We do not have much choice here but to call RemoveAllPresets(), even
  // though it is done in the destructor of the superclass too. The problem
  // with this code is that we override the virtual function DeAllocatePreset()
  // which is used by RemoveAllPresets(). At the time it is called by
  // the superclass, the virtual table of the subclass is gone, and
  // our DeAllocatePreset() is never called.

  this->RemoveAllPresets();
}

//----------------------------------------------------------------------------
void vtkKWVolumePropertyPresetSelector::DeAllocatePreset(int id)
{
  this->Superclass::DeAllocatePreset(id);

  vtkVolumeProperty *ptr = (vtkVolumeProperty*)
    this->GetPresetUserSlotAsPointer(id, "VolumeProperty");
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
    vtkVolumeProperty *ptr = (vtkVolumeProperty*)
      this->GetPresetUserSlotAsPointer(id, "VolumeProperty");
    if (!ptr)
      {
      ptr = vtkVolumeProperty::New();
      }
    ptr->DeepCopy(prop);
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
void vtkKWVolumePropertyPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
