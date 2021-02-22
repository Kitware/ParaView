/*=========================================================================

  Program:   ParaView
  Module:    vtkStructuredGridVolumeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
