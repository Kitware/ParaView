/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFly.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVJoystickFly.h"

#include "vtkCamera.h"
#include "vtkCameraManipulatorGUIHelper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

//-------------------------------------------------------------------------
vtkPVJoystickFly::vtkPVJoystickFly()
{
  this->In = -1;
  this->FlyFlag = 0;
  this->FlySpeed = 20.0;
  this->LastRenderTime = 0.1;

  this->CameraXAxis[0] = 1.0;
  this->CameraXAxis[1] = 0.0;
  this->CameraXAxis[2] = 0.0;

  this->CameraYAxis[0] = 0.0;
  this->CameraYAxis[1] = 1.0;
  this->CameraYAxis[2] = 0.0;

  this->CameraZAxis[0] = 0.0;
  this->CameraZAxis[1] = 0.0;
  this->CameraZAxis[2] = 1.0;
}

//-------------------------------------------------------------------------
vtkPVJoystickFly::~vtkPVJoystickFly() = default;

//-------------------------------------------------------------------------
void vtkPVJoystickFly::OnButtonDown(int, int, vtkRenderer* ren, vtkRenderWindowInteractor* rwi)
{
  if (this->In < 0)
  {
    vtkErrorMacro(
      "Joystick Fly manipulator has to be used from one of the two subclasses (In and Out)");
    return;
  }
  if (!this->GetGUIHelper())
  {
    vtkErrorMacro("GUIHelper is not defined");
    return;
  }
  if (!ren || !rwi)
  {
    vtkErrorMacro("Renderer or Render Window Interactor are not defined");
    return;
  }

  double* range = ren->GetActiveCamera()->GetClippingRange();
  this->Fly(ren, rwi, range[1], (this->In ? 1 : -1) * this->FlySpeed * .01);
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::OnButtonUp(int, int, vtkRenderer*, vtkRenderWindowInteractor* rwi)
{
  this->FlyFlag = 0;
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::OnMouseMove(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::Fly(vtkRenderer* ren, vtkRenderWindowInteractor* rwi, double, double ispeed)
{
  if (this->FlyFlag || !this->GetGUIHelper())
  {
    return;
  }

  // We'll need the size of the window
  int* size = ren->GetSize();

  // Also we will need the camera.
  vtkCamera* cam = ren->GetActiveCamera();

  // We need to time rendering so that we can adjust the speed
  // accordingly
  vtkTimerLog* timer = vtkTimerLog::New();

  // We are flying now!
  this->FlyFlag = 1;

  double speed;

  // The first time through we don't want to move
  int first = 1;

  // As long as the mouse is still pressed
  while (this->FlyFlag)
  {
    double* range = cam->GetClippingRange();
    double dist = 0.5 * (range[1] + range[0]);
    double lastx = rwi->GetLastEventPosition()[0];
    double lasty = size[1] - rwi->GetLastEventPosition()[1] - 1;

    // Compute a new render time if appropriate (delta time).
    if (!first)
    {
      // We need at least one time to determine
      // what our increments should be.
      timer->StopTimer();
      this->LastRenderTime = timer->GetElapsedTime();
      // Limit length of render time because we do not want such large jumps
      // when rendering takes more than 1 second.
      if (this->LastRenderTime > 1.0)
      {
        this->LastRenderTime = 1.0;
      }
    }
    first = 0;

    // Compute angle ralative to viewport.
    // These values will be from -0.5 to 0.5
    double vx = (size[0] / 2 - lastx) / (double)(size[0]);
    double vy = (size[1] / 2 - lasty) / (double)(size[0]);

    // Convert to world angle by multiplying by view angle.
    // (Speed up rotation for wide angle views).
    double viewAngle;
    if (cam->GetParallelProjection())
    { // We need to compute a pseudo viewport angle.
      double parallelScale = cam->GetParallelScale();
      viewAngle = 360.0 * atan2(parallelScale * 0.5, dist) / vtkMath::Pi();
    }
    else
    {
      viewAngle = cam->GetViewAngle();
    }
    vx = vx * viewAngle;
    vy = vy * viewAngle;

    // Compute speed.
    speed = ispeed * range[1];

    // Scale speed and rotation by render time
    // to get constant perceived velocities.
    speed = speed * this->LastRenderTime;
    vx = vx * this->LastRenderTime;
    vy = vy * this->LastRenderTime;

    // Start the timer up again
    timer->StartTimer();

    // Do the rotation
    cam->Yaw(vx);
    cam->Pitch(vy);
    cam->OrthogonalizeViewUp();

    // Now figure out if we should slow down our speed because
    // we are trying to make a sharp turn
    vx = (double)(size[0] / 2 - lastx) / (double)(size[0]);
    vy = (double)(size[1] / 2 - lasty) / (double)(size[1]);
    vx = (vx < 0) ? (-vx) : (vx);
    vy = (vy < 0) ? (-vy) : (vy);
    vx = (vx > vy) ? (vx) : (vy);
    speed *= (1.0 - 2.0 * vx);

    // Move the camera forward based on speed.
    // Although this has no effect for parallel projection,
    // it helps keep the pseudo view angle constant.
    double fp[3], pos[3];
    cam->GetPosition(pos);
    cam->GetFocalPoint(fp);
    double dir[3];
    dir[0] = fp[0] - pos[0];
    dir[1] = fp[1] - pos[1];
    dir[2] = fp[2] - pos[2];
    vtkMath::Normalize(dir);
    dir[0] *= speed;
    dir[1] *= speed;
    dir[2] *= speed;
    fp[0] += dir[0];
    fp[1] += dir[1];
    fp[2] += dir[2];
    pos[0] += dir[0];
    pos[1] += dir[1];
    pos[2] += dir[2];
    cam->SetPosition(pos);
    cam->SetFocalPoint(fp);

    // In parallel we need to adjust the parallel scale
    if (cam->GetParallelProjection())
    {
      double scale = cam->GetParallelScale();
      if (dist > 0.0 && dist > speed)
      {
        scale = scale * (dist - speed) / dist;
        cam->SetParallelScale(scale);
      }
    }

    rwi->Render();

    // Update to process mouse events to get the new position
    // and to check for mouse up events
    this->GetGUIHelper()->UpdateGUI();
  }

  timer->Delete();
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::ComputeCameraAxes(vtkRenderer* ren)
{
  vtkCamera* camera = ren->GetActiveCamera();

  camera->OrthogonalizeViewUp();
  camera->GetViewUp(this->CameraYAxis);
  camera->GetDirectionOfProjection(this->CameraZAxis);

  // I do not know if this is actually used, but this was originally
  // set to the ViewPlaneNormal.
  this->CameraZAxis[0] = -this->CameraZAxis[0];
  this->CameraZAxis[1] = -this->CameraZAxis[1];
  this->CameraZAxis[2] = -this->CameraZAxis[2];

  vtkMath::Cross(this->CameraYAxis, this->CameraZAxis, this->CameraXAxis);
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FlySpeed: " << this->FlySpeed << endl;
}
