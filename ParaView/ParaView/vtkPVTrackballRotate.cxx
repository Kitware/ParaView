/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballRotate.cxx
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
#include "vtkPVTrackballRotate.h"

#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkPVTrackballRotate, "1.2");
vtkStandardNewMacro(vtkPVTrackballRotate);

//-------------------------------------------------------------------------
vtkPVTrackballRotate::vtkPVTrackballRotate()
{
}

//-------------------------------------------------------------------------
vtkPVTrackballRotate::~vtkPVTrackballRotate()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballRotate::OnButtonDown(int x, int y, vtkRenderer *ren,
                                        vtkRenderWindowInteractor* rwi)
{
  this->LastX = x;
  this->LastY = y;

  ren->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());

  this->ComputeDisplayCenter(ren);
}


//-------------------------------------------------------------------------
void vtkPVTrackballRotate::OnButtonUp(int x, int y, vtkRenderer *ren,
                                    vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;

  ren->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
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
  double v2[3];
  
  // translate to center
  transform->Identity();
  transform->Translate(this->Center[0], this->Center[1], this->Center[2]);
  
  float dx = this->LastX - x;
  float dy = this->LastY - y;
  
  // azimuth
  camera->OrthogonalizeViewUp();
  double *viewUp = camera->GetViewUp();
  int *size = ren->GetSize();
  transform->RotateWXYZ(360.0 * dx / size[0], viewUp[0], viewUp[1], viewUp[2]);
  
  // elevation
  vtkMath::Cross(camera->GetDirectionOfProjection(), viewUp, v2);
  transform->RotateWXYZ(-360.0 * dy / size[1], v2[0], v2[1], v2[2]);
  
  // translate back
  transform->Translate(-this->Center[0], -this->Center[1], -this->Center[2]);
  
  camera->ApplyTransform(transform);
  camera->OrthogonalizeViewUp();
  ren->ResetCameraClippingRange();
  
  rwi->Render();
  transform->Delete();

  this->LastX = x;
  this->LastY = y;
}

//-------------------------------------------------------------------------
void vtkPVTrackballRotate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center: " << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << endl;
}






