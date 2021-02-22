/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRPolyfill.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenVROverlayInternal.h"
#include "vtkOpenVRRenderWindow.h"

vtkStandardNewMacro(vtkOpenVRPolyfill);

vtkOpenVRPolyfill::vtkOpenVRPolyfill()
{
  this->RenderWindow = nullptr;
  this->OpenVRRenderWindow = nullptr;

  this->PhysicalPose = new vtkOpenVRCameraPose();
  this->PhysicalPose->Distance = 1.0;
  std::fill(this->PhysicalPose->Translation, this->PhysicalPose->Translation + 3, 0.0);
  this->PhysicalPose->PhysicalViewDirection[0] = 0;
  this->PhysicalPose->PhysicalViewDirection[1] = 0;
  this->PhysicalPose->PhysicalViewDirection[2] = -1;
  this->PhysicalPose->PhysicalViewUp[0] = 0;
  this->PhysicalPose->PhysicalViewUp[1] = 1.0;
  this->PhysicalPose->PhysicalViewUp[2] = 0.0;

  // create a unique ID that can be used by desktop to
  // simulate a physical position for new poses
  uint64_t id = static_cast<uint64_t>(std::time(nullptr));
  this->ID = (id % 12) - 5.5;
}

vtkOpenVRPolyfill::~vtkOpenVRPolyfill()
{
  delete this->PhysicalPose;
}

void vtkOpenVRPolyfill::SetRenderWindow(vtkOpenGLRenderWindow* rw)
{
  auto ovr = vtkOpenVRRenderWindow::SafeDownCast(rw);
  this->OpenVRRenderWindow = ovr;
  this->RenderWindow = rw;
}

double vtkOpenVRPolyfill::GetPhysicalScale()
{
  if (this->OpenVRRenderWindow)
  {
    return this->OpenVRRenderWindow->GetPhysicalScale();
  }

  return this->PhysicalPose->Distance;
}

void vtkOpenVRPolyfill::SetPhysicalScale(double val)
{
  if (this->OpenVRRenderWindow)
  {
    this->OpenVRRenderWindow->SetPhysicalScale(val);
  }
  this->PhysicalPose->Distance = val;
}

double* vtkOpenVRPolyfill::GetPhysicalTranslation()
{
  if (this->OpenVRRenderWindow)
  {
    return this->OpenVRRenderWindow->GetPhysicalTranslation();
  }

  return this->PhysicalPose->Translation;
}

void vtkOpenVRPolyfill::SetPhysicalTranslation(double v1, double v2, double v3)
{
  if (this->OpenVRRenderWindow)
  {
    this->OpenVRRenderWindow->SetPhysicalTranslation(v1, v2, v3);
  }

  this->PhysicalPose->Translation[0] = v1;
  this->PhysicalPose->Translation[1] = v2;
  this->PhysicalPose->Translation[2] = v3;
}

double* vtkOpenVRPolyfill::GetPhysicalViewDirection()
{
  if (this->OpenVRRenderWindow)
  {
    return this->OpenVRRenderWindow->GetPhysicalViewDirection();
  }

  return this->PhysicalPose->PhysicalViewDirection;
}

void vtkOpenVRPolyfill::SetPhysicalViewDirection(double v1, double v2, double v3)
{
  if (this->OpenVRRenderWindow)
  {
    this->OpenVRRenderWindow->SetPhysicalViewDirection(v1, v2, v3);
  }

  this->PhysicalPose->PhysicalViewDirection[0] = v1;
  this->PhysicalPose->PhysicalViewDirection[1] = v2;
  this->PhysicalPose->PhysicalViewDirection[2] = v3;
}

double* vtkOpenVRPolyfill::GetPhysicalViewUp()
{
  if (this->OpenVRRenderWindow)
  {
    return this->OpenVRRenderWindow->GetPhysicalViewUp();
  }

  return this->PhysicalPose->PhysicalViewUp;
}

void vtkOpenVRPolyfill::SetPhysicalViewUp(double v1, double v2, double v3)
{
  if (this->OpenVRRenderWindow)
  {
    this->OpenVRRenderWindow->SetPhysicalViewUp(v1, v2, v3);
  }

  this->PhysicalPose->PhysicalViewUp[0] = v1;
  this->PhysicalPose->PhysicalViewUp[1] = v2;
  this->PhysicalPose->PhysicalViewUp[2] = v3;
}

void vtkOpenVRPolyfill::SetPose(
  vtkOpenVRCameraPose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin)
{
  auto cam = ren->GetActiveCamera();
  auto ovrcam = vtkOpenVRCamera::SafeDownCast(cam);
  if (ovrcam)
  {
    thePose->Set(ovrcam, static_cast<vtkOpenVRRenderWindow*>(renWin));
  }
  else
  {
    auto* opose = this->GetPhysicalPose();
    std::copy(opose->Translation, opose->Translation + 3, thePose->Translation);
    std::copy(opose->PhysicalViewUp, opose->PhysicalViewUp + 3, thePose->PhysicalViewUp);
    thePose->Distance = opose->Distance;
    std::copy(opose->PhysicalViewDirection, opose->PhysicalViewDirection + 3,
      thePose->PhysicalViewDirection);

    cam->GetPosition(thePose->Position);
    cam->GetDirectionOfProjection(thePose->ViewDirection);

    thePose->Loaded = true;
  }
}

void vtkOpenVRPolyfill::ApplyPose(
  vtkOpenVRCameraPose* thePose, vtkOpenGLRenderer* ren, vtkOpenGLRenderWindow* renWin)
{
  auto cam = ren->GetActiveCamera();
  auto ovrcam = vtkOpenVRCamera::SafeDownCast(cam);
  if (ovrcam)
  {
    thePose->Apply(ovrcam, static_cast<vtkOpenVRRenderWindow*>(renWin));
  }
  else
  {
    this->SetPhysicalScale(thePose->Distance);
    this->SetPhysicalTranslation(thePose->Translation);
    this->SetPhysicalViewDirection(thePose->PhysicalViewDirection);
    this->SetPhysicalViewUp(thePose->PhysicalViewUp);

    cam->SetPosition(thePose->Position);
    cam->SetFocalPoint(thePose->Position[0] + thePose->Distance * thePose->ViewDirection[0],
      thePose->Position[1] + thePose->Distance * thePose->ViewDirection[1],
      thePose->Position[2] + thePose->Distance * thePose->ViewDirection[2]);
    cam->SetViewUp(thePose->PhysicalViewUp);
    cam->OrthogonalizeViewUp();

    // rotate about a point 2 meters into the view direction
    double vec[3] = { -2.0 * thePose->Distance * thePose->ViewDirection[0],
      -2.0 * thePose->Distance * thePose->ViewDirection[1],
      -2.0 * thePose->Distance * thePose->ViewDirection[2] };
    double q[4] = { this->ID * 3.1415 * 0.05, thePose->PhysicalViewUp[0],
      thePose->PhysicalViewUp[1], thePose->PhysicalViewUp[2] };

    vtkMath::RotateVectorByWXYZ(vec, q, vec);
    cam->SetPosition(
      thePose->Position[0] + 2.0 * thePose->Distance * thePose->ViewDirection[0] + vec[0],
      thePose->Position[1] + 2.0 * thePose->Distance * thePose->ViewDirection[1] + vec[1],
      thePose->Position[2] + 2.0 * thePose->Distance * thePose->ViewDirection[2] + vec[2]);
  }
}
