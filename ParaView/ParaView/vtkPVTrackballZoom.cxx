/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoom.cxx
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
#include "vtkPVTrackballZoom.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

vtkCxxRevisionMacro(vtkPVTrackballZoom, "1.1");
vtkStandardNewMacro(vtkPVTrackballZoom);

//-------------------------------------------------------------------------
vtkPVTrackballZoom::vtkPVTrackballZoom()
{
  this->ZoomScale = 0.0;
}

//-------------------------------------------------------------------------
vtkPVTrackballZoom::~vtkPVTrackballZoom()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoom::OnButtonDown(int x, int y, vtkRenderer *ren,
                                      vtkRenderWindowInteractor *rwi)
{
  int *size = ren->GetSize();
  vtkCamera *camera = ren->GetActiveCamera();

  if (camera->GetParallelProjection())
    {
    this->ZoomScale = 1.5 / (float)size[1];
    }
  else
    {
    double *range = camera->GetClippingRange();
    this->ZoomScale = 1.5 * range[1] / (float)size[1];
    }
  this->LastX = x;
  this->LastY = y;

  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetDesiredUpdateRate());
}


//-------------------------------------------------------------------------
void vtkPVTrackballZoom::OnButtonUp(int x, int y, vtkRenderer *,
                                    vtkRenderWindowInteractor *rwi)
{
  this->LastX = x;
  this->LastY = y;

  rwi->GetRenderWindow()->SetDesiredUpdateRate(rwi->GetStillUpdateRate());
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoom::OnMouseMove(int x, int y, vtkRenderer *ren,
                                     vtkRenderWindowInteractor *rwi)
{
  double dy = this->LastY - y;
  vtkCamera *camera = ren->GetActiveCamera();
  double pos[3], fp[3], *norm, k, tmp;
  
  if (camera->GetParallelProjection())
    {
    k = dy * this->ZoomScale;
    camera->SetParallelScale((1.0 - k) * camera->GetParallelScale());
    }
  else
    { 
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
    ren->ResetCameraClippingRange();
    }

  rwi->Render();
  this->LastX = x;
  this->LastY = y;
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoom::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "ZoomScale: {" << this->ZoomScale << endl;
}






