// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkPVTrackballMultiRotate.h"

#include "vtkObjectFactory.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkRenderer.h"

#define MY_MAX(x, y) ((x) < (y) ? (y) : (x))
#define MY_SQR(x) ((x) * (x))

//=============================================================================
vtkStandardNewMacro(vtkPVTrackballMultiRotate);

//-----------------------------------------------------------------------------
vtkPVTrackballMultiRotate::vtkPVTrackballMultiRotate()
{
  this->RotateManipulator = vtkPVTrackballRotate::New();
  this->RollManipulator = vtkPVTrackballRoll::New();
  this->CurrentManipulator = nullptr;
}

vtkPVTrackballMultiRotate::~vtkPVTrackballMultiRotate()
{
  this->CurrentManipulator = nullptr;
  this->RotateManipulator->Delete();
  this->RollManipulator->Delete();
}

void vtkPVTrackballMultiRotate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkPVTrackballMultiRotate::OnButtonDown(
  int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  int* viewSize;
  viewSize = ren->GetSize();
  double viewCenter[2];
  viewCenter[0] = 0.5 * viewSize[0];
  viewCenter[1] = 0.5 * viewSize[1];
  double rotateRadius = 0.9 * (MY_MAX(viewCenter[0], viewCenter[1]));
  double dist2 = MY_SQR(viewCenter[0] - x) + MY_SQR(viewCenter[1] - y);

  if (rotateRadius * rotateRadius > dist2)
  {
    this->CurrentManipulator = this->RotateManipulator;
  }
  else
  {
    this->CurrentManipulator = this->RollManipulator;
  }

  this->CurrentManipulator->SetButton(this->GetButton());
  this->CurrentManipulator->SetShift(this->GetShift());
  this->CurrentManipulator->SetControl(this->GetControl());
  this->CurrentManipulator->SetCenter(this->GetCenter());

  this->CurrentManipulator->OnButtonDown(x, y, ren, rwi);
}

//-----------------------------------------------------------------------------
void vtkPVTrackballMultiRotate::OnButtonUp(
  int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (this->CurrentManipulator)
  {
    this->CurrentManipulator->OnButtonUp(x, y, ren, rwi);
  }
}

//-----------------------------------------------------------------------------
void vtkPVTrackballMultiRotate::OnMouseMove(
  int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (this->CurrentManipulator)
  {
    this->CurrentManipulator->OnMouseMove(x, y, ren, rwi);
  }
}
