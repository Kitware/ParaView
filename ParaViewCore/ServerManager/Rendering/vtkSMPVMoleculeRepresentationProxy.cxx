/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVMoleculeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVMoleculeRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMTrace.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cstring>

vtkStandardNewMacro(vtkSMPVMoleculeRepresentationProxy);

bool MapperParametersPreset::IsSimilar(const MapperParametersPreset& p) const
{
  return (p.RenderAtoms == this->RenderAtoms && p.RenderBonds == this->RenderBonds &&
    p.AtomicRadiusType == this->AtomicRadiusType &&
    p.AtomicRadiusFactor == this->AtomicRadiusFactor &&
    p.UseMultiCylindersForBonds == this->UseMultiCylindersForBonds &&
    p.BondRadius == this->BondRadius && p.UseAtomColorForBonds == this->UseAtomColorForBonds);
}

//----------------------------------------------------------------------------
vtkSMPVMoleculeRepresentationProxy::vtkSMPVMoleculeRepresentationProxy()
{
  this->Presets[Preset::BallAndStick] = MapperParametersPreset{ true, true, 1, 0.3, true, 0.075,
    true, "BallAndStick", "Ball and Stick" };
  this->Presets[Preset::Liquorice] =
    MapperParametersPreset{ true, true, 2, 0.15, false, 0.15, true, "Liquorice", "Liquorice" };
  this->Presets[Preset::VanDerWaals] =
    MapperParametersPreset{ true, false, 1, 1., true, 0.075, true, "VanDerWaals", "Van der Waals" };
  this->Presets[Preset::Fast] =
    MapperParametersPreset{ true, true, 2, 0.6, false, 0.075, false, "Fast", "Fast" };
}

//----------------------------------------------------------------------------
void vtkSMPVMoleculeRepresentationProxy::SetPreset(int preset)
{
  if (this->Presets.count(preset) == 0 || preset == Preset::None)
  {
    return;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("SetPreset")
    .arg(this->GetPresetName(preset))
    .arg("comment", "Set the preset");

  const MapperParametersPreset& parameters = this->Presets[preset];
  vtkSMPropertyHelper(this, "RenderAtoms").Set(parameters.RenderAtoms);
  vtkSMPropertyHelper(this, "RenderBonds").Set(parameters.RenderBonds);
  vtkSMPropertyHelper(this, "AtomicRadiusType").Set(parameters.AtomicRadiusType);
  vtkSMPropertyHelper(this, "AtomicRadiusFactor").Set(parameters.AtomicRadiusFactor);
  vtkSMPropertyHelper(this, "MultiCylindersForBonds").Set(parameters.UseMultiCylindersForBonds);
  vtkSMPropertyHelper(this, "BondRadius").Set(parameters.BondRadius);
  if (!parameters.UseAtomColorForBonds)
  {
    double color[3] = { 1., 1., 1. };
    vtkSMPropertyHelper(this, "BondColor").Set(color, 3);
  }
  vtkSMPropertyHelper(this, "BondColorMode").Set(parameters.UseAtomColorForBonds ? 1 : 0);

  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMPVMoleculeRepresentationProxy::SetPreset(const char* name)
{
  for (auto p : this->Presets)
  {
    if (std::strcmp(name, p.second.Name.c_str()) == 0)
    {
      this->SetPreset(p.first);
      return;
    }
  }
}

//----------------------------------------------------------------------------
const char* vtkSMPVMoleculeRepresentationProxy::GetPresetName(int preset)
{
  auto p = this->Presets.find(preset);
  if (p != this->Presets.end())
  {
    return p->second.Name.c_str();
  }

  return "None";
}

//----------------------------------------------------------------------------
const char* vtkSMPVMoleculeRepresentationProxy::GetPresetDisplayName(int preset)
{
  auto p = this->Presets.find(preset);
  if (p != this->Presets.end())
  {
    return p->second.DisplayName.c_str();
  }

  return "(no preset)";
}

//----------------------------------------------------------------------------
int vtkSMPVMoleculeRepresentationProxy::GetCurrentPreset()
{
  MapperParametersPreset p;
  p.RenderAtoms = vtkSMUncheckedPropertyHelper(this, "RenderAtoms").GetAsInt() == 1;
  p.RenderBonds = vtkSMUncheckedPropertyHelper(this, "RenderBonds").GetAsInt() == 1;
  p.AtomicRadiusType = vtkSMUncheckedPropertyHelper(this, "AtomicRadiusType").GetAsDouble();
  p.AtomicRadiusFactor = vtkSMUncheckedPropertyHelper(this, "AtomicRadiusFactor").GetAsDouble();
  p.UseMultiCylindersForBonds =
    vtkSMUncheckedPropertyHelper(this, "MultiCylindersForBonds").GetAsInt() == 1;
  p.BondRadius = vtkSMUncheckedPropertyHelper(this, "BondRadius").GetAsDouble();
  p.UseAtomColorForBonds = vtkSMUncheckedPropertyHelper(this, "BondColorMode").GetAsInt() == 1;

  for (const auto& preset : this->Presets)
  {
    if (preset.second.IsSimilar(p))
    {
      return preset.first;
    }
  }

  return Preset::None;
}

//----------------------------------------------------------------------------
void vtkSMPVMoleculeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
