/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTransform

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTransform.h"

#include "vtkObjectFactory.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTransform);
//----------------------------------------------------------------------------
vtkPVTransform::vtkPVTransform()
{
  this->AbsoluteTransform = vtkTransform::New();

  // Set position to 0
  this->AbsolutePosition[0] = this->AbsolutePosition[1] = this->AbsolutePosition[2] = 0;

  // Set rotation to 0
  this->AbsoluteRotation[0] = this->AbsoluteRotation[1] = this->AbsoluteRotation[2] = 0;

  // Set scale to 1
  this->AbsoluteScale[0] = this->AbsoluteScale[1] = this->AbsoluteScale[2] = 1;
}

//----------------------------------------------------------------------------
vtkPVTransform::~vtkPVTransform()
{
  this->AbsoluteTransform->Delete();
}

//----------------------------------------------------------------------------
void vtkPVTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVTransform::UpdateMatrix()
{
  this->AbsoluteTransform->Identity();
  this->AbsoluteTransform->Translate(this->AbsolutePosition);
  this->AbsoluteTransform->RotateZ(this->AbsoluteRotation[2]);
  this->AbsoluteTransform->RotateX(this->AbsoluteRotation[0]);
  this->AbsoluteTransform->RotateY(this->AbsoluteRotation[1]);
  this->AbsoluteTransform->Scale(this->AbsoluteScale);

  // Delegate the setting to the vtkMatrix
  this->SetMatrix(this->AbsoluteTransform->GetMatrix());
}

//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsolutePosition(double xyz[3])
{
  this->SetAbsolutePosition(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteRotation(double xyz[3])
{
  this->SetAbsoluteRotation(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteScale(double xyz[3])
{
  this->SetAbsoluteScale(xyz[0], xyz[1], xyz[2]);
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsolutePosition(double x, double y, double z)
{
  this->AbsolutePosition[0] = x;
  this->AbsolutePosition[1] = y;
  this->AbsolutePosition[2] = z;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteRotation(double x, double y, double z)
{
  this->AbsoluteRotation[0] = x;
  this->AbsoluteRotation[1] = y;
  this->AbsoluteRotation[2] = z;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
//----------------------------------------------------------------------------
void vtkPVTransform::SetAbsoluteScale(double x, double y, double z)
{
  this->AbsoluteScale[0] = x;
  this->AbsoluteScale[1] = y;
  this->AbsoluteScale[2] = z;

  UpdateMatrix(); // This will call modified... No need to do it twice
}
