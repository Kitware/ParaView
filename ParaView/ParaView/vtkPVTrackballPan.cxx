/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballPan.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballPan.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkPVTrackballPan, "1.5");
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
                                     vtkRenderWindowInteractor *)
{
}


//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnButtonUp(int x, int y, vtkRenderer *,
                                    vtkRenderWindowInteractor *)
{
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
  float dx = (float)(x - rwi->GetLastEventPosition()[0]) / (float)(size[1]);
  float dy = (float)(rwi->GetLastEventPosition()[1] - y) / (float)(size[1]);

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
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






