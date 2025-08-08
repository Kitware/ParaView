// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExtrusionRepresentation.h"

#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkExtrusionMapper.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkExtrusionRepresentation);

//----------------------------------------------------------------------------
vtkExtrusionRepresentation::vtkExtrusionRepresentation()
{
  // Replace the mappers created by the superclass.
  this->Mapper->Delete();
  this->LODMapper->Delete();

  this->Mapper = vtkExtrusionMapper::New();
  this->LODMapper = vtkExtrusionMapper::New();

  // Since we replaced the mappers, we need to call SetupDefaults() to ensure
  // the pipelines are setup correctly.
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetExtrusionFactor(double val)
{
  vtkExtrusionMapper::SafeDownCast(this->Mapper)->SetExtrusionFactor(static_cast<float>(val));
  vtkExtrusionMapper::SafeDownCast(this->LODMapper)->SetExtrusionFactor(static_cast<float>(val));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetBasisVisibility(bool val)
{
  vtkExtrusionMapper::SafeDownCast(this->Mapper)->SetBasisVisibility(val);
  vtkExtrusionMapper::SafeDownCast(this->LODMapper)->SetBasisVisibility(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetInputDataArray(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Mapper->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->LODMapper->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetNormalizeData(bool val)
{
  vtkExtrusionMapper::SafeDownCast(this->Mapper)->SetNormalizeData(val);
  vtkExtrusionMapper::SafeDownCast(this->LODMapper)->SetNormalizeData(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetAutoScaling(bool val)
{
  vtkExtrusionMapper::SafeDownCast(this->Mapper)->SetAutoScaling(val);
  vtkExtrusionMapper::SafeDownCast(this->LODMapper)->SetAutoScaling(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetScalingRange(double minimum, double maximum)
{
  vtkExtrusionMapper::SafeDownCast(this->Mapper)
    ->SetUserRange(static_cast<float>(minimum), static_cast<float>(maximum));
  vtkExtrusionMapper::SafeDownCast(this->LODMapper)
    ->SetUserRange(static_cast<float>(minimum), static_cast<float>(maximum));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetRecalculateNormals(bool enable)
{
  vtkExtrusionMapper::SafeDownCast(this->Mapper)->SetRecalculateNormals(enable);
  vtkExtrusionMapper::SafeDownCast(this->LODMapper)->SetRecalculateNormals(enable);
  this->Modified();
}
