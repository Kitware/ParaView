/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballPan.cxx
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
#include "vtkPVTrackballPan.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkPVTrackballPan, "1.1");
vtkStandardNewMacro(vtkPVTrackballPan);

//-------------------------------------------------------------------------
vtkPVTrackballPan::vtkPVTrackballPan()
{
}

//-------------------------------------------------------------------------
vtkPVTrackballPan::~vtkPVTrackballPan()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnButtonDown(int x, int y, vtkRenderer *,
                                     vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;
  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
}


//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnButtonUp(int x, int y, vtkRenderer *,
                                    vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;

  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnMouseMove(int x, int y, vtkRenderer *ren,
                                     vtkRenderWindowInteractor *rwi)
{
  if (ren == NULL)
    {
    return;
    }

  // These are different because y is flipped.
  int *size = ren->GetSize();
  float dx = (float)(x - this->LastX) / (float)(size[1]);
  float dy = (float)(this->LastY - y) / (float)(size[1]);

  vtkCamera *camera = ren->GetActiveCamera();
  if (camera->GetParallelProjection())
    {
    camera->OrthogonalizeViewUp();
    double *up = camera->GetViewUp();
    double *vpn = camera->GetViewPlaneNormal();
    double right[3];
    double pos[3];
    double fp[3];
    double scale, tmp;
    camera->GetViewUp(up);
    camera->GetViewPlaneNormal(vpn);
    vtkMath::Cross(vpn, up, right);
    camera->GetPosition(pos);
    camera->GetFocalPoint(fp);

    scale = camera->GetParallelScale();
    dx *= scale * 2.0;
    dy *= scale * 2.0;

    tmp = (right[0]*dx + up[0]*dy);
    pos[0] += tmp;
    fp[0] += tmp; 
    tmp = (right[1]*dx + up[1]*dy); 
    pos[1] += tmp;
    fp[1] += tmp; 
    tmp = (right[2]*dx + up[2]*dy); 
    pos[2] += tmp;
    fp[2] += tmp; 
    camera->SetPosition(pos);
    camera->SetFocalPoint(fp);
    }
  else
    {
    float viewAngle = camera->GetViewAngle();  
    camera->Yaw(viewAngle * dx);
    camera->Pitch(viewAngle * dy);
    }
  ren->ResetCameraClippingRange();
  rwi->Render();

  this->LastX = x;
  this->LastY = y;
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






