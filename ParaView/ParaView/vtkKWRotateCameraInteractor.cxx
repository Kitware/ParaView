/*=========================================================================

  Program:   ParaView
  Module:    vtkKWRotateCameraInteractor.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWRotateCameraInteractor.h"
#include "vtkKWToolbar.h"
#include "vtkKWCenterOfRotation.h"
#include "vtkPVRenderView.h"
#include "vtkObjectFactory.h"

int vtkKWRotateCameraInteractorCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);



#define VTK_VIEW_PAN 1
#define VTK_VIEW_ZOOM 2
#define VTK_VIEW_ROTATE 3
#define VTK_VIEW_ROLL 4

//----------------------------------------------------------------------------
vtkKWRotateCameraInteractor::vtkKWRotateCameraInteractor()
{
  this->CommandFunction = vtkKWRotateCameraInteractorCommand;

  // Create the toolbar for the speed slider
  if (this->Toolbar)
    {
    vtkErrorMacro("Toolbar already set!");
    }
  this->Toolbar = vtkKWToolbar::New();

  this->CenterUI = vtkKWCenterOfRotation::New();
  this->CenterUI->SetParentInteractor(this);
  this->CenterUI->SetParent(this->Toolbar);

  this->Interactor = vtkCameraInteractor::New();
  this->CenterUI->SetCameraInteractor(this->Interactor);

  this->CursorState = 0;
}

//----------------------------------------------------------------------------
vtkKWRotateCameraInteractor *vtkKWRotateCameraInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWRotateCameraInteractor");
  if(ret)
    {
    return (vtkKWRotateCameraInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWRotateCameraInteractor;
}


//----------------------------------------------------------------------------
vtkKWRotateCameraInteractor::~vtkKWRotateCameraInteractor()
{
  this->PrepareForDelete();
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::PrepareForDelete()
{
  if (this->CenterUI)
    {
    this->CenterUI->PrepareForDelete();
    this->CenterUI->Delete();
    this->CenterUI = NULL;
    }

  if (this->Interactor)
    {
    this->Interactor->Delete();
    this->Interactor = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  this->Toolbar->SetParent(this->GetParent());
  this->Toolbar->Create(app);

  // create the main frame for this widget
  this->Script( "frame %s", this->GetWidgetName());

  this->CenterUI->SetRenderView(this->RenderView);
  this->CenterUI->Create(app,"");
  this->Script( "pack %s -side left -expand 0 -fill x",
                this->CenterUI->GetWidgetName());
  this->CenterUI->GetResetButton()->SetBalloonHelpString(
    "This button sets the center of rotation back to the center of the screen.");


}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::Select()
{
  if (this->SelectedState)
    {
    return;
    }
  this->vtkKWInteractor::Select();

  // Cause a recompute of the center of rotation if default value.
  // CenterOfRotationDoes not have a select method. 
  this->CameraMovedNotify();
  this->CenterUI->ShowCenterOn();


  // Should change the cursor here -- done in MotionCallback

  // Do a full res render.
  this->RenderView->EventuallyRender(); 

}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::Deselect()
{
  if ( ! this->SelectedState)
    {
    return;
    }
  this->vtkKWInteractor::Deselect();

  this->CenterUI->ShowCenterOff();
  
   // Should change the cursor here
  this->Script("%s configure -cursor top_left_arrow",
               this->RenderView->GetWidgetName());
                 
  this->RenderView->SetRenderModeToStill();
  this->RenderView->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::CameraMovedNotify()
{
  this->CenterUI->CameraMovedNotify();  
}


//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::SetRenderView(vtkPVRenderView *view)
{
  this->vtkKWInteractor::SetRenderView(view);
  this->CenterUI->SetRenderView(view);

  if (view)
    {
    this->Interactor->SetRenderer(view->GetRenderer());
    }
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::AButtonPress(int num, int x, int y)
{
  if (num == 1)
    {
    this->Interactor->RotateRollStart(x, y);
    this->RenderView->SetRenderModeToInteractive();
    }
  if (num == 3)
    { // Hard code this for windows.
    int *size;
    double pos;
    // This is the same check in the CameraInteractor.
    // They should be combined in this class.
    size = this->RenderView->GetRenderer()->GetSize();
    pos = (double)y / (double)(size[1]);
    if (pos < 0.333 || pos > 0.667)
      {
#ifdef _WIN32      
      this->Script("%s configure -cursor size_ns",
                   this->RenderView->GetWidgetName());
#else      
      this->Script("%s configure -cursor sb_v_double_arrow",
		   this->RenderView->GetWidgetName());
#endif      
      this->CursorState = VTK_VIEW_ZOOM;
      }
    else
      {
      this->Script("%s configure -cursor fleur",
                   this->RenderView->GetWidgetName());
      this->CursorState = VTK_VIEW_PAN;
      }

    this->Interactor->PanZoomStart(x, y);
    this->RenderView->SetRenderModeToInteractive(); 
    }
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::AButtonRelease(int num, int x, int y)
{
  if (num == 2)
    {
    return;
    }
  this->RenderView->EventuallyRender(); 
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::Button1Motion(int x, int y)
{
  // Rather complicated to do this her also.
  // Change the cursor depending on which portion of screen we are on.
  if (this->CursorState == VTK_VIEW_ROLL)
    {
    vtkRenderer *ren = this->RenderView->GetRenderer();
    int *size = ren->GetSize();
    double px, py;
    double *worldCenter;
    float displayCenter[3];

    // Get the center in Screen Coordinates
    worldCenter = this->Interactor->GetCenter();
    ren->SetWorldPoint(worldCenter[0], worldCenter[1],
                      worldCenter[2], 1.0);
    ren->WorldToDisplay();
    ren->GetDisplayPoint(displayCenter);

    px = (double)(x - displayCenter[0]) / (double)(size[0]);
    // Flip y axis (origin from top left to bottom left).
    py = (double)(size[1] - y - displayCenter[1]) / (double)(size[1]);  
    this->UpdateRollCursor(px, py);
    }

  this->Interactor->RotateRollMotion(x, y);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  this->RenderView->Render(); 
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::Button3Motion(int x, int y)
{
  this->Interactor->PanZoomMotion(x, y);
  this->RenderView->GetRenderer()->ResetCameraClippingRange();
  this->RenderView->Render(); 
}


//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::MotionCallback(int x, int y)
{
  vtkRenderer *ren = this->RenderView->GetRenderer();
  int *size = ren->GetSize();
  double px, py;
  double *worldCenter;
  float displayCenter[3];

  // Get the center in Screen Coordinates
  worldCenter = this->Interactor->GetCenter();
  ren->SetWorldPoint(worldCenter[0], worldCenter[1],
                     worldCenter[2], 1.0);
  ren->WorldToDisplay();
  ren->GetDisplayPoint(displayCenter);

  px = (double)(x - displayCenter[0]) / (double)(size[0]);
  // Flip y axis (origin from top left to bottom left).
  py = (double)(size[1] - y - displayCenter[1]) / (double)(size[1]);  

  if (fabs(px) < 0.2 && fabs(py) < 0.2)
    {
    if (this->CursorState != VTK_VIEW_ROTATE)
      {
      this->Script("%s configure -cursor top_left_arrow",
                   this->RenderView->GetWidgetName());
      this->CursorState = VTK_VIEW_ROTATE;
      }
    }
  else
    { // Since we have to make do with arrows (win32)
    this->CursorState = VTK_VIEW_ROLL;
    this->UpdateRollCursor(px, py);
    }
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::SetCenter(double x, double y, double z)
{
  this->CenterUI->SetCenter(x, y, z);
}

//----------------------------------------------------------------------------
void vtkKWRotateCameraInteractor::UpdateRollCursor(double px, double py)
{
  double tmp;

#ifdef _WIN32  
  // Since we have to make do with arrows (win32)
  if (px == 0)
    {
    tmp = VTK_LARGE_FLOAT;
    }
  else
    {
    tmp = fabs(py) / fabs(px);
    }
  if (tmp < 0.41421)
    {
    this->Script("%s configure -cursor size_ns",
                 this->RenderView->GetWidgetName());
    }
  else if (tmp > 2.41421)
    {
    this->Script("%s configure -cursor size_we",
                 this->RenderView->GetWidgetName());
    }
  else if (px * py > 0.0)
    {
    this->Script("%s configure -cursor size_nw_se",
                 this->RenderView->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -cursor size_ne_sw",
                 this->RenderView->GetWidgetName());
    }
#else
  // Since we have to make do with arrows (win32)
  if (px == 0)
    {
    tmp = VTK_LARGE_FLOAT;
    }
  else
    {
    tmp = fabs(py) / fabs(px);
    }
  if (tmp < 0.41421)
    {
    this->Script("%s configure -cursor sb_v_double_arrow",
                 this->RenderView->GetWidgetName());
    }
  else if (tmp > 2.41421)
    {
    this->Script("%s configure -cursor sb_h_double_arrow",
                 this->RenderView->GetWidgetName());
    }
  else if (px > 0.0)
    {
    if (py > 0.0)
      {
      this->Script("%s configure -cursor ur_angle",
		   this->RenderView->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -cursor lr_angle",
		   this->RenderView->GetWidgetName());
      }
    }
  else
    {
    if (py > 0.0)
      {
      this->Script("%s configure -cursor ul_angle",
		   this->RenderView->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -cursor ll_angle",
		   this->RenderView->GetWidgetName());
      }
    }
#endif
}
