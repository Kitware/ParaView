/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkCameraInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include <stdlib.h>
#include <math.h>
#include "vtkCameraInteractor.h"
#include "vtkTransform.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkAxes.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"



//----------------------------------------------------------------------------
vtkCameraInteractor::vtkCameraInteractor()
{
  this->SaveX = this->SaveY = this->SaveY = 0.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->ZoomScale = 0.0;

  // display of the center of rotation
  this->CenterSource = vtkAxes::New();
  this->CenterMapper = vtkPolyDataMapper::New();
  this->CenterActor = vtkActor::New();
  this->CenterSource->SymmetricOn();
  this->CenterSource->ComputeNormalsOff();
  this->CenterMapper->SetInput(this->CenterSource->GetOutput());
  this->CenterActor->SetMapper(this->CenterMapper);
  //this->CenterActor->GetProperty()->SetAmbient(0.6);
  this->CenterActor->VisibilityOff();
}

//----------------------------------------------------------------------------
vtkCameraInteractor::~vtkCameraInteractor()
{
  this->CenterActor->Delete();
  this->CenterActor = NULL;

  this->CenterMapper->Delete();
  this->CenterMapper = NULL;

  this->CenterSource->Delete();
  this->CenterSource = NULL;
}


//----------------------------------------------------------------------------
void vtkCameraInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractor::PrintSelf(os,indent);
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1]
     << ", " << this->Center[2] << ")\n";
  os << indent << "Save: (" << this->SaveX << ", " << this->SaveY << ")\n";
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::ResetLights()
{
  vtkLightCollection *lights;
  vtkLight *light;
  vtkCamera *cam;

  if (this->Renderer == NULL) {return;}
  lights = this->Renderer->GetLights();
  lights->InitTraversal();
  light = lights->GetNextItem();
  if (light == NULL) 
    {
    return;
    }
  cam = this->Renderer->GetActiveCamera();
  light->SetPosition(cam->GetPosition());
  light->SetFocalPoint(cam->GetFocalPoint());
}


