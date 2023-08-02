// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVTrackballEnvironmentRotate.h"

#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPVTrackballEnvironmentRotate);

//-------------------------------------------------------------------------
void vtkPVTrackballEnvironmentRotate::EnvironmentRotate(
  vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  int dx = rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0];
  int sizeX = ren->GetRenderWindow()->GetSize()[0];

  vtkNew<vtkMatrix3x3> mat;

  double* up = ren->GetEnvironmentUp();
  double* right = ren->GetEnvironmentRight();

  double front[3];
  vtkMath::Cross(right, up, front);
  for (int i = 0; i < 3; i++)
  {
    mat->SetElement(i, 0, right[i]);
    mat->SetElement(i, 1, up[i]);
    mat->SetElement(i, 2, front[i]);
  }

  double angle = (dx / static_cast<double>(sizeX)) * this->RotationFactor;

  double c = std::cos(angle);
  double s = std::sin(angle);
  double t = 1.0 - c;

  vtkNew<vtkMatrix3x3> rot;

  rot->SetElement(0, 0, t * up[0] * up[0] + c);
  rot->SetElement(0, 1, t * up[0] * up[1] - up[2] * s);
  rot->SetElement(0, 2, t * up[0] * up[2] + up[1] * s);

  rot->SetElement(1, 0, t * up[0] * up[1] + up[2] * s);
  rot->SetElement(1, 1, t * up[1] * up[1] + c);
  rot->SetElement(1, 2, t * up[1] * up[2] - up[0] * s);

  rot->SetElement(2, 0, t * up[0] * up[2] - up[1] * s);
  rot->SetElement(2, 1, t * up[1] * up[2] + up[0] * s);
  rot->SetElement(2, 2, t * up[2] * up[2] + c);

  vtkMatrix3x3::Multiply3x3(rot, mat, mat);

  // update environment orientation
  ren->SetEnvironmentRight(mat->GetElement(0, 0), mat->GetElement(1, 0), mat->GetElement(2, 0));
}

//-------------------------------------------------------------------------
void vtkPVTrackballEnvironmentRotate::OnMouseMove(
  int vtkNotUsed(x), int vtkNotUsed(y), vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (ren == nullptr)
  {
    return;
  }

  // Update environment orientation
  this->EnvironmentRotate(ren, rwi);

  rwi->Render();
}
