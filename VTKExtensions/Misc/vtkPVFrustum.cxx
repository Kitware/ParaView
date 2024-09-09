// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVFrustum.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkVector.h"

vtkStandardNewMacro(vtkPVFrustum);

//----------------------------------------------------------------------------
void vtkPVFrustum::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Origin: " << this->Origin << std::endl;
  os << indent << "Orientation: " << this->Orientation << std::endl;
}

//----------------------------------------------------------------------------
void vtkPVFrustum::SetOrientation(const vtkVector3d& xyz)
{
  if (this->Orientation != xyz)
  {
    this->Orientation = xyz;
    this->UpdateTransform();
  }
}

//----------------------------------------------------------------------------
void vtkPVFrustum::SetOrientation(double x, double y, double z)
{
  this->SetOrientation(vtkVector3d(x, y, z));
}

//----------------------------------------------------------------------------
void vtkPVFrustum::SetOrientation(const double xyz[3])
{
  this->SetOrientation(vtkVector3d(xyz));
}

//----------------------------------------------------------------------------
double* vtkPVFrustum::GetOrientation()
{
  return this->Orientation.GetData();
}

//----------------------------------------------------------------------------
void vtkPVFrustum::GetOrientation(double& x, double& y, double& z)
{
  x = this->Orientation[0];
  y = this->Orientation[1];
  z = this->Orientation[2];
}

//----------------------------------------------------------------------------
void vtkPVFrustum::GetOrientation(double xyz[3])
{
  xyz[0] = this->Orientation[0];
  xyz[1] = this->Orientation[1];
  xyz[2] = this->Orientation[2];
}

//----------------------------------------------------------------------------
void vtkPVFrustum::SetOrigin(const vtkVector3d& xyz)
{
  if (this->Origin != xyz)
  {
    this->Origin = xyz;
    this->UpdateTransform();
  }
}

//----------------------------------------------------------------------------
void vtkPVFrustum::SetOrigin(double x, double y, double z)
{
  this->SetOrigin(vtkVector3d(x, y, z));
}

//----------------------------------------------------------------------------
void vtkPVFrustum::SetOrigin(const double xyz[3])
{
  this->SetOrigin(vtkVector3d(xyz));
}

//----------------------------------------------------------------------------
double* vtkPVFrustum::GetOrigin()
{
  return this->Origin.GetData();
}

//----------------------------------------------------------------------------
void vtkPVFrustum::GetOrigin(double& x, double& y, double& z)
{
  x = this->Origin[0];
  y = this->Origin[1];
  z = this->Origin[2];
}

//----------------------------------------------------------------------------
void vtkPVFrustum::GetOrigin(double xyz[3])
{
  xyz[0] = this->Origin[0];
  xyz[1] = this->Origin[1];
  xyz[2] = this->Origin[2];
}

//----------------------------------------------------------------------------
void vtkPVFrustum::UpdateTransform()
{
  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->Translate(this->Origin.GetData());
  transform->RotateZ(this->Orientation.GetZ());
  transform->RotateX(this->Orientation.GetX());
  transform->RotateY(this->Orientation.GetY());
  transform->Inverse();

  this->SetTransform(transform);
  this->Modified();
}
