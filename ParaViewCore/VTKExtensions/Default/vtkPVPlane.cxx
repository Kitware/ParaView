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
#include "vtkPVPlane.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVPlane);
//----------------------------------------------------------------------------
vtkPVPlane::vtkPVPlane()
{
  this->Plane = vtkPlane::New();
  this->Offset = 0;
}

//----------------------------------------------------------------------------
vtkPVPlane::~vtkPVPlane()
{
  this->Plane->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetTransform(vtkAbstractTransform* transform)
{
  this->Superclass::SetTransform(transform);
  this->Plane->SetTransform(transform);
}

//----------------------------------------------------------------------------
double vtkPVPlane::EvaluateFunction(double x[3])
{
  if (this->GetMTime() > this->Plane->GetMTime())
  {
    this->Plane->SetNormal(this->Normal);
    this->Plane->SetOrigin(this->Origin);
    this->Plane->Push(this->Offset);
  }

  return this->Plane->EvaluateFunction(x);
}

//----------------------------------------------------------------------------
void vtkPVPlane::EvaluateGradient(double x[3], double g[3])
{
  if (this->GetMTime() > this->Plane->GetMTime())
  {
    this->Plane->SetNormal(this->Normal);
    this->Plane->SetOrigin(this->Origin);
    this->Plane->Push(this->Offset);
  }

  this->Plane->EvaluateGradient(x, g);
}

//----------------------------------------------------------------------------
void vtkPVPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Offset: " << this->Offset << endl;
}
