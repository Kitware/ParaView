// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPrismSelectionRepresentation.h"

#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPrismGeometryRepresentation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPrismSelectionRepresentation);

//----------------------------------------------------------------------------
vtkPrismSelectionRepresentation::vtkPrismSelectionRepresentation()
{
  this->GeometryRepresentation->Delete();
  this->GeometryRepresentation = vtkPrismGeometryRepresentation::New();
  this->GeometryRepresentation->SetPickable(0);
  this->GeometryRepresentation->RequestGhostCellsIfNeededOff();

  vtkCommand* observer =
    vtkMakeMemberFunctionCommand(*this, &vtkPrismSelectionRepresentation::TriggerUpdateDataEvent);
  this->GeometryRepresentation->AddObserver(vtkCommand::UpdateDataEvent, observer);
  observer->Delete();
}

//----------------------------------------------------------------------------
vtkPrismSelectionRepresentation::~vtkPrismSelectionRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::TriggerUpdateDataEvent()
{
  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxy will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//------------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::SetIsSimulationData(bool isSimulationData)
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    if (geomRepr->GetIsSimulationData() != isSimulationData)
    {
      geomRepr->SetIsSimulationData(isSimulationData);
      this->MarkModified();
    }
  }
}

//------------------------------------------------------------------------------
bool vtkPrismSelectionRepresentation::GetIsSimulationData()
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    return geomRepr->GetIsSimulationData();
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::SetAttributeType(int type)
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    if (geomRepr->GetAttributeType() != type)
    {
      geomRepr->SetAttributeType(type);
      this->MarkModified();
    }
  }
}

//------------------------------------------------------------------------------
int vtkPrismSelectionRepresentation::GetAttributeType()
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    return geomRepr->GetAttributeType();
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::SetXArrayName(const char* name)
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    if (!geomRepr->GetXArrayName() || !name || strcmp(geomRepr->GetXArrayName(), name) != 0)
    {
      geomRepr->SetXArrayName(name);
      this->MarkModified();
    }
  }
}

//------------------------------------------------------------------------------
const char* vtkPrismSelectionRepresentation::GetXArrayName()
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    return geomRepr->GetXArrayName();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::SetYArrayName(const char* name)
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    if (!geomRepr->GetYArrayName() || !name || strcmp(geomRepr->GetYArrayName(), name) != 0)
    {
      geomRepr->SetYArrayName(name);
      this->MarkModified();
    }
  }
}

//------------------------------------------------------------------------------
const char* vtkPrismSelectionRepresentation::GetYArrayName()
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    return geomRepr->GetYArrayName();
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkPrismSelectionRepresentation::SetZArrayName(const char* name)
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    if (!geomRepr->GetZArrayName() || !name || strcmp(geomRepr->GetZArrayName(), name) != 0)
    {
      geomRepr->SetZArrayName(name);
      this->MarkModified();
    }
  }
}

//------------------------------------------------------------------------------
const char* vtkPrismSelectionRepresentation::GetZArrayName()
{
  if (auto geomRepr = vtkPrismGeometryRepresentation::SafeDownCast(this->GeometryRepresentation))
  {
    return geomRepr->GetZArrayName();
  }
  return nullptr;
}
