/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRenderView.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkXRenderWindow.h"

int vtkKWRenderViewFoundMatch;
Bool vtkKWRenderViewPredProc(Display *vtkNotUsed(disp), XEvent *event, 
			     char *arg)
{
  if (event->type == ConfigureNotify)
    {
    vtkKWRenderViewFoundMatch = 1;
    }
  if (event->type == ButtonPress)
    {
    vtkKWRenderViewFoundMatch = 1;
    }
  if (event->type == KeyPress)
    {
    vtkKWRenderViewFoundMatch = 1;
    }
  return 0;
}
#endif



//------------------------------------------------------------------------------
vtkKWRenderView* vtkKWRenderView::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWRenderView");
  if(ret)
    {
    return (vtkKWRenderView*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWRenderView;
}


void KWRenderViewAbortCheckMethod( void *arg )
{
  vtkKWRenderView *me = (vtkKWRenderView *)arg;

  // if we are printing then do not abort
  if (me->GetPrinting())
    {
    return;
    }

  if ( me->ShouldIAbort() == 2 )
    {
    me->GetRenderWindow()->SetAbortRender(1);    
    }
}

// Return 1 to mean abort but keep trying, 2 to mean hard abort
int vtkKWRenderView::ShouldIAbort()
{
  int flag = 0;
  
#ifdef _WIN32
  MSG msg;

  // Check all four - can't get the range right in one call without
  // including events we don't want

  if (PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_NCLBUTTONDOWN,WM_NCLBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_MBUTTONDOWN,WM_MBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_RBUTTONDOWN,WM_RBUTTONDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_KEYDOWN,WM_KEYDOWN,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_WINDOWPOSCHANGING,WM_WINDOWPOSCHANGING,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_WINDOWPOSCHANGED,WM_WINDOWPOSCHANGED,PM_NOREMOVE))
    {
    flag = 2;
    }
  if (PeekMessage(&msg,NULL,WM_SIZE,WM_SIZE,PM_NOREMOVE))
    {
    flag = 2;
    }

  if ( !flag )
    {
    // Check some other events to make sure UI isn't being updated
    if (PeekMessage(&msg,NULL,WM_SYNCPAINT,WM_SYNCPAINT,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_NCPAINT,WM_NCPAINT,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_PAINT,WM_PAINT,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_ERASEBKGND,WM_ERASEBKGND,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_ACTIVATE,WM_ACTIVATE,PM_NOREMOVE))
      {
      flag = 1;
      }
    if (PeekMessage(&msg,NULL,WM_NCACTIVATE,WM_NCACTIVATE,PM_NOREMOVE))
      {
      flag = 1;
      }
    }
  
#else
  XEvent report;
  
  vtkKWRenderViewFoundMatch = 0;
  Display *dpy = ((vtkXRenderWindow*)this->GetRenderWindow())->GetDisplayId();
  XSync(dpy,0);
  XCheckIfEvent(dpy, &report, vtkKWRenderViewPredProc, NULL);
  XSync(dpy,0);
  if (vtkKWRenderViewFoundMatch)
    {
    flag = 2;
    }
#endif
  
  return flag;
}


void KWRenderView_IdleRender( ClientData arg )
{
  vtkKWRenderView *me = (vtkKWRenderView *)arg;

  me->IdleRenderCallback();
}

void vtkKWRenderView::IdleRenderCallback()
{  
  int rescheduleDelay;
  int needToRender = 0;
  int abortFlag;
  double elapsedTime;
    
  this->RenderTimer->StopTimer();
  
  elapsedTime = this->RenderTimer->GetElapsedTime();
  abortFlag = this->ShouldIAbort();
  
  // Has enough time passed? Is there anything pending that will
  // abort this render?
  if ( elapsedTime > 0.1 && abortFlag == 0 )
    {
    for (int i=0; i < this->GetNumberOfStillUpdates(); i++)
      {
      this->RenderWindow->SetDesiredUpdateRate(this->GetStillUpdateRate(i));
      this->RenderWindow->Render();
      if ( this->RenderWindow->GetAbortRender() ||
           ( this->MultiPassStillAbortCheckMethod &&
             this->MultiPassStillAbortCheckMethod
             ( this->MultiPassStillAbortCheckMethodArg ) ) )
        {
        break;
        }
      }
    }
  else
    {
    if ( abortFlag == 1 )
      {
      needToRender = 1;
      rescheduleDelay = 1000;
      } 
    else if ( elapsedTime <= 0.1 )
      {  
      needToRender = 1;
      rescheduleDelay = 100;
      }
    }

  // If we still need to render, reschedule this callback
  if ( needToRender )
    {
    this->TimerToken = Tcl_CreateTimerHandler(rescheduleDelay, 
                                              KWRenderView_IdleRender, 
                                              (ClientData)this);
    }
  else
    {
    this->TimerToken = NULL;
    }
}

int vtkKWRenderViewCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

vtkKWRenderView::vtkKWRenderView()
{
  this->RenderWindow = vtkRenderWindow::New();
  this->RenderWindow->SetDesiredUpdateRate(1.0);
  this->RenderWindow->
    SetAbortCheckMethod(KWRenderViewAbortCheckMethod, (void *)this);
  this->InExpose = 0;
  this->CurrentLight    = NULL;
  this->Renderer = vtkRenderer::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  this->CurrentCamera = this->Renderer->GetActiveCamera();
  this->CommandFunction = vtkKWRenderViewCommand;
  this->InRender = 0;
  this->InMotion = 0;
  this->RenderTimer = vtkTimerLog::New();  

  this->CameraFrame = vtkKWLabeledFrame::New();
  this->CameraFrame->SetParent( this->GeneralProperties );
  this->CameraTopFrame = vtkKWWidget::New();
  this->CameraTopFrame->SetParent( this->CameraFrame->GetFrame() );
  this->CameraBottomFrame = vtkKWWidget::New();
  this->CameraBottomFrame->SetParent( this->CameraFrame->GetFrame() );
    
  this->CameraPlusXButton = vtkKWPushButton::New();
  this->CameraPlusXButton->SetParent(this->CameraTopFrame);
  this->CameraPlusYButton = vtkKWPushButton::New();
  this->CameraPlusYButton->SetParent(this->CameraTopFrame);
  this->CameraPlusZButton = vtkKWPushButton::New();
  this->CameraPlusZButton->SetParent(this->CameraTopFrame);
  this->CameraMinusXButton = vtkKWPushButton::New();
  this->CameraMinusXButton->SetParent(this->CameraBottomFrame);
  this->CameraMinusYButton = vtkKWPushButton::New();
  this->CameraMinusYButton->SetParent(this->CameraBottomFrame);
  this->CameraMinusZButton = vtkKWPushButton::New();
  this->CameraMinusZButton->SetParent(this->CameraBottomFrame);

  this->TimerToken = NULL;
  
}

vtkKWRenderView::~vtkKWRenderView()
{
  this->RenderWindow->Delete();
  this->Renderer->Delete();
  this->CameraFrame->Delete();
  this->CameraTopFrame->Delete();
  this->CameraBottomFrame->Delete();
  this->CameraPlusXButton->Delete();
  this->CameraPlusYButton->Delete();
  this->CameraPlusZButton->Delete();
  this->CameraMinusXButton->Delete();
  this->CameraMinusYButton->Delete();
  this->CameraMinusZButton->Delete();
  this->RenderTimer->Delete();
  
  if ( this->TimerToken )
    {
    Tcl_DeleteTimerHandler( this->TimerToken );
    this->TimerToken = NULL;
    }
  
}

void vtkKWRenderView::SetupMemoryRendering(int x, int y, void *cd) 
{
#ifdef _WIN32
  if (!cd)
    {
    cd = this->RenderWindow->GetGenericContext();
    }
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->SetupMemoryRendering(x,y,(HDC)cd);
#endif
}

void vtkKWRenderView::ResumeScreenRendering() 
{
#ifdef _WIN32
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->ResumeScreenRendering();
#endif
}

void *vtkKWRenderView::GetMemoryDC()
{
#ifdef _WIN32	
  return (void *)vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->GetMemoryDC();
#endif
  return NULL;
}

unsigned char *vtkKWRenderView::GetMemoryData()
{
#ifdef _WIN32	
  return vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->GetMemoryData();
#endif
  return NULL;
}

