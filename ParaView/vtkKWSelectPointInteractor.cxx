/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSelectPointInteractor.cxx
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
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWSelectPointInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVProbe.h"
#include "vtkPVWindow.h"
#include "vtkTclUtil.h"
#include "vtkSphereSource.h"

int vtkKWSelectPointInteractorCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWSelectPointInteractor::vtkKWSelectPointInteractor()
{
  this->CommandFunction = vtkKWSelectPointInteractorCommand;
  
  this->SelectedPoint[0] = 0.0;
  this->SelectedPoint[1] = 0.0;
  this->SelectedPoint[2] = 0.0;
  
  this->Cursor = vtkCursor3D::New();
  this->CursorMapper = vtkPolyDataMapper::New();
  this->CursorMapper->SetInput(this->Cursor->GetOutput());
  this->CursorActor = vtkActor::New();
  this->CursorActor->SetMapper(this->CursorMapper);
  
  this->XSphere1Actor = vtkActor::New();
  this->XSphere2Actor = vtkActor::New();
  this->YSphere1Actor = vtkActor::New();
  this->YSphere2Actor = vtkActor::New();
  this->ZSphere1Actor = vtkActor::New();
  this->ZSphere2Actor = vtkActor::New();
  
  this->CurrentSphereId = -1;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -1;
  
  this->PVProbe = NULL;
}

