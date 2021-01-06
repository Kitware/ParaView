/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballSkyboxRotate.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballSkyboxRotate.h"

#include "vtkMath.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSkybox.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPVTrackballSkyboxRotate);

//-------------------------------------------------------------------------
void vtkPVTrackballSkyboxRotate::EnvironmentRotate(vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
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
void vtkPVTrackballSkyboxRotate::OnMouseMove(
  int vtkNotUsed(x), int vtkNotUsed(y), vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (ren == nullptr || this->Skybox == nullptr)
  {
    return;
  }

  // Update environment orientation
  this->EnvironmentRotate(ren, rwi);

  // Update skybox orientation
  double* up = ren->GetEnvironmentUp();
  double* right = ren->GetEnvironmentRight();

  double front[3];
  vtkMath::Cross(right, up, front);

  this->Skybox->SetFloorPlane(up[0], up[1], up[2], 0.0);
  this->Skybox->SetFloorRight(front[0], front[1], front[2]);

  rwi->Render();
}
