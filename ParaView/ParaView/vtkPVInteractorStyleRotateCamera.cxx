/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleRotateCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVInteractorStyleRotateCamera.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkTransform.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkPVInteractorStyleRotateCamera, "1.1");
vtkStandardNewMacro(vtkPVInteractorStyleRotateCamera);

//-------------------------------------------------------------------------
vtkPVInteractorStyleRotateCamera::vtkPVInteractorStyleRotateCamera()
{
  this->UseTimers = 0;
  this->ZoomScale = 0.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

//-------------------------------------------------------------------------
vtkPVInteractorStyleRotateCamera::~vtkPVInteractorStyleRotateCamera()
{
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::OnMouseMove()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  
  switch (this->State)
    {
    case VTKIS_ROTATE:
      this->Rotate();
      break;
    case VTKIS_SPIN:
      this->Roll();
      break;
    case VTKIS_PAN:
      this->Pan();
      break;
    case VTKIS_ZOOM:
      this->Zoom();
      break;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::OnLeftButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  if (this->Interactor->GetShiftKey())
    {
    // save the center of rotation in screen coordinates
    this->CurrentRenderer->SetWorldPoint(this->Center[0],
                                         this->Center[1],
                                         this->Center[2], 1.0);
    this->CurrentRenderer->WorldToDisplay();
    float *pt = this->CurrentRenderer->GetDisplayPoint();
    this->DisplayCenter[0] = pt[0];
    this->DisplayCenter[1] = pt[1];
    
    this->StartSpin();
    }
  else
    {
    this->StartRotate();
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::OnLeftButtonUp()
{
  switch (this->State)
    {
    case VTKIS_SPIN:
      this->EndSpin();
      break;
    case VTKIS_ROTATE:
      this->EndRotate();
      break;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::OnRightButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  if (this->Interactor->GetShiftKey())
    {
    int *size = this->CurrentRenderer->GetSize();
    double *range = this->CurrentRenderer->GetActiveCamera()->GetClippingRange();
    this->ZoomScale = 1.5 * range[1] / (float)size[1];
    
    this->StartZoom();
    }
  else
    {
    this->StartPan();
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::OnRightButtonUp()
{
  switch (this->State)
    {
    case VTKIS_ZOOM:
      this->EndZoom();
      break;
    case VTKIS_PAN:
      this->EndPan();
      break;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::Rotate()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkTransform *transform = vtkTransform::New();
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  double v2[3];
  
  // translate to center
  transform->Identity();
  transform->Translate(this->Center[0], this->Center[1], this->Center[2]);
  
  float dx = rwi->GetLastEventPosition()[0] - rwi->GetEventPosition()[0];
  float dy = rwi->GetLastEventPosition()[1] - rwi->GetEventPosition()[1];
  
  // azimuth
  camera->OrthogonalizeViewUp();
  double *viewUp = camera->GetViewUp();
  int *size = this->CurrentRenderer->GetSize();
  transform->RotateWXYZ(360.0 * dx / size[0], viewUp[0], viewUp[1], viewUp[2]);
  
  // elevation
  vtkMath::Cross(camera->GetDirectionOfProjection(), viewUp, v2);
  transform->RotateWXYZ(-360.0 * dy / size[1], v2[0], v2[1], v2[2]);
  
  // translate back
  transform->Translate(-this->Center[0], -this->Center[1], -this->Center[2]);
  
  this->TransformCamera(transform, camera);
  camera->OrthogonalizeViewUp();
  this->ResetLights();
  this->CurrentRenderer->ResetCameraClippingRange();
  
  rwi->Render();
  transform->Delete();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::Roll()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  vtkTransform *transform = vtkTransform::New();
  double axis[3];
  vtkRenderWindowInteractor *rwi = this->Interactor;
  
  // compute view vector (rotation axis)
  double *pos = camera->GetPosition();
  double *fp = camera->GetFocalPoint();
  
  axis[0] = fp[0] - pos[0];
  axis[1] = fp[1] - pos[1];
  axis[2] = fp[2] - pos[2];
  
  // compute the angle of rotation
  // - first compute the two vectors (center to mouse)
  int x1, x2, y1, y2;
  x1 = rwi->GetLastEventPosition()[0] - (int)this->DisplayCenter[0];
  x2 = rwi->GetEventPosition()[0] - (int)this->DisplayCenter[0];
  y1 = rwi->GetLastEventPosition()[1] - (int)this->DisplayCenter[1];
  y2 = rwi->GetEventPosition()[1] - (int)this->DisplayCenter[1];
  
  // - compute cross product (only need z component)
  double zCross = x1*y2 - y1*x2;
  
  // - divide by madnitudes to get angle
  double angle = vtkMath::RadiansToDegrees() * zCross /
    (sqrt(static_cast<float>(x1*x1 + y1*y1)) *
     sqrt(static_cast<float>(x2*x2 + y2*y2)));
  
  // translate to center
  transform->Identity();
  transform->Translate(this->Center[0], this->Center[1], this->Center[2]);
  
  // roll
  transform->RotateWXYZ(angle, axis[0], axis[1], axis[2]);
  
  // translate back
  transform->Translate(-this->Center[0], -this->Center[1], -this->Center[2]);
  
  this->TransformCamera(transform, camera);
  camera->OrthogonalizeViewUp();
  this->ResetLights();
  this->CurrentRenderer->ResetCameraClippingRange();
  
  rwi->Render();
  transform->Delete();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  float viewAngle = camera->GetViewAngle();
  int *size = this->CurrentRenderer->GetSize();
  
  // These are different because y is flipped.
  float dx = rwi->GetEventPosition()[0] - rwi->GetLastEventPosition()[0];
  float dy = rwi->GetLastEventPosition()[1] - rwi->GetEventPosition()[1];
  
  camera->Yaw(viewAngle * dx / size[0]);
  camera->Pitch(viewAngle * dy / size[1]);
  
  this->ResetLights();
  this->CurrentRenderer->ResetCameraClippingRange();
  
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::Zoom()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  double dy = rwi->GetLastEventPosition()[1] - rwi->GetEventPosition()[1];
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  double pos[3], fp[3], *norm, k, tmp;
  
  camera->GetPosition(pos);
  camera->GetFocalPoint(fp);
  norm = camera->GetDirectionOfProjection();
  k = dy * this->ZoomScale;

  tmp = k * norm[0];
  pos[0] += tmp;
  fp[0] += tmp;
  
  tmp = k*norm[1];
  pos[1] += tmp;
  fp[1] += tmp;
  
  tmp = k * norm[2];
  pos[2] += tmp;
  fp[2] += tmp;
  
  camera->SetFocalPoint(fp);
  camera->SetPosition(pos);
  
  this->ResetLights();
  this->CurrentRenderer->ResetCameraClippingRange();
  
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::ResetLights()
{
  if ( ! this->CurrentRenderer)
    {
    return;
    }
  
  vtkLight *light;
  
  vtkLightCollection *lights = this->CurrentRenderer->GetLights();
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  
  lights->InitTraversal();
  light = lights->GetNextItem();
  if ( ! light)
    {
    return;
    }
  light->SetPosition(camera->GetPosition());
  light->SetFocalPoint(camera->GetFocalPoint());
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::TransformCamera(vtkTransform *transform,
                                                       vtkCamera *camera)
{
  double posOld[4], posNew[4], fpOld[4], fpNew[4], vuOld[4], vuNew[4];

  // apply transform the camera (the hard way).
  camera->GetPosition(posOld); posOld[3] = 1.0;
  camera->GetFocalPoint(fpOld); fpOld[3] = 1.0;
  camera->GetViewUp(vuOld); vuOld[3] = 1.0;
  // change view up into world coordinates
  vuOld[0] += posOld[0];
  vuOld[1] += posOld[1];
  vuOld[2] += posOld[2];

  // transform these three points
  transform->MultiplyPoint(posOld, posNew);
  transform->MultiplyPoint(fpOld, fpNew);
  transform->MultiplyPoint(vuOld, vuNew);

  // Change viewup back
  vuNew[0] -= posNew[0];
  vuNew[1] -= posNew[1];
  vuNew[2] -= posNew[2];
  
  camera->SetPosition(posNew);
  camera->SetFocalPoint(fpNew);
  camera->SetViewUp(vuNew);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleRotateCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Center: {" << this->Center[0] << ", "
     << this->Center[1] << ", " << this->Center[2] << ")\n";
}