vtkKWSelectPointInteractor *vtkKWSelectPointInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret =
    vtkObjectFactory::CreateInstance("vtkKWSelectPointInteractor");
  if (ret)
    {
    return (vtkKWSelectPointInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWSelectPointInteractor;
}
  
vtkKWSelectPointInteractor::~vtkKWSelectPointInteractor()
{
  this->Cursor->Delete();
  this->Cursor = NULL;
  
  this->CursorMapper->Delete();
  this->CursorMapper = NULL;
  
  this->CursorActor->Delete();
  this->CursorActor = NULL;
  
  this->XSphere1Actor->Delete();
  this->XSphere1Actor = NULL;
  
  this->XSphere2Actor->Delete();
  this->XSphere2Actor = NULL;
  
  this->YSphere1Actor->Delete();
  this->YSphere1Actor = NULL;
  
  this->YSphere2Actor->Delete();
  this->YSphere2Actor = NULL;
  
  this->ZSphere1Actor->Delete();
  this->ZSphere1Actor = NULL;
  
  this->ZSphere2Actor->Delete();
  this->ZSphere2Actor = NULL;
}

void vtkKWSelectPointInteractor::SetBounds(float bounds[6])
{
  float focalPoint[3], position[3];
  vtkSphereSource *sphere;
  vtkPolyDataMapper *sphereMapper;
  vtkRenderer *renderer;
  int i;
  float scale, tempScale;
  
  for (i = 0; i < 6; i++)
    {
    this->Bounds[i] = bounds[i];
    if (i % 2 == 1)
      {
      this->SelectedPoint[i/2] = bounds[i-1] + (bounds[i] - bounds[i-1])/2;
      focalPoint[i/2] = this->SelectedPoint[i/2];
      }
    }
  
  sphere = vtkSphereSource::New();
  sphere->SetRadius(1);
  sphereMapper = vtkPolyDataMapper::New();
  sphereMapper->SetInput(sphere->GetOutput());
  renderer = this->RenderView->GetRenderer();
  
  // Set up the 3D cursor.
  this->Cursor->SetModelBounds(this->Bounds);
  this->Cursor->SetFocalPoint(focalPoint);
  renderer->AddActor(this->CursorActor);
  
  scale = this->Bounds[1] - this->Bounds[0];
  if ((tempScale = this->Bounds[3] - this->Bounds[2]) > scale)
    {
    scale = tempScale;
    }
  if ((tempScale = this->Bounds[5] - this->Bounds[4]) > scale)
    {
    scale = tempScale;
    }
  
  this->XSphere1Actor->SetMapper(sphereMapper);
  this->XSphere1Actor->SetScale(scale/20.0);
  this->XSphere2Actor->SetMapper(sphereMapper);
  this->XSphere2Actor->SetScale(scale/20.0);
  this->YSphere1Actor->SetMapper(sphereMapper);
  this->YSphere1Actor->SetScale(scale/20.0);
  this->YSphere2Actor->SetMapper(sphereMapper);
  this->YSphere2Actor->SetScale(scale/20.0);
  this->ZSphere1Actor->SetMapper(sphereMapper);
  this->ZSphere1Actor->SetScale(scale/20.0);
  this->ZSphere2Actor->SetMapper(sphereMapper);
  this->ZSphere2Actor->SetScale(scale/20.0);
  
  position[0] = this->Bounds[1];
  position[1] = focalPoint[1];
  position[2] = focalPoint[2];
  this->XSphere1Actor->SetPosition(position);
  renderer->AddActor(this->XSphere1Actor);
  position[0] = this->Bounds[0];
  this->XSphere2Actor->SetPosition(position);
  renderer->AddActor(this->XSphere2Actor);
  
  position[0] = focalPoint[0];
  position[1] = this->Bounds[3];
  this->YSphere1Actor->SetPosition(position);
  renderer->AddActor(this->YSphere1Actor);
  position[1] = this->Bounds[2];
  this->YSphere2Actor->SetPosition(position);
  renderer->AddActor(this->YSphere2Actor);
  
  position[1] = focalPoint[1];
  position[2] = this->Bounds[5];
  this->ZSphere1Actor->SetPosition(position);
  renderer->AddActor(this->ZSphere1Actor);
  position[2] = this->Bounds[4];
  this->ZSphere2Actor->SetPosition(position);
  renderer->AddActor(this->ZSphere2Actor);
  
  this->RenderView->Render();
  
  sphere->Delete();
  sphereMapper->Delete();
}

void vtkKWSelectPointInteractor::MotionCallback(int x, int y)
{
  int i, bestId = -1, *size;
  float display[3], spherePos[4];
  float temp, dist2, bestDist2 = VTK_LARGE_FLOAT;
  vtkRenderer *renderer = this->RenderView->GetRenderer();
  float white[3], red[3];
  
  if (!renderer)
    {
    return;
    }
  
  size = renderer->GetSize();
  y = size[1] - y;
  
  for (i = 0; i < 6; i++)
    {
    this->GetSphereCoordinates(i, spherePos);
    spherePos[3] = 1.0;
    renderer->SetWorldPoint(spherePos);
    renderer->WorldToDisplay();
    renderer->GetDisplayPoint(display);
    temp = (float)x - display[0];
    dist2 = temp * temp;
    temp = (float)y - display[1];
    dist2 += temp * temp;
    
    if (dist2 < 300)
      {
      if (dist2 < bestDist2)
        {
        bestDist2 = dist2;
        bestId = i;
        }
      }
    }
  
  if (bestId != this->CurrentSphereId)
    {
    white[0] = white[1] = white[2] = 1.0;
    red[0] = 1.0;
    red[1] = red[2] = 0.0;
    this->ColorSphere(this->CurrentSphereId, white);
    this->ColorSphere(bestId, red);
    this->CurrentSphereId = bestId;
    this->RenderView->Render();
    }
}

void vtkKWSelectPointInteractor::Button1Motion(int x, int y)
{
  vtkRenderer *renderer = this->RenderView->GetRenderer();
  int *size;
  int newY, i;
  float newWorldPt[4], focalPt[3], sphereCenter[3], sphereDisplay[3];
  int dimensionality, endPtId;
  
  if (!renderer || this->CurrentSphereId == -1)
    {
    return;
    }
  
  this->GetSphereCoordinates(this->CurrentSphereId, sphereCenter);
  renderer->SetWorldPoint(sphereCenter[0], sphereCenter[1],
                          sphereCenter[2], 1);
  renderer->WorldToDisplay();
  renderer->GetDisplayPoint(sphereDisplay);
  
  this->Cursor->GetFocalPoint(focalPt);

  size = renderer->GetSize();
  newY = size[1] - y;

  renderer->SetDisplayPoint(x, newY, sphereDisplay[2]);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(newWorldPt);  
  
  if (newWorldPt[3] != 0 && newWorldPt[3] != 1.0)
    {
    for (i = 0; i < 3; i++)
      {
      newWorldPt[i] /= newWorldPt[3];
      }
    }

  if (newWorldPt[0] < this->Bounds[0])
    {
    newWorldPt[0] = this->Bounds[0];
    }
  else if (newWorldPt[0] > this->Bounds[1])
    {
    newWorldPt[0] = this->Bounds[1];
    }
  if (newWorldPt[1] < this->Bounds[2])
    {
    newWorldPt[1] = this->Bounds[2];
    }
  else if (newWorldPt[1] > this->Bounds[3])
    {
    newWorldPt[1] = this->Bounds[3];
    }
  if (newWorldPt[2] < this->Bounds[4])
    {
    newWorldPt[2] = this->Bounds[4];
    }
  else if (newWorldPt[2] > this->Bounds[5])
    {
    newWorldPt[2] = this->Bounds[5];
    }
  
  switch (this->CurrentSphereId)
    {
    case 0:
    case 1:
      // changing y and z coords
      focalPt[1] = newWorldPt[1];
      focalPt[2] = newWorldPt[2];
      break;
    case 2:
    case 3:
      // changing x and z coords
      focalPt[0] = newWorldPt[0];
      focalPt[2] = newWorldPt[2];
      break;
    case 4:
    case 5:
      // changing x and y coords
      focalPt[0] = newWorldPt[0];
      focalPt[1] = newWorldPt[1];
      break;
    default:
      break;
    }
  
  this->XSphere1Actor->SetPosition(this->Bounds[1], focalPt[1], focalPt[2]);
  this->XSphere2Actor->SetPosition(this->Bounds[0], focalPt[1], focalPt[2]);
  this->YSphere1Actor->SetPosition(focalPt[0], this->Bounds[3], focalPt[2]);
  this->YSphere2Actor->SetPosition(focalPt[0], this->Bounds[2], focalPt[2]);
  this->ZSphere1Actor->SetPosition(focalPt[0], focalPt[1], this->Bounds[5]);
  this->ZSphere2Actor->SetPosition(focalPt[0], focalPt[1], this->Bounds[4]);
  this->Cursor->SetFocalPoint(focalPt);

  this->SelectedPoint[0] = focalPt[0];
  this->SelectedPoint[1] = focalPt[1];
  this->SelectedPoint[2] = focalPt[2];

  if (this->PVProbe)
    {
    dimensionality = this->PVProbe->GetDimensionality();
    if (dimensionality == 0)
      {
      this->PVProbe->SetSelectedPoint(this->SelectedPoint);
      }
    else if (dimensionality == 1)
      {
      endPtId = this->PVProbe->GetCurrentEndPoint();
      if (endPtId == 1)
        {
        this->PVProbe->SetEndPoint1(this->SelectedPoint);
        }
      else if (endPtId == 2)
        {
        this->PVProbe->SetEndPoint2(this->SelectedPoint);
        }
      }
    }
  
  this->RenderView->Render();
}

void vtkKWSelectPointInteractor::GetSphereCoordinates(int i, float coords[3])
{
  switch (i)
    {
    case 0:
      this->XSphere1Actor->GetPosition(coords);
      break;
    case 1:
      this->XSphere2Actor->GetPosition(coords);
      break;
    case 2:
      this->YSphere1Actor->GetPosition(coords);
      break;
    case 3:
      this->YSphere2Actor->GetPosition(coords);
      break;
    case 4:
      this->ZSphere1Actor->GetPosition(coords);
      break;
    case 5:
      this->ZSphere2Actor->GetPosition(coords);
      break;
    default:
      break;
    }
}

void vtkKWSelectPointInteractor::ColorSphere(int i, float rgb[3])
{
  switch (i)
    {
    case 0:
      this->XSphere1Actor->GetProperty()->SetColor(rgb);
      break;
    case 1:
      this->XSphere2Actor->GetProperty()->SetColor(rgb);
      break;
    case 2:
      this->YSphere1Actor->GetProperty()->SetColor(rgb);
      break;
    case 3:
      this->YSphere2Actor->GetProperty()->SetColor(rgb);
      break;
    case 4:
      this->ZSphere1Actor->GetProperty()->SetColor(rgb);
      break;
    case 5:
      this->ZSphere2Actor->GetProperty()->SetColor(rgb);
      break;
    default:
      break;
    }
}

void vtkKWSelectPointInteractor::SetCursorVisibility(int value)
{
  this->CursorActor->SetVisibility(value);
  this->XSphere1Actor->SetVisibility(value);
  this->XSphere2Actor->SetVisibility(value);
  this->YSphere1Actor->SetVisibility(value);
  this->YSphere2Actor->SetVisibility(value);
  this->ZSphere1Actor->SetVisibility(value);
  this->ZSphere2Actor->SetVisibility(value);
  this->RenderView->Render();
}

void vtkKWSelectPointInteractor::SetPVProbe(vtkPVProbe *probe)
{
  this->PVProbe = probe;
}

void vtkKWSelectPointInteractor::SetSelectedPoint(float X, float Y, float Z)
{
  this->SelectedPoint[0] = X;
  this->SelectedPoint[1] = Y;
  this->SelectedPoint[2] = Z;
  
  if (this->SelectedPoint[0] < this->Bounds[0])
    {
    this->SelectedPoint[0] = this->Bounds[0];
    }
  else if (this->SelectedPoint[0] > this->Bounds[1])
    {
    this->SelectedPoint[0] = this->Bounds[1];
    }
  if (this->SelectedPoint[1] < this->Bounds[2])
    {
    this->SelectedPoint[1] = this->Bounds[2];
    }
  else if (this->SelectedPoint[1] > this->Bounds[3])
    {
    this->SelectedPoint[1] = this->Bounds[3];
    }
  if (this->SelectedPoint[2] < this->Bounds[4])
    {
    this->SelectedPoint[2] = this->Bounds[4];
    }
  else if (this->SelectedPoint[2] > this->Bounds[5])
    {
    this->SelectedPoint[2] = this->Bounds[5];
    }
  
  if (this->PVProbe->GetDimensionality() == 0)
    {
    this->PVProbe->SetSelectedPoint(this->SelectedPoint);
    }
  else
    {
    if (this->PVProbe->GetCurrentEndPoint() == 1)
      {
      this->PVProbe->SetEndPoint1(this->SelectedPoint);
      }
    else
      {
      this->PVProbe->SetEndPoint2(this->SelectedPoint);
      }
    }
  
  this->Cursor->SetFocalPoint(this->SelectedPoint);
  
  this->XSphere1Actor->SetPosition(this->Bounds[1], this->SelectedPoint[1],
                                   this->SelectedPoint[2]);
  this->XSphere2Actor->SetPosition(this->Bounds[0], this->SelectedPoint[1],
                                   this->SelectedPoint[2]);
  this->YSphere1Actor->SetPosition(this->SelectedPoint[0], this->Bounds[3],
                                   this->SelectedPoint[2]);
  this->YSphere2Actor->SetPosition(this->SelectedPoint[0], this->Bounds[2],
                                   this->SelectedPoint[2]);
  this->ZSphere1Actor->SetPosition(this->SelectedPoint[0],
                                   this->SelectedPoint[1], this->Bounds[5]);
  this->ZSphere2Actor->SetPosition(this->SelectedPoint[0],
                                   this->SelectedPoint[1], this->Bounds[4]);

  this->RenderView->Render();
}

void vtkKWSelectPointInteractor::SetSelectedPoint(float point[3])
{
  this->SetSelectedPoint(point[0], point[1], point[2]);
}

void vtkKWSelectPointInteractor::SetSelectedPointX(float X)
{
  this->SetSelectedPoint(X, this->SelectedPoint[1], this->SelectedPoint[2]);
}

void vtkKWSelectPointInteractor::SetSelectedPointY(float Y)
{
  this->SetSelectedPoint(this->SelectedPoint[0], Y, this->SelectedPoint[2]);
}

void vtkKWSelectPointInteractor::SetSelectedPointZ(float Z)
{
  this->SetSelectedPoint(this->SelectedPoint[0], this->SelectedPoint[1], Z);
}