//----------------------------------------------------------------------------
void vtkCameraInteractor::PanZ(float dist)
{
  vtkCamera *cam;
  double *norm, *pos, *fp;

  cam = this->Renderer->GetActiveCamera();
  norm = cam->GetDirectionOfProjection();
  pos = cam->GetPosition();
  fp = cam->GetFocalPoint();

  cam->SetPosition(pos[0] + dist * norm[0], 
                   pos[1] + dist * norm[1],
                   pos[2] + dist * norm[2]);
  cam->SetFocalPoint(fp[0] - dist * norm[0], 
                     fp[1] + dist * norm[1],
                     fp[2] + dist * norm[2]);
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::ApplyTransformToCamera(vtkTransform *tform, vtkCamera *cam)
{
  double posOld[4], posNew[4], fpOld[4], fpNew[4], vuOld[4], vuNew[4];

  // apply transform the camera (the hard way).
  cam->GetPosition(posOld); posOld[3] = 1.0;
  cam->GetFocalPoint(fpOld); fpOld[3] = 1.0;
  cam->GetViewUp(vuOld); vuOld[3] = 1.0;
  // change view up into world coordinates
  vuOld[0] += posOld[0];
  vuOld[1] += posOld[1];
  vuOld[2] += posOld[2];

  // transform these three points
  tform->MultiplyPoint(posOld, posNew);
  tform->MultiplyPoint(fpOld, fpNew);
  tform->MultiplyPoint(vuOld, vuNew);

  // Change viewup back
  vuNew[0] -= posNew[0];
  vuNew[1] -= posNew[1];
  vuNew[2] -= posNew[2];
  
  cam->SetPosition(posNew);
  cam->SetFocalPoint(fpNew);
  cam->SetViewUp(vuNew);
}


//----------------------------------------------------------------------------
void vtkCameraInteractor::Fly(int x, int y, float velocity)
{
  vtkCamera *cam;
  int *size; 
  float fx, fy, px, py;
  double *pos;

  cam = this->Renderer->GetActiveCamera();
  pos = cam->GetPosition();
  size = this->Renderer->GetSize();
  fx = (size[0] * 0.5 - x) / size[0];
  fy = (size[1] * 0.5 - y) / size[1];
 
  // slow the velocity down durring tight turns.
  if (fx > 0) {px = fx;} else {px = -fx;}
  if (fy > 0) {py = fy;} else {py = -fy;}
  if (px < py) {px = py;}
  if (px > 0.5) {px = 0.5;}
  velocity = velocity * (1.0 - 2.0 * px);

  // move the camera
  this->ComputeCameraAxes();
  this->Transform->Identity();

  // the turning part
  // translate camera position to origin
  this->Transform->Translate(-pos[0], -pos[1], -pos[2]);
  this->Transform->RotateWXYZ(fx*3.0, this->CameraYAxis[0], 
                    this->CameraYAxis[1], this->CameraYAxis[2]);
  this->Transform->RotateWXYZ(fy*3.0, this->CameraXAxis[0],
                    this->CameraXAxis[1], this->CameraXAxis[2]);
  this->Transform->Translate(pos[0], pos[1], pos[2]);

  // the flying part (forward and backward) (z points back)
  this->Transform->Translate(-velocity * this->CameraZAxis[0], 
            -velocity * this->CameraZAxis[1], -velocity * this->CameraZAxis[2]);
  this->ApplyTransformToCamera(this->Transform, cam);
  //this->PanZ(velocity);
	//cam->Yaw(fx * 3.0);
	cam->OrthogonalizeViewUp();
	this->ResetLights();
  this->Renderer->ResetCameraClippingRange();
}


//----------------------------------------------------------------------------
void vtkCameraInteractor::SetCenter(double x, double y, double z)
{
  // maybe we should just consider center actor whith mtime check.
  if (this->Center[0] != x)
    {
    this->Center[0] = x;
    this->Modified();
    }
  if (this->Center[1] != y)
    {
    this->Center[1] = y;
    this->Modified();
    }
  if (this->Center[2] != z)
    {
    this->Center[2] = z;
    this->Modified();
    }
  this->CenterActor->SetPosition(x, y, z);

  this->ResetCenterActorSize();
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::ShowCenterOn()
{
  this->CenterActor->VisibilityOn();
  this->ResetCenterActorSize();
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::ShowCenterOff()
{
  this->CenterActor->VisibilityOff();
}

//----------------------------------------------------------------------------
// needed to add center actor.
void vtkCameraInteractor::SetRenderer(vtkRenderer *ren)
{
  if (this->Renderer == ren)
    {
    return;
    }
  if (this->Renderer != NULL)
    {
    this->Renderer->RemoveActor(this->CenterActor);
    this->Renderer->UnRegister(this);
    this->Renderer = NULL;
    }
  this->Renderer = ren;
  if (this->Renderer)
    {
    this->Renderer->Register(this);
    this->Renderer->AddActor(this->CenterActor);
    this->ResetCenterActorSize();
    }
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkCameraInteractor::ResetCenterActorSize()
{
  vtkActorCollection *actors;
  vtkActor *a;
  float bounds[6], *temp;
  int idx, firstFlag;

  if (this->Renderer == NULL)
    {
    return;
    }
  
  firstFlag = 1;
  // loop through all visible actors
  actors = this->Renderer->GetActors();
  actors->InitTraversal();
  while ((a = actors->GetNextItem()))
    {
    if (a != this->CenterActor && a->GetVisibility())
      {
      temp = a->GetBounds();
      for (idx = 0; idx < 3; ++idx)
        {
        if (firstFlag || temp[idx*2] < bounds[idx*2])
          {
          bounds[idx*2] = temp[idx*2];
          }
        if (firstFlag || temp[idx*2 + 1] > bounds[idx*2 + 1])
          {
          bounds[idx*2 + 1] = temp[idx*2 + 1];
          }
        }
        firstFlag = 0;
      }
    }

  // now use bounds to set the size fot the center cursor actor
  if ( ! firstFlag)
    {
    this->CenterActor->SetScale(bounds[1]-bounds[0], bounds[3]-bounds[2],
                                bounds[5]-bounds[4]);
    }
}

//============================================================================
// Rotate Interactor
// Rotates around arbitrary center of rotation.
//============================================================================

//----------------------------------------------------------------------------
void vtkCameraInteractor::RotateStart(int x1, int y1)
{
	this->SaveX = x1;
	this->SaveY = y1;
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::RotateMotion(int x2, int y2)
{
  double dx, dy;
  vtkCamera *cam;
  double *vu;
  double v2[3];
  int *size;
  
  cam = this->Renderer->GetActiveCamera();
  size = this->Renderer->GetSize();
  
  dx = this->SaveX - x2;
  // compensate for tk upper right origin
  dy = y2 - this->SaveY;
  
  this->SaveX = x2;
  this->SaveY = y2;
  
  // translate to center of rotation
  this->Transform->Identity();
  this->Transform->Translate(-this->Center[0], -this->Center[1], 
			     -this->Center[2]);
  // azimuth
  cam->OrthogonalizeViewUp();
  vu = cam->GetViewUp();	
  this->Transform->RotateWXYZ(360.0 * dx / size[0], vu[0], vu[1], vu[2]);
  // Elevation
  vtkMath::Cross(cam->GetDirectionOfProjection(), cam->GetViewUp(), v2);
  this->Transform->RotateWXYZ(-360.0 * dy / size[1], v2[0], v2[1], v2[2]);
  
  // translate back to world origin
  this->Transform->Translate(this->Center[0],this->Center[1],this->Center[2]);
  
  this->ApplyTransformToCamera(this->Transform, cam);
  this->ResetLights();
  this->Renderer->ResetCameraClippingRange();
}

//============================================================================
// Roll Interactor
// Rolls around arbitrary center of rotation.
//============================================================================

//----------------------------------------------------------------------------
void vtkCameraInteractor::RollStart(int x1, int y1)
{
  float *pt;

  if (this->Renderer == NULL)
    {
    return;
    }

  this->SaveX = x1;
  this->SaveY = y1;

  // save the center of rotation in screen coordinates
  this->Renderer->SetWorldPoint(this->Center[0], this->Center[1],
				this->Center[2], 1.0);
  this->Renderer->WorldToDisplay();

  pt = this->Renderer->GetDisplayPoint();
  this->DisplayCenter[0] = pt[0];
  this->DisplayCenter[1] = pt[1];
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::RollMotion(int x2, int y2)
{
  int *size;
  int x1, y1;
  vtkCamera *cam;
  double *pos, *fp, axis[3], zCross, angle;

  if (this->Renderer == NULL)
    {
    return;
    }

  // get the old point
  x1 = this->SaveX;
  y1 = this->SaveY;
  this->SaveX = x2;
  this->SaveY = y2;
  
  // compensate for tk origin in upper left
  size = this->Renderer->GetSize();
  y1 = size[1] - y1;
  y2 = size[1] - y2;

  // get needed variables
  cam = this->Renderer->GetActiveCamera();
  pos = cam->GetPosition();
  fp = cam->GetFocalPoint();
  
  // compute view vector (for rotation axis)
  axis[0] = fp[0] - pos[0];
  axis[1] = fp[1] - pos[1];
  axis[2] = fp[2] - pos[2];

  // compute the angle of rotation 
  // first compute the two vectors (center to mouse)
  x1 -= this->DisplayCenter[0];
  y1 -= this->DisplayCenter[1];
  x2 -= this->DisplayCenter[0];
  y2 -= this->DisplayCenter[1];

  // compute cross product (only z needs to be computed)
  zCross = x1 * y2 - y1 * x2;
  
  // divide by magnitudes to get angle
  angle = 57.2958 * zCross / (sqrt(x1*x1 + y1*y1) * sqrt(x2*x2 + y2*y2));

  // set the transform
  this->Transform->Identity();
  this->Transform->Translate(-this->Center[0], -this->Center[1], 
			     -this->Center[2]);
  // roll
  this->Transform->RotateWXYZ(angle, axis[0], axis[1], axis[2]);
  
  // translate back to world origin
  this->Transform->Translate(this->Center[0],this->Center[1],this->Center[2]);
  
  this->ApplyTransformToCamera(this->Transform, cam);
  cam->OrthogonalizeViewUp();
  this->ResetLights();
}


//============================================================================
// Hybrid Rotate/Roll.  Start new center -> Rotate.
// otherwise Roll.
//============================================================================

#define VTK_INTERACTOR_HYBRID_ROTATE 1
#define VTK_INTERACTOR_HYBRID_ROLL 2


//----------------------------------------------------------------------------
void vtkCameraInteractor::RotateRollStart(int x1, int y1)
{
  int *size;
  double px, py;
  float displayCenter[3];
  
  if (this->Renderer == NULL)
    {
    return;
    }

  // Are we rotating or rolling?
  size = this->Renderer->GetSize();

  // Get the center in Screen Coordinates
  this->Renderer->SetWorldPoint(this->Center[0], this->Center[1],
                                this->Center[2], 1.0);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(displayCenter);

  px = (double)(x1 - displayCenter[0]) / (double)(size[0]);
  // Flip y axis (origin from top left to bottom left).
  py = (double)(size[1] - y1 - displayCenter[1]) / (double)(size[1]);  

  if (fabs(px) < 0.2 && fabs(py) < 0.2)
    {
    this->HybridState = VTK_INTERACTOR_HYBRID_ROTATE;
    this->RotateStart(x1, y1);
    }
  else
    {
    this->HybridState = VTK_INTERACTOR_HYBRID_ROLL;
    this->RollStart(x1, y1);
    }
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::RotateRollMotion(int x2, int y2)
{
  if (this->HybridState == VTK_INTERACTOR_HYBRID_ROTATE)
    {
    this->RotateMotion(x2, y2);
    }
  if (this->HybridState == VTK_INTERACTOR_HYBRID_ROLL)
    {
    this->RollMotion(x2, y2);
    }
}

//============================================================================
// Pan
//============================================================================

//----------------------------------------------------------------------------
void vtkCameraInteractor::PanStart(int x1, int y1)
{
  this->SaveX = x1;
  this->SaveY = y1;
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::PanMotion(int x2, int y2)
{
  vtkCamera *cam;
  float viewAngle;
  int *size;
  
  cam	= this->Renderer->GetActiveCamera();
  viewAngle = cam->GetViewAngle();
  size = this->Renderer->GetSize();
  
  cam->Yaw(viewAngle * (x2 - this->SaveX) / size[0]);
  cam->Pitch(viewAngle * (y2 - this->SaveY) / size[1]);
  
  // Save current camera position for incremental computation (next translate)
  this->SaveX = x2;
  this->SaveY = y2;
  this->ResetLights();
}

//============================================================================
// Zoom
//============================================================================

//----------------------------------------------------------------------------
void vtkCameraInteractor::ZoomStart(int x1, int y1)
{
  double *range;
  int *size;
  vtkCamera *cam; 
  
  if (this->Renderer == NULL)
    {
    return;
    }
  size = this->Renderer->GetSize();
  cam	= this->Renderer->GetActiveCamera();
  range = cam->GetClippingRange();

  this->ZoomScale = 1.5 * range[1] / (float)size[1];
  this->SaveX = x1;
  this->SaveY = y1;
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::ZoomMotion(int x2, int y2)
{
  vtkCamera *cam;
  double pos[3];
  double fp[3];
  double *norm;
  double k, tmp;

  if (this->Renderer == NULL)
    {
    return;
    }
  cam	= this->Renderer->GetActiveCamera();
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);
  norm = cam->GetDirectionOfProjection();
  k = (double)(y2 - this->SaveY) * this->ZoomScale;

  tmp = k * norm[0];
  pos[0] += tmp;
  fp[0] += tmp;

  tmp = k * norm[1];
  pos[1] += tmp;
  fp[1] += tmp;

  tmp = k * norm[2];
  pos[2] += tmp;
  fp[2] += tmp;
  
  cam->SetFocalPoint(fp);
  cam->SetPosition(pos);

  // Save current camera position for incremental computation (next translate)
  this->SaveX = x2;
  this->SaveY = y2;
  this->ResetLights();
}


//============================================================================
// Hybrid Pan/Zoom.  Start in middle (y axis) third -> pan.
// Top or bottom third -> zoom
//============================================================================

#define VTK_INTERACTOR_HYBRID_PAN 1
#define VTK_INTERACTOR_HYBRID_ZOOM 2


//----------------------------------------------------------------------------
void vtkCameraInteractor::PanZoomStart(int x1, int y1)
{
  int *size;
  double pos;
  
  if (this->Renderer == NULL)
    {
    return;
    }
  size = this->Renderer->GetSize();

  // Are we panning or zooming.
  pos = (double)y1 / (double)(size[1]);
  if (pos < 0.333 || pos > 0.667)
    {
    this->HybridState = VTK_INTERACTOR_HYBRID_ZOOM;
    this->ZoomStart(x1, y1);
    }
  else
    {
    this->HybridState = VTK_INTERACTOR_HYBRID_PAN;
    this->PanStart(x1, y1);
    }
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::PanZoomMotion(int x2, int y2)
{
  if (this->HybridState == VTK_INTERACTOR_HYBRID_PAN)
    {
    this->PanMotion(x2, y2);
    }
  if (this->HybridState == VTK_INTERACTOR_HYBRID_ZOOM)
    {
    this->ZoomMotion(x2, y2);
    }
}



//============================================================================
// BoundingBoxZoom
// Interactor tries to zoom to the region defined by a rectangle.
//============================================================================

//----------------------------------------------------------------------------
void vtkCameraInteractor::BoundingBoxZoomStart(int x1, int y1)
{
  this->SaveX = x1;
  this->SaveY = y1;
}

//----------------------------------------------------------------------------
void vtkCameraInteractor::BoundingBoxZoomEnd(int x2, int y2)
{
  vtkCamera *cam;
  float viewAngle;
  int *size, x1, y1;
  float xm, ym, dx, dy, dist;
  double *range;
  
  cam	= this->Renderer->GetActiveCamera();
  viewAngle = cam->GetViewAngle();
  size = this->Renderer->GetSize();
  
  // compensate for tk upper left origin
  y1 = size[1] - this->SaveY;
  y2 = size[1] - y2;
  x1 = this->SaveX;
  
  // compute the middle of this box
  xm = 0.5 * (x1 + x2);
  ym = 0.5 * (y1 + y2);
  
  // first do the translation
  dx = xm - 0.5 * size[0];
  dy = ym - 0.5 * size[1];
  cam->Yaw(-viewAngle * dx / size[0]);
  cam->Pitch(viewAngle * dy / size[1]);
  
  // Now we need to zoom
  // compute fractions of screen
  if (x1 < x2) 
    {
    dx = x2 - x1;
    } 
  else 
    {
    dx = x1 - x2;
    }
  if (y1 < y2) 
    {
    dy = y2 - y1;
    } 
  else 
    {
    dy = y1 - y2;
    }
  // special case: Zoom box too small (click without drag)
  if (dx < 5 && dy < 5) 
    {
    dx = 0.3;
    } 
  else 
    {
    dx = dx / size[0];
    dy = dy / size[1];
    if (dy > dx) {dx = dy;}
    }	
  // we need a distance to compute zoom factor.
  // assume the ClippingRange is being reset each render.
  range = cam->GetClippingRange();
  dist = 0.5 * (range[0] + range[1]);
  // do we need to subtract pos->fp distance ?
  dist = -dist  * (1.0 - dx);
  // move camera forward
  this->PanZ(dist);
  
  this->ResetLights();
}


