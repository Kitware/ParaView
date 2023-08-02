// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMySpecialRepresentation.h"

#include "vtkMySpecialPolyDataMapper.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMySpecialRepresentation);
//----------------------------------------------------------------------------
vtkMySpecialRepresentation::vtkMySpecialRepresentation()
{
  // Replace the mappers created by the superclass.
  this->Mapper->Delete();
  this->LODMapper->Delete();

  this->Mapper = vtkMySpecialPolyDataMapper::New();
  this->LODMapper = vtkMySpecialPolyDataMapper::New();

  // Since we replaced the mappers, we need to call SetupDefaults() to ensure
  // the pipelines are setup correctly.
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkMySpecialRepresentation::~vtkMySpecialRepresentation() = default;

//----------------------------------------------------------------------------
void vtkMySpecialRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
