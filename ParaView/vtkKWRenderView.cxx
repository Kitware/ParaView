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

#include "vtkTimerLog.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
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

#ifdef _WIN32
  MSG msg;

  // Check all four - can't get the range right in one call without
  // including events we don't want

  if (PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDOWN,PM_NOREMOVE))
    {
      me->GetRenderWindow()->SetAbortRender(1);
    }
  if (PeekMessage(&msg,NULL,WM_NCLBUTTONDOWN,WM_NCLBUTTONDOWN,PM_NOREMOVE))
    {
      me->GetRenderWindow()->SetAbortRender(1);
    }
  if (PeekMessage(&msg,NULL,WM_MBUTTONDOWN,WM_MBUTTONDOWN,PM_NOREMOVE))
    {
      me->GetRenderWindow()->SetAbortRender(1);
    }
  if (PeekMessage(&msg,NULL,WM_RBUTTONDOWN,WM_RBUTTONDOWN,PM_NOREMOVE))
    {
      me->GetRenderWindow()->SetAbortRender(1);
    }
  if (PeekMessage(&msg,NULL,WM_KEYDOWN,WM_KEYDOWN,PM_NOREMOVE))
    {
      me->GetRenderWindow()->SetAbortRender(1);
    }
#else
  XEvent report;
  
  vtkKWRenderViewFoundMatch = 0;
  Display *dpy = ((vtkXRenderWindow*)me->GetRenderWindow())->GetDisplayId();
  XSync(dpy,false);
  XCheckIfEvent(dpy, &report, vtkKWRenderViewPredProc, NULL);
  XSync(dpy,false);
  if (vtkKWRenderViewFoundMatch)
    {
    me->GetRenderWindow()->SetAbortRender(1);
    }
#endif
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

  this->GeneralProperties = vtkKWWidget::New();

  this->BackgroundFrame = vtkKWLabeledFrame::New();
  this->BackgroundFrame->SetParent( this->GeneralProperties );
  this->BackgroundColor = vtkKWChangeColorButton::New();
  this->BackgroundColor->SetParent( this->BackgroundFrame->GetFrame() );

  this->PageMenu = vtkKWMenu::New();
}

vtkKWRenderView::~vtkKWRenderView()
{
  this->RenderWindow->Delete();
  this->Renderer->Delete();
  this->GeneralProperties->Delete();
  this->BackgroundFrame->Delete();
  this->BackgroundColor->Delete();
  this->PageMenu->Delete();
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
#else
  if (this->RenderWindow->IsA("vtkMesaRenderWindow"))
    {
    vtkMesaRenderWindow::
      SafeDownCast(this->RenderWindow)->SetOffScreenRendering(1);
    } 
#endif
}

void vtkKWRenderView::ResumeScreenRendering() 
{
#ifdef _WIN32
  vtkWin32OpenGLRenderWindow::
    SafeDownCast(this->RenderWindow)->ResumeScreenRendering();
#else
  if (this->RenderWindow->IsA("vtkMesaRenderWindow"))
    {
    vtkMesaRenderWindow::
      SafeDownCast(this->RenderWindow)->SetOffScreenRendering(0);
    } 
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

void vtkKWRenderView::Create(vtkKWApplication *app, char *args)
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
  delete local;
}

