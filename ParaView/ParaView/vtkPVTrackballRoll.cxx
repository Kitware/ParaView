/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballRoll.cxx
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
#include "vtkPVTrackballRoll.h"

#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkPVTrackballRoll, "1.1");
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
void vtkPVTrackballRoll::OnButtonDown(int x, int y, vtkRenderer *ren,
                                      vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;

  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  this->ComputeDisplayCenter(ren);
}


//-------------------------------------------------------------------------
void vtkPVTrackballRoll::OnButtonUp(int x, int y, vtkRenderer *,
                                    vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;

  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballRoll::OnMouseMove(int x, int y, vtkRenderer *ren,
                                     vtkRenderWindowInteractor *rwi)
{
  if (ren == NULL)
    {
    return;
    }

  vtkCamera *camera = ren->GetActiveCamera();
  vtkTransform *transform = vtkTransform::New();
  double axis[3];
  
  // compute view vector (rotation axis)
  double *pos = camera->GetPosition();
  double *fp = camera->GetFocalPoint();
  
  axis[0] = fp[0] - pos[0];
  axis[1] = fp[1] - pos[1];
  axis[2] = fp[2] - pos[2];
  
  // compute the angle of rotation
  // - first compute the two vectors (center to mouse)
  int x1, x2, y1, y2;
  x1 = this->LastX - (int)this->DisplayCenter[0];
  x2 = x - (int)this->DisplayCenter[0];
  y1 = this->LastY - (int)this->DisplayCenter[1];
  y2 = y - (int)this->DisplayCenter[1];
  
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
  
  camera->ApplyTransform(transform);
  camera->OrthogonalizeViewUp();
  ren->ResetCameraClippingRange();
  
  rwi->Render();
  transform->Delete();

  this->LastX = x;
  this->LastY = y;
}

//-------------------------------------------------------------------------
void vtkPVTrackballRoll::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






