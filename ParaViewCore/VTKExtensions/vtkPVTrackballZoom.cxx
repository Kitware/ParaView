/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoom.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballZoom.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

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
void vtkPVTrackballZoom::OnButtonDown(int, int, vtkRenderer *ren,
                                      vtkRenderWindowInteractor *)
{
  int *size = ren->GetSize();
  vtkCamera *camera = ren->GetActiveCamera();

  if (camera->GetParallelProjection())
    {
    this->ZoomScale = 1.5 / (double)size[1];
    }
  else
    {
    double *range = camera->GetClippingRange();
    this->ZoomScale = 1.5 * range[1] / (double)size[1];
    }
}


//-------------------------------------------------------------------------
void vtkPVTrackballZoom::OnButtonUp(int, int, vtkRenderer *,
                                    vtkRenderWindowInteractor *)
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoom::OnMouseMove(int vtkNotUsed(x), int y,
                                     vtkRenderer *ren,
                                     vtkRenderWindowInteractor *rwi)
{
  double dy = rwi->GetLastEventPosition()[1] - y;
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
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoom::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "ZoomScale: {" << this->ZoomScale << endl;
}






