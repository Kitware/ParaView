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
#include "vtkPVFrustumActor.h"

#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

vtkStandardNewMacro(vtkPVFrustumActor);
//----------------------------------------------------------------------------
vtkPVFrustumActor::vtkPVFrustumActor()
{
  this->SetPickable(0);
  this->Outline = vtkOutlineSource::New();
  this->Outline->SetBoxType(1);

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Outline->GetOutputPort());
  this->SetMapper(this->Mapper);

  this->GetProperty()->SetRepresentationToWireframe();
}

//----------------------------------------------------------------------------
vtkPVFrustumActor::~vtkPVFrustumActor()
{
  this->Outline->Delete();
  this->Mapper->Delete();
}

//----------------------------------------------------------------------------
void vtkPVFrustumActor::SetFrustum(double corners[24])
{
  this->Outline->SetCorners(corners);
}

//----------------------------------------------------------------------------
void vtkPVFrustumActor::SetColor(double r, double g, double b)
{
  this->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPVFrustumActor::SetLineWidth(double r)
{
  this->GetProperty()->SetLineWidth(r);
}
//----------------------------------------------------------------------------
void vtkPVFrustumActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
