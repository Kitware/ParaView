// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
