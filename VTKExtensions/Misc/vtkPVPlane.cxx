// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVPlane.h"

#include "vtkObjectFactory.h"

#include <cmath>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPlane);

//----------------------------------------------------------------------------
vtkPVPlane::vtkPVPlane()
  : Offset(0.0)
  , AxisAligned(false)
{
}

//----------------------------------------------------------------------------
vtkPVPlane::~vtkPVPlane() = default;

//----------------------------------------------------------------------------
void vtkPVPlane::InternalPlaneUpdate()
{
  this->Plane->SetNormal(this->Normal);
  this->Plane->SetOrigin(this->Origin);
  this->Plane->Push(this->Offset);
  this->Plane->SetTransform(this->Transform);
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetOffset(double offset)
{
  if (this->Offset != offset)
  {
    this->Offset = offset;
    this->Modified();
    this->InternalPlaneUpdate();
  }
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetOrigin(double x, double y, double z)
{
  this->Superclass::SetOrigin(x, y, z);
  this->InternalPlaneUpdate();
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetNormal(double x, double y, double z)
{
  if (this->AxisAligned)
  {
    x = std::fabs(x) >= std::fabs(y) && std::fabs(x) >= std::fabs(z) ? 1 : 0;
    y = std::fabs(y) >= std::fabs(x) && std::fabs(y) >= std::fabs(z) ? 1 : 0;
    z = std::fabs(z) >= std::fabs(x) && std::fabs(z) >= std::fabs(y) ? 1 : 0;
  }
  this->Superclass::SetNormal(x, y, z);
  this->InternalPlaneUpdate();
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetAxisAligned(bool axisAligned)
{
  if (this->AxisAligned != axisAligned)
  {
    this->AxisAligned = axisAligned;
    this->Modified();
    this->SetNormal(this->GetNormal());
  }
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetTransform(vtkAbstractTransform* transform)
{
  this->Superclass::SetTransform(transform);
  this->InternalPlaneUpdate();
}

//----------------------------------------------------------------------------
void vtkPVPlane::SetTransform(const double elements[16])
{
  this->Superclass::SetTransform(elements);
  this->InternalPlaneUpdate();
}

//----------------------------------------------------------------------------
void vtkPVPlane::EvaluateFunction(vtkDataArray* input, vtkDataArray* output)
{
  return this->Plane->EvaluateFunction(input, output);
}
//----------------------------------------------------------------------------
double vtkPVPlane::EvaluateFunction(double x[3])
{
  return this->Plane->EvaluateFunction(x);
}

//----------------------------------------------------------------------------
void vtkPVPlane::EvaluateGradient(double x[3], double g[3])
{
  this->Plane->EvaluateGradient(x, g);
}

//----------------------------------------------------------------------------
void vtkPVPlane::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Offset: " << this->Offset << endl;
  os << indent << "AxisAligned: " << (this->AxisAligned ? "On" : "Off") << endl;
  this->Plane->PrintSelf(os, indent);
}
