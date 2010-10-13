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
#include "vtkSurfaceLICRepresentation.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkObjectFactory.h"
#include "vtkSurfaceLICDefaultPainter.h"
#include "vtkSurfaceLICPainter.h"

vtkStandardNewMacro(vtkSurfaceLICRepresentation);
//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::vtkSurfaceLICRepresentation()
{
  this->Painter = vtkSurfaceLICPainter::New();
  this->LODPainter = vtkSurfaceLICPainter::New();
  this->LODPainter->SetEnhancedLIC(0);
  this->LODPainter->SetEnable(0);
  this->UseLICForLOD = false;

  vtkSurfaceLICDefaultPainter* painter = vtkSurfaceLICDefaultPainter::New();
  painter->SetSurfaceLICPainter(this->Painter);
  painter->SetDelegatePainter(this->Mapper->GetPainter()->GetDelegatePainter());
  this->Mapper->SetPainter(painter);
  painter->Delete();

  painter = vtkSurfaceLICDefaultPainter::New();
  painter->SetSurfaceLICPainter(this->LODPainter);
  painter->SetDelegatePainter(this->LODMapper->GetPainter()->GetDelegatePainter());
  this->LODMapper->SetPainter(painter);
  painter->Delete();
}

//----------------------------------------------------------------------------
vtkSurfaceLICRepresentation::~vtkSurfaceLICRepresentation()
{
  this->Painter->Delete();
  this->LODPainter->Delete();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetUseLICForLOD(bool val)
{
  this->UseLICForLOD = val;
  this->LODPainter->SetEnable(this->Painter->GetEnable() && this->UseLICForLOD);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetEnable(bool val)
{
  this->Painter->SetEnable(val);
  this->LODPainter->SetEnable(this->Painter->GetEnable() && this->UseLICForLOD);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetNumberOfSteps(int val)
{
  this->Painter->SetNumberOfSteps(val);
  this->LODPainter->SetNumberOfSteps(val);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetStepSize(double val)
{
  this->Painter->SetStepSize(val);
  this->LODPainter->SetStepSize(val);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetLICIntensity(double val)
{
  this->Painter->SetLICIntensity(val);
  this->LODPainter->SetLICIntensity(val);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SetEnhancedLIC(int val)
{
  this->Painter->SetEnhancedLIC(val);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICRepresentation::SelectInputVectors(int, int, int,
  int attributeMode, const char* name)
{
  this->Painter->SetInputArrayToProcess(attributeMode, name);
  this->LODPainter->SetInputArrayToProcess(attributeMode, name);
}
