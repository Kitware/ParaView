/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleTranslateCamera.cxx
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
#include "vtkPVInteractorStyleTranslateCamera.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkLight.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkPVInteractorStyleTranslateCamera, "1.2");
vtkStandardNewMacro(vtkPVInteractorStyleTranslateCamera);

//-------------------------------------------------------------------------
vtkPVInteractorStyleTranslateCamera::vtkPVInteractorStyleTranslateCamera()
{
  this->UseTimers = 0;
  this->ZoomScale = 0.0;
}

//-------------------------------------------------------------------------
vtkPVInteractorStyleTranslateCamera::~vtkPVInteractorStyleTranslateCamera()
{
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleTranslateCamera::OnMouseMove()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  
  switch (this->State)
    {
    case VTKIS_PAN:
      this->Pan();
      break;
    case VTKIS_ZOOM:
      this->Zoom();
      break;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleTranslateCamera::OnLeftButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartPan();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleTranslateCamera::OnLeftButtonUp()
{
  this->EndPan();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleTranslateCamera::OnRightButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  int *size = this->CurrentRenderer->GetSize();
  double *range = this->CurrentRenderer->GetActiveCamera()->GetClippingRange();
  this->ZoomScale = 1.5 * range[1] / (float)size[1];
  
  this->StartZoom();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleTranslateCamera::OnRightButtonUp()
{
  this->EndZoom();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleTranslateCamera::Pan()
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
void vtkPVInteractorStyleTranslateCamera::Zoom()
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
void vtkPVInteractorStyleTranslateCamera::ResetLights()
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
void vtkPVInteractorStyleTranslateCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
