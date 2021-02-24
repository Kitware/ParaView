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
#include "vtkCameraManipulatorGUIHelper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVTrackballPan);

//-------------------------------------------------------------------------
vtkPVTrackballPan::vtkPVTrackballPan() = default;

//-------------------------------------------------------------------------
vtkPVTrackballPan::~vtkPVTrackballPan() = default;

//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnButtonDown(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnButtonUp(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (ren == nullptr || !this->GetGUIHelper())
  {
    return;
  }

  vtkCamera* camera = ren->GetActiveCamera();
  double pos[3], fp[3];
  camera->GetPosition(pos);
  camera->GetFocalPoint(fp);

  if (camera->GetParallelProjection())
  {
    camera->OrthogonalizeViewUp();
    double* up = camera->GetViewUp();
    double* vpn = camera->GetViewPlaneNormal();
    double right[3];
    double scale, tmp;
    camera->GetViewUp(up);
    camera->GetViewPlaneNormal(vpn);
    vtkMath::Cross(vpn, up, right);

    // These are different because y is flipped.
    int* size = ren->GetSize();
    double dx = (double)(x - rwi->GetLastEventPosition()[0]) / (double)(size[1]);
    double dy = (double)(rwi->GetLastEventPosition()[1] - y) / (double)(size[1]);

    scale = camera->GetParallelScale();
    dx *= scale * 2.0;
    dy *= scale * 2.0;

    tmp = (right[0] * dx + up[0] * dy);
    pos[0] += tmp;
    fp[0] += tmp;
    tmp = (right[1] * dx + up[1] * dy);
    pos[1] += tmp;
    fp[1] += tmp;
    tmp = (right[2] * dx + up[2] * dy);
    pos[2] += tmp;
    fp[2] += tmp;
    camera->SetPosition(pos);
    camera->SetFocalPoint(fp);
  }
  else
  {
    double depth, worldPt[4], lastWorldPt[4];
    double bounds[6], center[3];
    if (this->GetGUIHelper()->GetActiveSourceBounds(bounds))
    {
      int idx;
      for (idx = 0; idx < 3; idx++)
      {
        center[idx] = (bounds[idx * 2] + bounds[idx * 2 + 1]) / 2.0;
      }
      ren->SetWorldPoint(center[0], center[1], center[2], 1.0);
    }
    else
    {
      if (this->GetGUIHelper()->GetCenterOfRotation(center))
      {
        ren->SetWorldPoint(center[0], center[1], center[2], 1.0);
      }
    }

    ren->WorldToDisplay();
    depth = ren->GetDisplayPoint()[2];

    ren->SetDisplayPoint(x, y, depth);
    ren->DisplayToWorld();
    ren->GetWorldPoint(worldPt);
    if (worldPt[3])
    {
      worldPt[0] /= worldPt[3];
      worldPt[1] /= worldPt[3];
      worldPt[2] /= worldPt[3];
      worldPt[3] = 1.0;
    }

    ren->SetDisplayPoint(rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1], depth);
    ren->DisplayToWorld();
    ren->GetWorldPoint(lastWorldPt);
    if (lastWorldPt[3])
    {
      lastWorldPt[0] /= lastWorldPt[3];
      lastWorldPt[1] /= lastWorldPt[3];
      lastWorldPt[2] /= lastWorldPt[3];
      lastWorldPt[3] = 1.0;
    }

    pos[0] += lastWorldPt[0] - worldPt[0];
    pos[1] += lastWorldPt[1] - worldPt[1];
    pos[2] += lastWorldPt[2] - worldPt[2];

    fp[0] += lastWorldPt[0] - worldPt[0];
    fp[1] += lastWorldPt[1] - worldPt[1];
    fp[2] += lastWorldPt[2] - worldPt[2];

    camera->SetPosition(pos);
    camera->SetFocalPoint(fp);
  }
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballPan::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
