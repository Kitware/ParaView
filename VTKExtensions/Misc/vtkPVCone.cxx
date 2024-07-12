// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVCone.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkVector.h"

#include <cmath>

vtkStandardNewMacro(vtkPVCone);

//----------------------------------------------------------------------------
void vtkPVCone::SetAxis(double axis[3])
{
  this->Superclass::SetAxis(axis);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkPVCone::SetAxis(double x, double y, double z)
{
  this->Superclass::SetAxis(x, y, z);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkPVCone::SetOrigin(double x, double y, double z)
{
  this->Superclass::SetOrigin(x, y, z);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkPVCone::SetOrigin(const double xyz[3])
{
  this->Superclass::SetOrigin(xyz);
  this->UpdateTransform();
}

//------------------------------------------------------------------------------
double vtkPVCone::EvaluateFunction(double x[3])
{
  if (x[0] < 0.)
  {
    return 0;
  }
  return this->Superclass::EvaluateFunction(x);
}

//------------------------------------------------------------------------------
void vtkPVCone::EvaluateGradient(double x[3], double g[3])
{
  if (x[0] < 0.)
  {
    g[0] = 0.;
    g[1] = 0.;
    g[2] = 0.;
  }
  return this->Superclass::EvaluateGradient(x, g);
}

//----------------------------------------------------------------------------
void vtkPVCone::UpdateTransform()
{
  // vtkCone is aligned to the x-axis. Setup a transform that rotates
  // <1, 0, 0> to the vector in Axis and translates according to origin
  const vtkVector3d xAxis(1., 0., 0.);
  vtkVector3d axis(this->Axis);
  axis.Normalize();

  vtkVector3d cross = xAxis.Cross(axis);
  double crossNorm = cross.Normalize();
  double dot = xAxis.Dot(axis);
  double angle = vtkMath::DegreesFromRadians(std::atan2(crossNorm, dot));

  vtkNew<vtkTransform> xform;
  xform->Identity();
  xform->Translate(this->Origin);
  xform->RotateWXYZ(angle, cross.GetData());
  xform->Inverse();

  this->SetTransform(xform.GetPointer());
  this->Modified();
}
