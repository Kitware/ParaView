/*=========================================================================

  Program:   ParaView
  Module:    vtkKWFlyInteractor.cxx
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
#include "vtkKWWindow.h"
#include "vtkKWFlyInteractor.h"
#include "vtkKWToolbar.h"
#include "vtkPVRenderView.h"
#include "vtkObjectFactory.h"

int vtkKWFlyInteractorCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWFlyInteractor::vtkKWFlyInteractor()
{
  this->CommandFunction = vtkKWFlyInteractorCommand;

  this->Toolbar = vtkKWToolbar::New();

  this->Label = vtkKWWidget::New();
  this->Label->SetParent(this->Toolbar);
  this->SpeedSlider = vtkKWScale::New();
  this->SpeedSlider->SetParent(this->Toolbar);

  this->Helper = vtkCameraInteractor::New();
  this->FlyFlag = 0;

  this->RenderTkWindow = NULL;
  this->PlaneCursor = NULL;
}

//----------------------------------------------------------------------------
vtkKWFlyInteractor *vtkKWFlyInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWFlyInteractor");
  if(ret)
    {
    return (vtkKWFlyInteractor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWFlyInteractor;
}


//----------------------------------------------------------------------------
vtkKWFlyInteractor::~vtkKWFlyInteractor()
{
  this->Toolbar->Delete();
  this->Toolbar = NULL;

  this->Label->Delete();
  this->Label = NULL;
  this->SpeedSlider->Delete();
  this->SpeedSlider = NULL;

  this->Helper->Delete();
  this->Helper = NULL;
}


//----------------------------------------------------------------------------
void vtkKWFlyInteractor::Create(vtkKWApplication *app, char* vtkNotUsed(args))
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);

  // create the main frame for this widget
  //this->Script( "frame %s -bd 1 -relief raised", this->GetWidgetName());

  this->Toolbar->SetParent(this->GetParent());
  this->Toolbar->Create(app);
  this->Toolbar->SetHeight(25);

  this->Label->Create(app,"label","-text {Fly Speed} -bd 2");
  this->Label->SetBalloonHelpString("Change the flying speed");
  this->Script( "pack %s -side left -expand 0 -fill none",
                this->Label->GetWidgetName());
  
  this->SpeedSlider->Create(app,"-resolution 0.1 -orient horizontal -bd 2");
  this->SpeedSlider->SetRange(0, 50);
  this->SpeedSlider->SetValue(20.0);    
  this->SpeedSlider->SetBalloonHelpString("Change the flying speed");
  this->Script( "pack %s -side left -expand 0 -fill none",
                this->SpeedSlider->GetWidgetName());

  //this->CreateCursor();
}

//----------------------------------------------------------------------------
void vtkKWFlyInteractor::Select()
{
  if (this->SelectedState)
    {
    return;
    }
  this->vtkKWInteractor::Select();

  // Should change the cursor here
  this->Script("%s configure -cursor {%s}; update",
               this->RenderView->GetWidgetName(), "arrow");
  //Tk_DefineCursor(this->RenderTkWindow, this->PlaneCursor);
}

//----------------------------------------------------------------------------
void vtkKWFlyInteractor::Deselect()
{
  if ( ! this->SelectedState)
    {
    return;
    }
  this->vtkKWInteractor::Deselect();

  // Should change the cursor here
  this->Script("%s configure -cursor top_left_arrow",
               this->RenderView->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWFlyInteractor::AButtonPress(int num, int x, int y)
{
  float speed = static_cast<float>(this->SpeedSlider->GetValue())/10;
  int xmin, ymin;
  double *range;
  const char *renWidgetName = this->RenderView->GetVTKWidget()->GetWidgetName();

  if (this->RenderView == NULL)
    {
    return;
    }

  // Scale speed by the cameras clipping range.
  range = this->RenderView->GetRenderer()->GetActiveCamera()->GetClippingRange();
  speed *= (range[1] - range[2]) / 200.0;

  this->Helper->SetRenderer(this->RenderView->GetRenderer());

  if (num == 3)
    {
    speed = -speed;
    }

  // get the relative position of the render in the window
  this->Script( "winfo rootx %s",renWidgetName);
  xmin = vtkKWObject::GetIntegerResult(this->Application);
  this->Script( "winfo rooty %s", renWidgetName);
  ymin = vtkKWObject::GetIntegerResult(this->Application);

  
  // Set a state variable the indicates flying vs stop.
  this->FlyFlag = 1;
  this->RenderView->SetRenderModeToInteractive(); 
  while (this->FlyFlag)
    {
    // Get the position of the mouse in the renderer.
    this->Script( "winfo pointerx %s", renWidgetName);
    x = vtkKWObject::GetIntegerResult(this->Application);
    this->Script( "winfo pointery %s", renWidgetName);
    y = vtkKWObject::GetIntegerResult(this->Application);
    
    // relative to render window
    x = x - xmin;
    y = y - ymin;

    this->Helper->Fly(x, y, speed);
  
    this->RenderView->GetRenderer()->ResetCameraClippingRange();
    this->RenderView->Render();
  
    // Check to see if user has let go of mouse
    this->Script( "update");
    }
}

//----------------------------------------------------------------------------
void vtkKWFlyInteractor::AButtonRelease(int vtkNotUsed(num), 
                                        int vtkNotUsed(x), 
                                        int vtkNotUsed(y))
{
  if (this->RenderView == NULL)
    {
    return;
    }

  this->FlyFlag = 0;
  this->RenderView->EventuallyRender();
  
  vtkCamera *cam;
  double *pos, *fp, *up;
  // Lets always save the camera in the state on still render.
  cam = this->RenderView->GetRenderer()->GetActiveCamera();
  pos = cam->GetPosition();
  fp = cam->GetFocalPoint();
  up = cam->GetViewUp();
  this->AddTraceEntry("$kw(%s) SetCameraState %.3f %.3f %.3f  %.3f %.3f %.3f  %.3f %.3f %.3f",
                      this->GetTclName(), pos[0],pos[1],pos[2], 
                      fp[0],fp[1],fp[2], up[0],up[1],up[2]);
}



#define plane3_width 21 
#define plane3_height 21 
static unsigned char plane3_bits[] = { \
   0x00, 0x04, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x0a, 0x00, \
   0x00, 0x0a, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x1b, 0x00, 0x80, 0x2a, 0x00, \
   0x40, 0x4a, 0x00, 0x20, 0x8a, 0x00, 0x10, 0x0a, 0x01, 0x08, 0x1b, 0x02, \
   0xc4, 0x6a, 0x04, 0x32, 0x8a, 0x09, 0x0d, 0x0a, 0x16, 0x03, 0x0a, 0x18, \
   0x00, 0x1f, 0x00, 0x80, 0x24, 0x00, 0x40, 0x44, 0x00, 0xc0, 0x7f, 0x00, \
   0x00, 0x04, 0x00};

#define plane3_mask_width 21
#define plane3_mask_height 21
static unsigned char plane3_mask_bits[] = { \
   0x00, 0x0a, 0xe0, 0x00, 0x15, 0xe0, 0x00, 0x15, 0xe0, 0x00, 0x15, 0xe0, \
   0x00, 0x15, 0xe0, 0x00, 0x15, 0xe0, 0x80, 0x24, 0xe0, 0x40, 0x55, 0xe0, \
   0xa0, 0xb5, 0xe0, 0xd0, 0x75, 0xe1, 0xe8, 0xf5, 0xe2, 0xf4, 0xe4, 0xe5, \
   0x3a, 0x95, 0xeb, 0xcd, 0x75, 0xf6, 0x32, 0x95, 0xe9, 0x1c, 0x15, 0xe6, \
   0x83, 0x20, 0xf8, 0x40, 0x5b, 0xe0, 0xa0, 0xbb, 0xe0, 0x20, 0x80, 0xe0, \
   0xe0, 0xfb, 0xe0};





//----------------------------------------------------------------------------
void vtkKWFlyInteractor::CreateCursor()
{
  // We have to copy the name because we cannot convert const char* to char*
  char *name = new char[strlen(this->RenderView->GetParentWindow()->GetWidgetName())+1];
  strcpy(name, this->RenderView->GetParentWindow()->GetWidgetName());

  Tk_Window mainTkWin;
  mainTkWin = Tk_MainWindow(this->Application->GetMainInterp());
  this->RenderTkWindow = Tk_NameToWindow(this->Application->GetMainInterp(), 
                                         name, mainTkWin);
  delete [] name;
  
  this->PlaneCursor = Tk_GetCursorFromData(this->Application->GetMainInterp(), 
  				this->RenderTkWindow, (char*)plane3_bits, (char*)plane3_mask_bits, 
          21, 21, 10, 17, Tk_GetUid("white"), Tk_GetUid("black"));

  if (this->PlaneCursor == NULL)
    {
    vtkWarningMacro("Could not define cursor.");
    }
}

