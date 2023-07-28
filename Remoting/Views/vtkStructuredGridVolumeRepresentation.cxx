// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredGridVolumeRepresentation.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkStructuredGridVolumeRepresentation);
//----------------------------------------------------------------------------
vtkStructuredGridVolumeRepresentation::vtkStructuredGridVolumeRepresentation() = default;

//----------------------------------------------------------------------------
vtkStructuredGridVolumeRepresentation::~vtkStructuredGridVolumeRepresentation() = default;

//----------------------------------------------------------------------------
int vtkStructuredGridVolumeRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkStructuredGridVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