void vtkKWRenderView::CreateViewProperties()
{
  vtkKWApplication *app = this->Application;

  this->vtkKWView::CreateViewProperties();

  this->Notebook->AddPage("General");
  
  this->GeneralProperties->SetParent(this->Notebook->GetFrame("General"));
  this->GeneralProperties->Create(app,"frame","");
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->Script("pack %s -pady 2 -fill both -expand yes -anchor n",
               this->GeneralProperties->GetWidgetName());  

  this->BackgroundFrame->Create( app );
  this->BackgroundFrame->SetLabel("Background");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->BackgroundFrame->GetWidgetName());

  float c[3];  c[0] = 0.0;  c[1] = 0.0;  c[2] = 0.0;
  this->BackgroundColor->SetColor( c );
  this->BackgroundColor->Create( app, "" );
  this->BackgroundColor->SetCommand( this, "SetBackgroundColor" );
  this->Script("pack %s -side top -padx 15 -pady 4 -expand 1 -fill x",
               this->BackgroundColor->GetWidgetName());
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
  
  vtkTimerLog  *timer = vtkTimerLog::New();
  timer->StartTimer();
  cerr << "Start View Render\n";
  
  this->InRender = 1;

  if (this->CurrentLight)
    {
    this->CurrentLight->SetPosition(this->CurrentCamera->GetPosition());
    this->CurrentLight->SetFocalPoint(this->CurrentCamera->GetFocalPoint());
    }

  if ( this->RenderMode == VTK_KW_INTERACTIVE_RENDER )
    {
    this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
    this->RenderWindow->Render();
    if ( this->RenderWindow->GetAbortRender() )
      {
      this->InMotion = 0;
      }
    }
  else if ( this->RenderMode == VTK_KW_STILL_RENDER )
    {
    for (int i=0; i < this->NumberOfStillUpdates; i++)
      {
      this->RenderWindow->SetDesiredUpdateRate(this->StillUpdateRates[i]);
      this->RenderWindow->Render();
      if ( this->RenderWindow->GetAbortRender() ||
	   ( this->MultiPassStillAbortCheckMethod &&
	     this->MultiPassStillAbortCheckMethod( this->MultiPassStillAbortCheckMethodArg ) ) )
	{
	this->InMotion = 0;
	break;
	}
      }
    }
  else if ( this->RenderMode == VTK_KW_SINGLE_RENDER )
    {
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
  
  timer->StopTimer();
  cerr << "End View Render: " << timer->GetElapsedTime() << endl;
  timer->Delete();
  
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
  this->Renderer->GetActiveCamera()->ComputeViewPlaneNormal();
  this->Renderer->GetActiveCamera()->SetViewUp(0,1,0);
  this->Renderer->ResetCamera();
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
  os << indent << "BackgroundColor ";
  this->BackgroundColor->Serialize(os,indent);
}

void vtkKWRenderView::SerializeToken(istream& is, const char token[1024])
{
  float a,b,c;
  
  // if this file is from an old version then set the colors to the default
  // we cheat and look for the CornerButton tag which is no longer used
  // but in vvs file prior to versioning
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
  if (!strcmp(token,"BackgroundColor"))
    {
    this->BackgroundColor->Serialize(is);
    return;
    }
  
  vtkKWView::SerializeToken(is,token);
}

void vtkKWRenderView::OnPrint1() 
{
  this->PrintTargetDPI = 100;
}
void vtkKWRenderView::OnPrint2() 
{
  this->PrintTargetDPI = 150;
}
void vtkKWRenderView::OnPrint3() 
{
  this->PrintTargetDPI = 300;
}

void vtkKWRenderView::Select(vtkKWWindow *pw)
{
  if (!this->PageMenu->GetParent())
    {
    // add render quality setting
    this->PageMenu->SetParent(pw->GetMenuFile());
    this->PageMenu->Create(this->Application,"-tearoff 0");
    

    char* rbv = 
      this->PageMenu->CreateRadioButtonVariable(this,"PageSetup");
    // now add our own menu options 
    this->Script( "set %s 0", rbv );
    this->PageMenu->AddRadioButton(0,"100 DPI",rbv,this,"OnPrint1");
    this->PageMenu->AddRadioButton(1,"150 DPI",rbv,this,"OnPrint2");
    this->PageMenu->AddRadioButton(2,"300 DPI",rbv,this,"OnPrint3");
    delete [] rbv;
    }
  
  // add the Print option
#ifdef _WIN32
  pw->GetMenuFile()->InsertCascade(pw->GetFileMenuIndex(),
                                     "Page Setup", this->PageMenu,0);
#endif

  this->vtkKWView::Select(pw);
}



void vtkKWRenderView::Deselect(vtkKWWindow *pw)
{
#ifdef _WIN32
  pw->GetMenuFile()->DeleteMenuItem("Page Setup");
#endif  
  this->vtkKWView::Deselect(pw);
}

void vtkKWRenderView::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWView::SerializeRevision(os,indent);
  os << indent << "vtkKWRenderView ";
  this->ExtractRevision(os,"$Revision: 1.7 $");
}
