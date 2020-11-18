/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRotateAroundOriginTransform

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRotateAroundOriginTransform.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRotateAroundOriginTransform);

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OriginOfRotation: (" << this->OriginOfRotation[0] << ", "
     << this->OriginOfRotation[1] << ", " << this->OriginOfRotation[2] << ")" << endl;
}

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::UpdateMatrix()
{
  this->AbsoluteTransform->Identity();

  this->AbsoluteTransform->Translate(this->OriginOfRotation);
  this->AbsoluteTransform->RotateZ(this->AbsoluteRotation[2]);
  this->AbsoluteTransform->RotateX(this->AbsoluteRotation[0]);
  this->AbsoluteTransform->RotateY(this->AbsoluteRotation[1]);
  this->AbsoluteTransform->Translate(
    -this->OriginOfRotation[0], -this->OriginOfRotation[1], -this->OriginOfRotation[2]);

  // Delegate the setting to the vtkMatrix
  this->SetMatrix(this->AbsoluteTransform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::SetOriginOfRotation(double xyz[3])
{
  this->SetOriginOfRotation(xyz[0], xyz[1], xyz[2]);
}

//----------------------------------------------------------------------------
void vtkPVRotateAroundOriginTransform::SetOriginOfRotation(double x, double y, double z)
{
  this->OriginOfRotation[0] = x;
  this->OriginOfRotation[1] = y;
  this->OriginOfRotation[2] = z;

  UpdateMatrix(); // This will call Modifed(), no need to do it twice
}
