/*=========================================================================

  Program:   ParaView
  Module:    vtkPVJoystickFly.cxx
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
#include "vtkPVJoystickFly.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

vtkCxxRevisionMacro(vtkPVJoystickFly, "1.5");

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
vtkPVJoystickFly::~vtkPVJoystickFly()
{
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::OnButtonDown(int x, int y, vtkRenderer *ren,
                                    vtkRenderWindowInteractor* rwi)
{
  if ( this->In < 0 )
    {
    vtkErrorMacro("Joystick Fly manipulator has to be used from one of the two subclasses (In and Out)");
    return;
    }
  if ( !this->Application )
    {
    vtkErrorMacro("Application is not defined");
    return;
    }
  if ( !ren ||!rwi )
    {
    vtkErrorMacro("Renderer or Render Window Interactor are not defined");
    return;
    }
  this->LastX = x;
  this->LastY = y;

  double *range = ren->GetActiveCamera()->GetClippingRange();
  this->Fly(ren, rwi, range[1], (this->In?1:-1)*this->FlySpeed);
}


//-------------------------------------------------------------------------
void vtkPVJoystickFly::OnButtonUp(int, int, vtkRenderer*,
                                  vtkRenderWindowInteractor*)
{
  this->FlyFlag = 0;
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::OnMouseMove(int x, int y, vtkRenderer*,
                                   vtkRenderWindowInteractor*)
{
  // Need to update the instance variables for mouse position. This
  // will be called when update happens.
  this->LastX = x;
  this->LastY = y;
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::Fly(vtkRenderer* ren, vtkRenderWindowInteractor *rwi,
                           float, float ispeed)
{
  if ( this->FlyFlag || !this->Application )
    {
    return;
    }
  
  // We'll need the size of the window
  int *size = ren->GetSize();

  // Also we will need the camera.
  vtkCamera *cam = ren->GetActiveCamera();  

  // We need to time rendering so that we can adjust the speed
  // accordingly
  vtkTimerLog *timer = vtkTimerLog::New();

  // We are flying now!
  this->FlyFlag = 1;

  float speed, angle; 
  
  // The first time through we don't want to move
  int first = 1;
  
  // As long as the mouse is still pressed
  while (this->FlyFlag)
    {
    // Don't move - we need at least one time to determine
    // what our increments should be
    if ( first )
      {
      speed = 0;
      angle = 0;
      first = 0;
      }
    // This is not the first time - Figure out how long the last loop
    // took. Use this plus the scale value and the distance
    // between the near and the far plane to determine the
    // forward/backward translation increment. This keeps the apparent 
    // motion independent of rendering speed or scale of the data.
    // For the angle, our factor is based soley on rendering speed here.
    else
      {
      timer->StopTimer();
      float t = timer->GetElapsedTime();
      double *range = cam->GetClippingRange();
      speed = ispeed * 
        (range[1] - range[0])/100.0 * t;
      angle = t;
      }

    float lastx = this->LastX;
    float lasty = size[1] - this->LastY - 1;
    
    // Start the timer up again
    timer->StartTimer();
    
    // If our mouse is not in the center, rotate 
    if ( size[0]/2 - lastx >  20 ||
         size[0]/2 - lastx < -20 ||
         size[1]/2 - lasty >  20 ||
         size[1]/2 - lasty < -20 )
      {
      // The angle is a square value to increase rotation faster as
      // you move out from the center
      float vx = 0.001 * angle *
        ((size[0]/2 - lastx>0)?(1):(-1)) *
        (size[0]/2 - lastx)*
        (size[0]/2 - lastx);
      float vy = 0.001 * angle *
        ((size[1]/2 - lasty>0)?(1):(-1)) *
        (size[1]/2 - lasty)*
        (size[1]/2 - lasty);

      // If we are in parallel projection mode, we need to tone
      // down this rotation if we are close in (small parallel
      // scale)
      if ( cam->GetParallelProjection() )
        {
        float reduce = 
          cam->GetParallelScale() / 100.0;
        reduce = (reduce > 1.0)?(1.0):(reduce);
        vx *= reduce;
        vy *= reduce;
        }
      
      // Do the rotation
      cam->Yaw(vx);
      cam->Pitch(vy);
      cam->OrthogonalizeViewUp();
      
      // Now figure out if we should slow down our speed because
      // we are trying to make a sharp turn
      vx = (vx<0)?(-vx):(vx);
      vy = (vy<0)?(-vy):(vy);
      vx += vy;
      if ( vx >= 1.0 )
        {
        speed = 0;
        }
      else
        {
        speed *= (1.0 - vx);
        }
      }
    
    // In parallel we need to adjust the parallel scale
    if ( cam->GetParallelProjection() )
      {
      float scale = cam->GetParallelScale();
      scale -= 0.004*speed*scale;
      cam->SetParallelScale(scale);
      }
    // In perspective, we need to translate the position and
    // focal point of the camera
    else
      {
      float fp[3], pos[3];
      cam->GetPosition(pos);
      cam->GetFocalPoint(fp);
    
      float dir[3];
      
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
      }
    
    ren->ResetCameraClippingRange();
    rwi->Render();
    
    // Update to process mouse events to get the new position
    // and to check for mouse up events
    this->Application->Script("update");
    }
    
  timer->Delete();
}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::ComputeCameraAxes(vtkRenderer* ren)
{
  vtkCamera *camera = ren->GetActiveCamera();
  
  camera->OrthogonalizeViewUp();
  camera->GetViewUp(this->CameraYAxis);
  camera->GetDirectionOfProjection(this->CameraZAxis);
  
  // I do not know if this is actually used, but this was originally
  // set to the ViewPlaneNormal.
  this->CameraZAxis[0] = - this->CameraZAxis[0];
  this->CameraZAxis[1] = - this->CameraZAxis[1];
  this->CameraZAxis[2] = - this->CameraZAxis[2];  

  vtkMath::Cross(this->CameraYAxis, this->CameraZAxis, this->CameraXAxis);
}

//void vtkPVJoystickFly::SetFlySpeed(double d)
//{
//  this->FlySpeed = d;
//  cout << this << ": Set flyspeed to " << d << endl;
//}

//-------------------------------------------------------------------------
void vtkPVJoystickFly::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}






