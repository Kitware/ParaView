// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVCenterAxesActor.h"

#include "vtkAxes.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"

vtkStandardNewMacro(vtkPVCenterAxesActor);
//----------------------------------------------------------------------------
vtkPVCenterAxesActor::vtkPVCenterAxesActor()
{
  this->Axes = vtkAxes::New();
  this->Axes->SetSymmetric(1);
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Axes->GetOutputPort());
  this->SetMapper(this->Mapper);
  // We disable this, since it results in the center axes being skipped when
  // IceT is rendering.
  // this->SetUseBounds(0); // don't use bounds of this actor in renderer bounds
  // computations.
}

//----------------------------------------------------------------------------
vtkPVCenterAxesActor::~vtkPVCenterAxesActor()
{
  this->Axes->Delete();
  this->Mapper->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetSymmetric(int val)
{
  this->Axes->SetSymmetric(val);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::SetComputeNormals(int val)
{
  this->Axes->SetComputeNormals(val);
}

//----------------------------------------------------------------------------
void vtkPVCenterAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
