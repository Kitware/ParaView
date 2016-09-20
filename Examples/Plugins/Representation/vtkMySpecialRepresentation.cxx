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
vtkMySpecialRepresentation::~vtkMySpecialRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkMySpecialRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
