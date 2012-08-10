/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballRotate.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballRotate.h"

#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPVTrackballRotate);

//-------------------------------------------------------------------------
vtkPVTrackballRotate::vtkPVTrackballRotate()
{
  this->Center[0] = 0;
  this->Center[1] = 0;
  this->Center[2] = 0;
  this->DisplayCenter[0] = 0;
  this->DisplayCenter[1] = 0;
}

//-------------------------------------------------------------------------
vtkPVTrackballRotate::~vtkPVTrackballRotate()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballRotate::OnButtonDown(int, int, vtkRenderer *ren,
                                        vtkRenderWindowInteractor*)
{
  this->ComputeDisplayCenter(ren);
}


//-------------------------------------------------------------------------
void vtkPVTrackballRotate::OnButtonUp(int, int, vtkRenderer *,
                                    vtkRenderWindowInteractor *)
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballRotate::OnMouseMove(int x, int y, vtkRenderer *ren,
                                     vtkRenderWindowInteractor *rwi)
{
  if (ren == NULL)
    {
    return;
    }
  
  vtkTransform *transform = vtkTransform::New();
  vtkCamera *camera = ren->GetActiveCamera();

  double scale = vtkMath::Norm(camera->GetPosition());
  if (scale <= 0.0)
    {
    scale = vtkMath::Norm(camera->GetFocalPoint());
    if (scale <= 0.0)
      {
      scale = 1.0;
      }
    }
  double* temp = camera->GetFocalPoint();
  camera->SetFocalPoint(temp[0]/scale, temp[1]/scale, temp[2]/scale);
  temp = camera->GetPosition();
  camera->SetPosition(temp[0]/scale, temp[1]/scale, temp[2]/scale);

  double v2[3];
  // translate to center
  transform->Identity();
  transform->Translate(this->Center[0]/scale, this->Center[1]/scale, this->Center[2]/scale);
  
  int dx = rwi->GetLastEventPosition()[0] - x;
  int dy = rwi->GetLastEventPosition()[1] - y;
  
  // azimuth
  camera->OrthogonalizeViewUp();
  double *viewUp = camera->GetViewUp();
  int *size = ren->GetSize();
  transform->RotateWXYZ(360.0 * dx / size[0], viewUp[0], viewUp[1], viewUp[2]);
  
  // elevation
  vtkMath::Cross(camera->GetDirectionOfProjection(), viewUp, v2);
  transform->RotateWXYZ(-360.0 * dy / size[1], v2[0], v2[1], v2[2]);
  
  // translate back
  transform->Translate(-this->Center[0]/scale, -this->Center[1]/scale, -this->Center[2]/scale);
  
  camera->ApplyTransform(transform);
  camera->OrthogonalizeViewUp();

  // For rescale back.
  temp = camera->GetFocalPoint();
  camera->SetFocalPoint(temp[0]*scale, temp[1]*scale, temp[2]*scale);
  temp = camera->GetPosition();
  camera->SetPosition(temp[0]*scale, temp[1]*scale, temp[2]*scale);

  ren->ResetCameraClippingRange();
  
  rwi->Render();
  transform->Delete();
}

//-------------------------------------------------------------------------
void vtkPVTrackballRotate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: " << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << endl;
}






