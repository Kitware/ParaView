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
#include "vtkPrismCubeAxesRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkCubeAxesActor.h"

vtkStandardNewMacro(vtkPrismCubeAxesRepresentation);
//----------------------------------------------------------------------------
vtkPrismCubeAxesRepresentation::vtkPrismCubeAxesRepresentation()
{
}

//----------------------------------------------------------------------------
vtkPrismCubeAxesRepresentation::~vtkPrismCubeAxesRepresentation()
{
}

//***************************************************************************
// Forwarded to internal vtkPrismCubeAxesActor
//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::SetLabelRanges(
  double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  this->CubeAxesActor->SetXAxisRange(xmin,xmax);
  this->CubeAxesActor->SetYAxisRange(ymin,ymax);
  this->CubeAxesActor->SetZAxisRange(zmin,zmax);
}

//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
