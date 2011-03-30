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

#include "vtkObjectFactory.h"
#include "vtkMySpecialPolyDataMapper.h"

vtkStandardNewMacro(vtkMySpecialRepresentation);
//----------------------------------------------------------------------------
vtkMySpecialRepresentation::vtkMySpecialRepresentation()
{
  this->MyMapper = vtkMySpecialPolyDataMapper::New();
  this->MyLODMapper = vtkMySpecialPolyDataMapper::New();

  // Replace the mappers created by the superclass.
  this->SetMapper(this->MyMapper);
  this->SetLODMapper(this->MyLODMapper);
}

//----------------------------------------------------------------------------
vtkMySpecialRepresentation::~vtkMySpecialRepresentation()
{
  this->MyLODMapper->Delete();
  this->MyMapper->Delete();
}

//----------------------------------------------------------------------------
void vtkMySpecialRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
