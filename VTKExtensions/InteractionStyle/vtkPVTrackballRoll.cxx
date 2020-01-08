/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballRoll.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballRoll.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPVTrackballRoll);

//-------------------------------------------------------------------------
vtkPVTrackballRoll::vtkPVTrackballRoll()
{
}

//-------------------------------------------------------------------------
vtkPVTrackballRoll::~vtkPVTrackballRoll()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballRoll::OnButtonDown(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballRoll::OnButtonUp(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballRoll::OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (ren == NULL)
  {
    return;
  }

  vtkCamera* camera = ren->GetActiveCamera();
  double axis[3];

  // compute view vector (rotation axis)
  double* pos = camera->GetPosition();
  double* fp = camera->GetFocalPoint();

  axis[0] = fp[0] - pos[0];
  axis[1] = fp[1] - pos[1];
  axis[2] = fp[2] - pos[2];

  // compute the angle of rotation
  // - first compute the two vectors (center to mouse)
  this->ComputeDisplayCenter(ren);

  int x1, x2, y1, y2;
  x1 = rwi->GetLastEventPosition()[0] - (int)this->DisplayCenter[0];
  x2 = x - (int)this->DisplayCenter[0];
  y1 = rwi->GetLastEventPosition()[1] - (int)this->DisplayCenter[1];
  y2 = y - (int)this->DisplayCenter[1];
  if ((x2 == 0.0 && y2 == 0.0) || (x1 == 0.0 && y1 == 0.0))
  {
    // don't ever want to divide by zero
    return;
  }

  // - divide by magnitudes to get angle
  double angle = vtkMath::DegreesFromRadians((x1 * y2 - y1 * x2) /
    (sqrt(static_cast<double>(x1 * x1 + y1 * y1)) * sqrt(static_cast<double>(x2 * x2 + y2 * y2))));

  // translate to center
  vtkTransform* transform = vtkTransform::New();
  transform->Identity();
  transform->Translate(this->Center[0], this->Center[1], this->Center[2]);

  // roll
  transform->RotateWXYZ(angle, axis[0], axis[1], axis[2]);

  // translate back
  transform->Translate(-this->Center[0], -this->Center[1], -this->Center[2]);

  camera->ApplyTransform(transform);
  camera->OrthogonalizeViewUp();

  rwi->Render();
  transform->Delete();
}

//-------------------------------------------------------------------------
void vtkPVTrackballRoll::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
