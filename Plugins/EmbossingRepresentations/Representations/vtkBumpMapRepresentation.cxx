/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBumpMapRepresentation.h"

#include "vtkBumpMapMapper.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkBumpMapRepresentation);

//----------------------------------------------------------------------------
vtkBumpMapRepresentation::vtkBumpMapRepresentation()
{
  // Replace the mappers created by the superclass.
  this->Mapper->Delete();
  this->LODMapper->Delete();

  this->Mapper = vtkBumpMapMapper::New();
  this->LODMapper = vtkBumpMapMapper::New();

  // Since we replaced the mappers, we need to call SetupDefaults() to ensure
  // the pipelines are setup correctly.
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
void vtkBumpMapRepresentation::SetBumpMappingFactor(double val)
{
  static_cast<vtkBumpMapMapper*>(this->Mapper)->SetBumpMappingFactor(static_cast<float>(val));
  static_cast<vtkBumpMapMapper*>(this->LODMapper)->SetBumpMappingFactor(static_cast<float>(val));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkBumpMapRepresentation::SetInputDataArray(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Mapper->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->LODMapper->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->Modified();
}
