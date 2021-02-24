/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCylinder

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVCylinder.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"
#include "vtkVector.h"

#include <algorithm>
#include <cmath>

vtkStandardNewMacro(vtkPVCylinder);

//----------------------------------------------------------------------------
vtkPVCylinder::vtkPVCylinder()
{
  this->OrientedAxis[0] = 0.;
  this->OrientedAxis[1] = 1.;
  this->OrientedAxis[2] = 0.;
}

//----------------------------------------------------------------------------
vtkPVCylinder::~vtkPVCylinder() = default;

//----------------------------------------------------------------------------
void vtkPVCylinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Oriented Axis: [ " << this->OrientedAxis[0] << "," << this->OrientedAxis[1]
     << "," << this->OrientedAxis[2] << " ]\n";
}

//----------------------------------------------------------------------------
void vtkPVCylinder::SetOrientedAxis(const double axis[3])
{
  if (!std::equal(axis, axis + 3, this->OrientedAxis))
  {
    std::copy(axis, axis + 3, this->OrientedAxis);
    this->UpdateTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVCylinder::SetCenter(double x, double y, double z)
{
  this->Superclass::SetCenter(x, y, z);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkPVCylinder::SetCenter(const double xyz[3])
{
  this->Superclass::SetCenter(xyz);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkPVCylinder::UpdateTransform()
{
  // The vtkCylinder is aligned to the y-axis. Setup a transform that rotates
  // <0, 1, 0> to the vector in Axis.
  const vtkVector3d yAxis(0., 1., 0.);
  vtkVector3d axis(this->OrientedAxis);
  axis.Normalize();

  // Calculate the rotation if needed:
  if (!std::equal(axis.GetData(), axis.GetData() + 3, yAxis.GetData()))
  {
    vtkVector3d cross = yAxis.Cross(axis);
    double crossNorm = cross.Normalize();
    double dot = yAxis.Dot(axis);
    double angle = vtkMath::DegreesFromRadians(std::atan2(crossNorm, dot));

    vtkNew<vtkTransform> xform;
    xform->Identity();
    xform->Translate(this->Center);
    xform->RotateWXYZ(angle, cross.GetData());
    xform->Translate(-this->Center[0], -this->Center[1], -this->Center[2]);

    xform->Inverse();
    this->SetTransform(xform.GetPointer());
  }
  else
  {
    this->SetTransform(static_cast<vtkAbstractTransform*>(nullptr));
  }
}