void vtkKWRenderView::Create(vtkKWApplication *app, const char *args)
{
  char *local;
  const char *wname;

  local = new char [strlen(args)+100];

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  this->SetApplication(app);
  Tcl_Interp *interp = this->Application->GetMainInterp();

  // create the frame
  wname = this->GetWidgetName();
  this->Script("frame %s -bd 0 %s",wname,args);
  //this->Script("pack %s -expand yes -fill both",wname);
  
  // create the label
  this->Frame->Create(app,"frame","-bd 3 -relief ridge");
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->Frame->GetWidgetName());
  this->Frame2->Create(app,"frame","-bd 0 -bg #888");
  this->Script("pack %s -fill x -side top -anchor nw",
               this->Frame2->GetWidgetName());
  this->Label->Create(app,"label","-fg #fff -text {3D View}");
  this->Script("pack %s  -side left -anchor w",this->Label->GetWidgetName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Label->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Frame2->GetWidgetName(), this->GetTclName());
  
  // add the -rw argument
  sprintf(local,"%s -rw Addr=%p",args,this->RenderWindow);
  this->Script("vtkTkRenderWidget %s %s",
               this->VTKWidget->GetWidgetName(),local);
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->VTKWidget->GetWidgetName());


  this->RenderWindow->Render();
  delete [] local;
}

void vtkKWRenderView::CreateViewProperties()
{
  vtkKWApplication *app = this->Application;

  this->vtkKWView::CreateViewProperties();

  this->CameraFrame->Create( app );
  this->CameraFrame->SetLabel( "Viewing Directions" );
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->CameraFrame->GetWidgetName());

  this->CameraTopFrame->Create( app, "frame", "" );
  this->CameraBottomFrame->Create( app, "frame", "" );
  this->Script("pack %s %s -side top -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->CameraTopFrame->GetWidgetName(),
               this->CameraBottomFrame->GetWidgetName());

  this->CameraPlusXButton->Create( app, "-text {+X}" );
  this->CameraPlusXButton->SetCommand( this, "SetStandardCameraView 0" );
  this->CameraPlusYButton->Create( app, "-text {+Y}" );
  this->CameraPlusYButton->SetCommand( this, "SetStandardCameraView 1" );
  this->CameraPlusZButton->Create( app, "-text {+Z}" );
  this->CameraPlusZButton->SetCommand( this, "SetStandardCameraView 2" );
  this->Script("pack %s %s %s -side left -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->CameraPlusXButton->GetWidgetName(),
               this->CameraPlusYButton->GetWidgetName(),
               this->CameraPlusZButton->GetWidgetName() );

  this->CameraMinusXButton->Create( app, "-text {-X}" );
  this->CameraMinusXButton->SetCommand( this, "SetStandardCameraView 3" );
  this->CameraMinusYButton->Create( app, "-text {-Y}" );
  this->CameraMinusYButton->SetCommand( this, "SetStandardCameraView 4" );
  this->CameraMinusZButton->Create( app, "-text {-Z}" );
  this->CameraMinusZButton->SetCommand( this, "SetStandardCameraView 5" );
  this->Script("pack %s %s %s -side left -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->CameraMinusXButton->GetWidgetName(),
               this->CameraMinusYButton->GetWidgetName(),
               this->CameraMinusZButton->GetWidgetName() );

}

void vtkKWRenderView::SetStandardCameraView( int type )
{
  vtkCamera *c;
  float     v[3];

  c = this->Renderer->GetActiveCamera();
  c->GetFocalPoint(v);

  switch ( type ) 
    {
    case 0:
      c->SetPosition( v[0]-1, v[1], v[2] );
      c->SetViewUp( 0, 1, 0 );
      break;
    case 1:
      c->SetPosition( v[0], v[1]-1, v[2] );
      c->SetViewUp( 0, 0, 1 );
      break;
    case 2:
      c->SetPosition( v[0], v[1], v[2]-1 );
      c->SetViewUp( 0, 1, 0 );
      break;
    case 3:
      c->SetPosition( v[0]+1, v[1], v[2] );
      c->SetViewUp( 0, 1, 0 );
      break;
    case 4:
      c->SetPosition( v[0], v[1]+1, v[2] );
      c->SetViewUp( 0, 0, 1 );
      break;
    case 5:
      c->SetPosition( v[0], v[1], v[2]+1 );
      c->SetViewUp( 0, 1, 0 );
      break;
    }
  this->ResetCamera();
  this->RenderWindow->Render();
}

