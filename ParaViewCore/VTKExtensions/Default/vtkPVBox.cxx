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

#include "vtkObjectFactory.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPVBox);
//----------------------------------------------------------------------------
vtkPVBox::vtkPVBox()
{
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Rotation[0] = this->Rotation[1] = this->Rotation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkPVBox::~vtkPVBox()
{
}

//----------------------------------------------------------------------------
void vtkPVBox::SetPosition(const double pos[3])
{
  memcpy(this->Position, pos, sizeof(double) * 3);
  this->UpdateTransform();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBox::SetRotation(const double pos[3])
{
  memcpy(this->Rotation, pos, sizeof(double) * 3);
  this->UpdateTransform();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBox::SetScale(const double pos[3])
{
  memcpy(this->Scale, pos, sizeof(double) * 3);
  this->UpdateTransform();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVBox::UpdateTransform()
{
  vtkTransform* trans = vtkTransform::New();
  trans->Identity();
  trans->Translate(this->Position);
  trans->RotateZ(this->Rotation[2]);
  trans->RotateX(this->Rotation[0]);
  trans->RotateY(this->Rotation[1]);
  trans->Scale(this->Scale);
  trans->Inverse();
  this->SetTransform(trans);
  trans->Delete();
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
