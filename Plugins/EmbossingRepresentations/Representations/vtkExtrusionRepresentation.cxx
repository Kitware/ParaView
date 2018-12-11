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
void vtkExtrusionRepresentation::SetExtrusionFactor(double val)
{
  static_cast<vtkExtrusionMapper*>(this->Mapper)->SetExtrusionFactor(static_cast<float>(val));
  static_cast<vtkExtrusionMapper*>(this->LODMapper)->SetExtrusionFactor(static_cast<float>(val));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetBasisVisibility(bool val)
{
  static_cast<vtkExtrusionMapper*>(this->Mapper)->SetBasisVisibility(val);
  static_cast<vtkExtrusionMapper*>(this->LODMapper)->SetBasisVisibility(val);
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
  static_cast<vtkExtrusionMapper*>(this->Mapper)->SetNormalizeData(val);
  static_cast<vtkExtrusionMapper*>(this->LODMapper)->SetNormalizeData(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetAutoScaling(bool val)
{
  static_cast<vtkExtrusionMapper*>(this->Mapper)->SetAutoScaling(val);
  static_cast<vtkExtrusionMapper*>(this->LODMapper)->SetAutoScaling(val);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtrusionRepresentation::SetScalingRange(double minimum, double maximum)
{
  static_cast<vtkExtrusionMapper*>(this->Mapper)
    ->SetUserRange(static_cast<float>(minimum), static_cast<float>(maximum));
  static_cast<vtkExtrusionMapper*>(this->LODMapper)
    ->SetUserRange(static_cast<float>(minimum), static_cast<float>(maximum));
  this->Modified();
}
