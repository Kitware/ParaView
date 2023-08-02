// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBivariateNoiseRepresentation.h"

#include "vtkBivariateNoiseMapper.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkBivariateNoiseRepresentation);
//----------------------------------------------------------------------------
vtkBivariateNoiseRepresentation::vtkBivariateNoiseRepresentation()
{
  // Replace the mappers created by the superclass.
  this->Mapper->Delete();
  this->LODMapper->Delete();

  this->Mapper = vtkBivariateNoiseMapper::New();
  this->LODMapper = vtkBivariateNoiseMapper::New();

  // Since we replaced the mappers, we need to call SetupDefaults() to ensure
  // the pipelines are setup correctly.
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkBivariateNoiseRepresentation::~vtkBivariateNoiseRepresentation() = default;

//----------------------------------------------------------------------------
void vtkBivariateNoiseRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* attributeTypeorName)
{
  if (idx == 1 && fieldAssociation == vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS)
  {
    // Pass the noise array to the mapper (idx == 1)
    this->Mapper->SetInputArrayToProcess(
      1, port, connection, fieldAssociation, attributeTypeorName);
    this->LODMapper->SetInputArrayToProcess(
      1, port, connection, fieldAssociation, attributeTypeorName);
  }
  else
  {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, attributeTypeorName);
  }
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseRepresentation::SetFrequency(double frequency)
{
  vtkBivariateNoiseMapper::SafeDownCast(this->Mapper)->SetFrequency(frequency);
  vtkBivariateNoiseMapper::SafeDownCast(this->LODMapper)->SetFrequency(frequency);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseRepresentation::SetAmplitude(double amplitude)
{
  vtkBivariateNoiseMapper::SafeDownCast(this->Mapper)->SetAmplitude(amplitude);
  vtkBivariateNoiseMapper::SafeDownCast(this->LODMapper)->SetAmplitude(amplitude);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseRepresentation::SetSpeed(double speed)
{
  vtkBivariateNoiseMapper::SafeDownCast(this->Mapper)->SetSpeed(speed);
  vtkBivariateNoiseMapper::SafeDownCast(this->LODMapper)->SetSpeed(speed);
}

//----------------------------------------------------------------------------
void vtkBivariateNoiseRepresentation::SetNbOfOctaves(int nbOctaves)
{
  vtkBivariateNoiseMapper::SafeDownCast(this->Mapper)->SetNbOfOctaves(nbOctaves);
  vtkBivariateNoiseMapper::SafeDownCast(this->LODMapper)->SetNbOfOctaves(nbOctaves);
}