void vtkKWRenderView::ResetCamera()
{
  float bounds[6];
  float center[3];
  float distance;
  float width;
  double vn[3], *vup;
  
  this->Renderer->ComputeVisiblePropBounds( bounds );
  if ( bounds[0] == VTK_LARGE_FLOAT )
    {
    vtkDebugMacro( << "Cannot reset camera!" );
    return;
    }

  this->CurrentCamera = this->Renderer->GetActiveCamera();
  if ( this->CurrentCamera != NULL )
    {
    this->CurrentCamera->GetViewPlaneNormal(vn);
    }
  else
    {
    vtkErrorMacro(<< "Trying to reset non-existant camera");
    return;
    }

  center[0] = (bounds[0] + bounds[1])/2.0;
  center[1] = (bounds[2] + bounds[3])/2.0;
  center[2] = (bounds[4] + bounds[5])/2.0;

  width = bounds[3] - bounds[2];
  if (width < (bounds[1] - bounds[0]))
    {
    width = bounds[1] - bounds[0];
    }
  if (width < (bounds[5] - bounds[4]))
    {
    width = bounds[5] - bounds[4];
    }
  distance = 
    width/tan(this->CurrentCamera->GetViewAngle()*vtkMath::Pi()/360.0);

  // check view-up vector against view plane normal
  vup = this->CurrentCamera->GetViewUp();
  if ( fabs(vtkMath::Dot(vup,vn)) > 0.999 )
    {
    vtkWarningMacro(<<"Resetting view-up since view plane normal is parallel");
    this->CurrentCamera->SetViewUp(-vup[2], vup[0], vup[1]);
    }

  // update the camera
  this->CurrentCamera->SetFocalPoint(center[0],center[1],center[2]);
  this->CurrentCamera->SetPosition(center[0]+distance*vn[0],
                                  center[1]+distance*vn[1],
                                  center[2]+distance*vn[2]);

  this->Renderer->ResetCameraClippingRange( bounds );

  // setup default parallel scale
  this->CurrentCamera->SetParallelScale(0.6*width);
}

void vtkKWRenderView::SetBackgroundColor( float r, float g, float b )
{
  this->Renderer->SetBackground( r, g, b );
  this->Render();
}

// we have to handle all pending expose events otherwise they queue up.
void vtkKWRenderView::Exposed()
{
  if (this->InExpose) return;
  this->InExpose = 1;
  this->Script("update");
  this->Render();
  this->InExpose = 0;
}

void vtkKWRenderView::AKeyPress(char key, int x, int y)
{
  this->MakeSelected();
  switch (key)
    {
    case 'w': this->Wireframe(); break;
    case 's': this->Surface(); break;
    case 'r': this->Reset(); break;
    }
}

void vtkKWRenderView::UpdateRenderer(int x, int y) 
{
  memcpy(this->Center,this->Renderer->GetCenter(),sizeof(float)*2);
  this->CurrentCamera = this->Renderer->GetActiveCamera();
  vtkLightCollection *lights = this->Renderer->GetLights();
  lights->InitTraversal(); 
  this->CurrentLight = lights->GetNextItem();
   
  this->LastPosition[0] = x;
  this->LastPosition[1] = y;
}

void vtkKWRenderView::Render()
{
  if ( this->InRender )
    {
    return;
    }
  
  this->InRender = 1;

  if (this->CurrentLight)
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }

  if ( this->RenderMode == VTK_KW_INTERACTIVE_RENDER )
    {
    if ( this->TimerToken )
      {
      Tcl_DeleteTimerHandler( this->TimerToken );
      this->TimerToken = NULL;
      }
    
    this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
    this->RenderWindow->Render();
    if ( this->RenderWindow->GetAbortRender() )
      {
      this->InMotion = 0;
      }
    }
  else if ( this->RenderMode == VTK_KW_STILL_RENDER )
    {
    // If this is a still render do it as an timer callback
    // Start the timer here. Then, if we don't already have a timer call
    // going, start one. The timer callback will render only if it has been
    // at least some length of time since the last time a render request was
    // made (we keep track of this with the timerlog - we re-start it each 
    // time a render request is made. The timer will keep rescheduling itself
    // until it can do its render successfully.
    this->RenderTimer->StartTimer();
    if ( !this->TimerToken )
      {
      this->TimerToken = Tcl_CreateTimerHandler(100, 
                                                KWRenderView_IdleRender, 
                                                (ClientData)this);
      }
    }
  else if ( this->RenderMode == VTK_KW_SINGLE_RENDER )
    {
    if ( this->TimerToken )
      {
      Tcl_DeleteTimerHandler( this->TimerToken );
      this->TimerToken = NULL;
      }

    // if we are printing pick a good update rate
    if (this->Printing)
      {
      // default to longest update rate
      this->RenderWindow->SetDesiredUpdateRate
        (this->StillUpdateRates[this->NumberOfStillUpdates-1]);
      }
    this->RenderWindow->Render();
    this->InMotion = 0;
    }
  
  this->InRender = 0;
}

