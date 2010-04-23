/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBoxProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMBoxProxy);

//----------------------------------------------------------------------------
vtkSMBoxProxy::vtkSMBoxProxy()
{
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Rotation[0] = this->Rotation[1] = this->Rotation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkSMBoxProxy::~vtkSMBoxProxy()
{

}

//----------------------------------------------------------------------------
void vtkSMBoxProxy::UpdateVTKObjects(vtkClientServerStream& stream)
{
  this->Superclass::UpdateVTKObjects(stream);
  
  vtkMatrix4x4* mat = vtkMatrix4x4::New();
  this->GetMatrix(mat);
   
  stream  << vtkClientServerStream::Invoke
          << this->GetID() 
          << "SetTransform"
          << vtkClientServerStream::InsertArray(&(mat->Element[0][0]),16)
          << vtkClientServerStream::End;
  mat->Delete();
}

//----------------------------------------------------------------------------
void vtkSMBoxProxy::GetMatrix(vtkMatrix4x4* mat)
{
  vtkTransform* trans = vtkTransform::New();
  trans->Identity();
  trans->Translate(this->Position);
  trans->RotateZ(this->Rotation[2]);
  trans->RotateX(this->Rotation[0]);
  trans->RotateY(this->Rotation[1]);
  trans->Scale(this->Scale);
  mat->DeepCopy(trans->GetMatrix());
  mat->Invert();
  trans->Delete();
}

//----------------------------------------------------------------------------
void vtkSMBoxProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Position: " << this->Position[0] << ","
                               << this->Position[1] << ","
                               << this->Position[2] << endl;
  os << indent << "Rotation: " << this->Rotation[0] << ","
                               << this->Rotation[1] << ","
                               << this->Rotation[2] << endl;
  os << indent << "Scale: " << this->Scale[0] << ","
                            << this->Scale[1] << ","
                            << this->Scale[2] << endl;
}
