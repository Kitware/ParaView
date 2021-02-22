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
#include "vtkPVBox.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkTransform.h"

#include <algorithm>

vtkStandardNewMacro(vtkPVBox);
//----------------------------------------------------------------------------
vtkPVBox::vtkPVBox()
{
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Rotation[0] = this->Rotation[1] = this->Rotation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;

  vtkMath::UninitializeBounds(this->ReferenceBounds);
  this->UseReferenceBounds = false;
}

//----------------------------------------------------------------------------
vtkPVBox::~vtkPVBox() = default;

//----------------------------------------------------------------------------
void vtkPVBox::SetUseReferenceBounds(bool val)
{
  if (this->UseReferenceBounds != val)
  {
    this->UseReferenceBounds = val;
    this->UpdateTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVBox::SetReferenceBounds(const double bds[6])
{
  if (!std::equal(bds, bds + 6, this->ReferenceBounds))
  {
    std::copy(bds, bds + 6, this->ReferenceBounds);
    this->UpdateTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVBox::SetPosition(const double pos[3])
{
  if (!std::equal(pos, pos + 3, this->Position))
  {
    std::copy(pos, pos + 3, this->Position);
    this->UpdateTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVBox::SetRotation(const double pos[3])
{
  if (!std::equal(pos, pos + 3, this->Rotation))
  {
    std::copy(pos, pos + 3, this->Rotation);
    this->UpdateTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVBox::SetScale(const double pos[3])
{
  if (!std::equal(pos, pos + 3, this->Scale))
  {
    std::copy(pos, pos + 3, this->Scale);
    this->UpdateTransform();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVBox::UpdateTransform()
{
  auto trans = vtkTransform::SafeDownCast(this->GetTransform());
  if (trans == nullptr)
  {
    trans = vtkTransform::New();
    this->SetTransform(trans);
    trans->Delete();
  }

  if (this->UseReferenceBounds)
  {
    this->SetBounds(this->ReferenceBounds);
  }
  else
  {
    this->SetBounds(0, 1, 0, 1, 0, 1);
  }

  trans->Identity();
  trans->Translate(this->Position);
  trans->RotateZ(this->Rotation[2]);
  trans->RotateX(this->Rotation[0]);
  trans->RotateY(this->Rotation[1]);
  trans->Scale(this->Scale);
  trans->Inverse();
}

//----------------------------------------------------------------------------
void vtkPVBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Position: " << this->Position[0] << "," << this->Position[1] << ","
     << this->Position[2] << endl;
  os << indent << "Rotation: " << this->Rotation[0] << "," << this->Rotation[1] << ","
     << this->Rotation[2] << endl;
  os << indent << "Scale: " << this->Scale[0] << "," << this->Scale[1] << "," << this->Scale[2]
     << endl;
}