void vtkKWRenderView::Enter(int x, int y)
{
  this->Script("focus %s",this->VTKWidget->GetWidgetName());
  this->UpdateRenderer(x,y);
}

void vtkKWRenderView::StartMotion(int x, int y)
{
  this->UpdateRenderer(x,y);
  this->RenderMode = VTK_KW_INTERACTIVE_RENDER;
  this->InMotion = 1;
}

void vtkKWRenderView::EndMotion(int x, int y)
{
  this->InMotion = 0;
  this->RenderMode = VTK_KW_STILL_RENDER;
  this->Render();
}

void vtkKWRenderView::Rotate(int x, int y)
{ 
  this->CurrentCamera->Azimuth(this->LastPosition[0] - x);
  this->CurrentCamera->Elevation(y - this->LastPosition[1]);
  this->CurrentCamera->OrthogonalizeViewUp();
  this->Renderer->ResetCameraClippingRange();

  this->LastPosition[0] = x;
  this->LastPosition[1] = y;
  this->Render();
}

void vtkKWRenderView::Pan(int x, int y)
{
  vtkCamera *cam = this->CurrentCamera;
  vtkRenderer *ren = this->Renderer;
  int *last = this->LastPosition;
  
  if (!ren) 
    {
    return;
    }
  
  float FPoint[3];
  cam->GetFocalPoint(FPoint);
  float PPoint[3];
  cam->GetPosition(PPoint);

  ren->SetWorldPoint(FPoint[0], FPoint[1], FPoint[2], 1.0);
  ren->WorldToDisplay();
  float *DPoint = ren->GetDisplayPoint();
  float focalDepth = DPoint[2];

  float APoint0 = ren->GetCenter()[0] + (x - last[0]);
  float APoint1 = ren->GetCenter()[1] - (y - last[1]);

  ren->SetDisplayPoint(APoint0,APoint1,focalDepth);
  ren->DisplayToWorld();
  float *RPoint = ren->GetWorldPoint();
  if (RPoint[3] != 0.0)
    {
    RPoint[0] = RPoint[0] / RPoint[3];
    RPoint[1] = RPoint[1] / RPoint[3];
    RPoint[2] = RPoint[2] / RPoint[3];
    }

  cam->SetFocalPoint((FPoint[0] - RPoint[0])/2.0 + FPoint[0],
		     (FPoint[1] - RPoint[1])/2.0 + FPoint[1],
		     (FPoint[2] - RPoint[2])/2.0 + FPoint[2]);

  cam->SetPosition((FPoint[0] - RPoint[0])/2.0 + PPoint[0],
		(FPoint[1] - RPoint[1])/2.0 + PPoint[1],
		(FPoint[2] - RPoint[2])/2.0 + PPoint[2]);
  
  last[0] = x;
  last[1] = y;
  this->Render();
}

void vtkKWRenderView::Zoom(int x, int y)
{
  vtkCamera *cam = this->CurrentCamera;

  float zoomFactor = pow(1.02,(0.5*(y - this->LastPosition[1])));
  
  if (cam->GetParallelProjection()) 
    {
    float parallelScale = cam->GetParallelScale()* zoomFactor;
    cam->SetParallelScale(parallelScale);
    } 
  else 
    {
    cam->Dolly(zoomFactor);
    }

  this->Renderer->ResetCameraClippingRange();

  this->LastPosition[0] = x;
  this->LastPosition[1] = y;
  this->Render();
}

void vtkKWRenderView::Reset()
{
  this->Renderer->GetActiveCamera()->SetPosition(0,0,1);
  this->Renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  this->Renderer->GetActiveCamera()->SetViewUp(0,1,0);
  this->ResetCamera();
  this->Render();
}

void vtkKWRenderView::Wireframe()
{
  vtkActorCollection *actors = this->Renderer->GetActors();

  actors->InitTraversal();
  vtkActor *actor = actors->GetNextItem();
  while (actor) 
    {
    actor->GetProperty()->SetRepresentationToWireframe();
    actor = actors->GetNextItem();
    }
  this->Render();
}

void vtkKWRenderView::Surface()
{
  vtkActorCollection *actors = this->Renderer->GetActors();

  actors->InitTraversal();
  vtkActor *actor = actors->GetNextItem();
  while (actor) 
    {
    actor->GetProperty()->SetRepresentationToSurface();
    actor = actors->GetNextItem();
    }
  this->Render();
}

// Description:
// Chaining method to serialize an object and its superclasses.
void vtkKWRenderView::SerializeSelf(ostream& os, vtkIndent indent)
{
  // invoke superclass
  this->vtkKWView::SerializeSelf(os,indent);

  // write out the camera
  vtkCamera *cam = this->Renderer->GetActiveCamera();
  os << indent << "CameraPosition " 
     << cam->GetPosition()[0] << " "  
     << cam->GetPosition()[1] << " "  
     << cam->GetPosition()[2] << endl;
  os << indent << "CameraFocalPoint " 
     << cam->GetFocalPoint()[0] << " "  
     << cam->GetFocalPoint()[1] << " "  
     << cam->GetFocalPoint()[2] << endl;
  os << indent << "CameraViewUp " 
     << cam->GetViewUp()[0] << " " 
     << cam->GetViewUp()[1] << " "  
     << cam->GetViewUp()[2] << endl;
  os << indent << "CameraClippingRange " 
     << cam->GetClippingRange()[0] << " " 
     << cam->GetClippingRange()[1] << endl;
  os << indent << "CameraViewAngle " << cam->GetViewAngle() << endl;
  os << indent << "CameraParallelScale " << cam->GetParallelScale() << endl;
}

void vtkKWRenderView::SerializeToken(istream& is, const char token[1024])
{
  float a,b,c;
  
  // if this file is from an old version then set the colors to the default
  if (!this->VersionsLoaded)
    {
    float c[3];  c[0] = 0.0;  c[1] = 0.0;  c[2] = 0.0;
    this->BackgroundColor->SetColor( c );
    this->SetBackgroundColor(0,0,0);
    }

  if (!strcmp(token,"CameraPosition"))
    {
    is >> a >> b >> c;
    this->Renderer->GetActiveCamera()->SetPosition(a,b,c);
    this->Renderer->GetActiveCamera()->ComputeViewPlaneNormal();
    return;
    }
  if (!strcmp(token,"CameraFocalPoint"))
    {
    is >> a >> b >> c;
    this->Renderer->GetActiveCamera()->SetFocalPoint(a,b,c);
    this->Renderer->GetActiveCamera()->ComputeViewPlaneNormal();
    return;
    }
  if (!strcmp(token,"CameraViewUp"))
    {
    is >> a >> b >> c;
    this->Renderer->GetActiveCamera()->SetViewUp(a,b,c);
    return;
    }
  if (!strcmp(token,"CameraClippingRange"))
    {
    is >> a >> b;
    this->Renderer->GetActiveCamera()->SetClippingRange(a,b);
    return;
    }
  if (!strcmp(token,"CameraViewAngle"))
    {
    is >> a;
    this->Renderer->GetActiveCamera()->SetViewAngle(a);
    return;
    }
  if (!strcmp(token,"CameraParallelScale"))
    {
    is >> a;
    this->Renderer->GetActiveCamera()->SetParallelScale(a);
    return;
    }
  
  vtkKWView::SerializeToken(is,token);
}

void vtkKWRenderView::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWView::SerializeRevision(os,indent);
  os << indent << "vtkKWRenderView ";
  this->ExtractRevision(os,"$Revision: 1.11 $");
}
