/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderView.cxx
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

#include "vtkToolkits.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"

#include "vtkPVRenderView.h"
#include "vtkPVApplication.h"
#include "vtkMultiProcessController.h"
#include "vtkDummyRenderWindowInteractor.h"
#include "vtkObjectFactory.h"

#include "vtkKWEventNotifier.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#if VTK_USE_MESA
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#endif
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#endif

#include "vtkTimerLog.h"
#include "vtkPVActorComposite.h"


//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVRenderView::New()
{
  return new vtkPVRenderView();
}


int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  this->CommandFunction = vtkPVRenderViewCommand;
  this->InteractorStyle = NULL;
  this->Interactor = vtkDummyRenderWindowInteractor::New();
  
  this->Interactive = 0;
  
  this->RenderWindow->SetDesiredUpdateRate(1.0);  
}

//----------------------------------------------------------------------------
void vtkPVRenderView::CreateRenderObjects(vtkPVApplication *pvApp)
{
  // Get rid of renderer created by the superclass
  this->Renderer->Delete();
  this->Renderer = (vtkRenderer*)pvApp->MakeTclObject("vtkRenderer", "Ren1");
  this->RendererTclName = NULL;
  this->SetRendererTclName("Ren1");
  
  // Get rid of render window created by the superclass
  this->RenderWindow->Delete();
  this->RenderWindow = (vtkRenderWindow*)pvApp->MakeTclObject("vtkRenderWindow", "RenWin1");
  this->RenderWindowTclName = NULL;
  this->SetRenderWindowTclName("RenWin1");
  
  // Create the compositer.
  this->Composite = (vtkTreeComposite*)pvApp->MakeTclObject("vtkTreeComposite", "TreeComp1");
  this->CompositeTclName = NULL;
  this->SetCompositeTclName("TreeComp1");

  pvApp->BroadcastScript("%s AddRenderer %s", this->RenderWindowTclName,
			 this->RendererTclName);
  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
			 this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);
  pvApp->BroadcastScript("%s InitializeOffScreen", this->CompositeTclName);

  // The only call that should not be a broadcast is render.
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  
  this->SetInteractorStyle(NULL);

  // We are having problems with renderWindow being deleted after the RenderWidget.
  this->Interactor->SetRenderWindow(NULL);

  this->Interactor->Delete();
  this->Interactor = NULL;
  
  pvApp->BroadcastScript("%s Delete", this->RendererTclName);
  this->SetRendererTclName(NULL);
  this->Renderer = NULL;
  
  pvApp->BroadcastScript("%s Delete", this->RenderWindowTclName);
  this->SetRenderWindowTclName(NULL);
  this->RenderWindow = NULL;
  
  pvApp->BroadcastScript("%s Delete", this->CompositeTclName);
  this->SetCompositeTclName(NULL);
  this->Composite = NULL;
}

//----------------------------------------------------------------------------
// Here we are going to change only the satellite procs.
void vtkPVRenderView::OffScreenRenderingOn()
{
#ifdef VTK_USE_MESA  
  int i, num;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller;
  
  controller = pvApp->GetController();
  num = 1;
  if (controller)
    {  
    num = controller->GetNumberOfProcesses();
    }
  
  for (i = 1; i < num; ++i)
    {
    pvApp->RemoteScript(i, "%s Delete", this->RendererTclName);
    pvApp->RemoteScript(i, "%s Delete", this->RenderWindowTclName);
    pvApp->RemoteScript(i, "vtkMesaRenderer %s", this->RendererTclName);
    pvApp->RemoteScript(i, "vtkMesaRenderWindow %s", this->RenderWindowTclName);
    pvApp->RemoteScript(i, "%s AddRenderer %s", 
			this->RenderWindowTclName, this->RendererTclName);
    pvApp->RemoteScript(i, "%s SetRenderWindow %s", 
			this->CompositeTclName, this->RendererTclName);
    }
  
#endif
}
  


//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderView::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkPVRenderView::GetRenderWindow()
{
  return this->RenderWindow;
}


//----------------------------------------------------------------------------
void vtkPVRenderView::SetInteractorStyle(vtkInteractorStyle *style)
{
  if (this->Interactor)
    {
    this->Interactor->SetRenderWindow(this->GetRenderer()->GetRenderWindow());
    this->Interactor->SetInteractorStyle(style);
    }
  if (this->InteractorStyle)
    {
    this->InteractorStyle->UnRegister(this);
    this->InteractorStyle = NULL;
    }
  if (style)
    {
    this->InteractorStyle = style;
    style->Register(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Create(vtkKWApplication *app, const char *args)
{
  char *local;
  const char *wname;

  local = new char [strlen(args)+100];

  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  
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
  this->Label->Create(app,"label","-fg #fff -text {3D View} -bd 0");
  this->Script("pack %s  -side left -anchor w",this->Label->GetWidgetName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Label->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <Any-ButtonPress> {%s MakeSelected}",
               this->Frame2->GetWidgetName(), this->GetTclName());

  // Create the control frame - only pack it if support option enabled
  this->ControlFrame->Create(app,"frame","-bd 0");
  if (this->SupportControlFrame)
    {
    this->Script("pack %s -expand no -fill x -side top -anchor nw",
                 this->ControlFrame->GetWidgetName());
    }
  
  // add the -rw argument
  sprintf(local,"%s -rw Addr=%p",args,this->RenderWindow);
  this->Script("vtkTkRenderWidget %s %s",
               this->VTKWidget->GetWidgetName(),local);
  this->Script("pack %s -expand yes -fill both -side top -anchor nw",
               this->VTKWidget->GetWidgetName());


  // Styles need motion events.
  this->Script("bind %s <Motion> {%s MotionCallback %%x %%y}", 
               this->VTKWidget->GetWidgetName(), this->GetTclName());
  
  // Expose.
  this->Script("bind %s <Expose> {%s Exposed}", this->GetTclName(),
	       this->GetTclName());
  
  this->RenderWindow->Render();
  delete [] local;
}

//----------------------------------------------------------------------------
// a litle more complex than just "bind $widget <Expose> {%W Render}"
// we have to handle all pending expose events otherwise they que up.
void vtkPVRenderView::Exposed()
{
  if (this->InExpose) return;
  this->InExpose = 1;
  this->Script("update");
  this->Render();
  this->InExpose = 0;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Update()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ComputeVisiblePropBounds(float bounds[6])
{
  float tmp[6];
  int id, num;
  
  this->GetRenderer()->ComputeVisiblePropBounds(bounds);

}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCamera()
{
  this->GetRenderer()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  this->GetRenderer()->ResetCameraClippingRange();
}

//----------------------------------------------------------------------------
// Called by a binding, so I must flip y.
void vtkPVRenderView::MotionCallback(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderView::AButtonPress(int num, int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  this->Interactive = 1;
  this->Application->GetEventNotifier()->
    InvokeCallbacks( "InteractiveRenderStart",  
  		     this->GetWindow(), "" );
  
  if (this->InteractorStyle)
    {
    if (num == 1)
      {
      this->InteractorStyle->OnLeftButtonDown(0, 0, x, y);
      }
    if (num == 2)
      {
      this->InteractorStyle->OnMiddleButtonDown(0, 0, x, y);
      }
    if (num == 3)
      {
      this->InteractorStyle->OnRightButtonDown(0, 0, x, y);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AButtonRelease(int num, int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  this->Interactive = 0;
  this->Application->GetEventNotifier()->
    InvokeCallbacks( "InteractiveRenderEnd",  
  		     this->GetWindow(), "" );
  
  if (this->InteractorStyle)
    {
    if (num == 1)
      {
      this->InteractorStyle->OnLeftButtonUp(0, 0, x, y);
      }
    if (num == 2)
      {
      this->InteractorStyle->OnMiddleButtonUp(0, 0, x, y);
      }
    if (num == 3)
      {
      this->InteractorStyle->OnRightButtonUp(0, 0, x, y);
      }
    }
  this->Render();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button1Motion(int x, int y)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    // Make sure all pipelines update.
    // Reset camera causes an update on process 0.
    this->Update();
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button2Motion(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button3Motion(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnMouseMove(0, 0, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AKeyPress(char key, int x, int y)
{
  x = y;

  if (this->InteractorStyle)
    {
    this->InteractorStyle->OnChar(0, 0, key, 1);
    }
}



//----------------------------------------------------------------------------
vtkPVApplication* vtkPVRenderView::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVRenderView::AddComposite(vtkKWComposite *c)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVActorComposite *pvc = vtkPVActorComposite::SafeDownCast(c);
  
  if (pvc == NULL)
    {
    // Default
    this->vtkKWView::AddComposite(c);
    return;
    }
  
  c->SetView(this);
  // never allow a composite to be added twice
  if (this->Composites->IsItemPresent(c))
    {
    return;
    }
  this->Composites->AddItem(c);
  if (pvc->GetActorTclName() != NULL)
    {
    pvApp->BroadcastScript("%s AddProp %s", this->RendererTclName,
			   pvc->GetActorTclName());
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::RemoveComposite(vtkKWComposite *c)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVActorComposite *pvc = vtkPVActorComposite::SafeDownCast(c);

  if (pvc == NULL)
    {
    // Default
    this->vtkKWView::RemoveComposite(c);
    return;
    }
  
  c->SetView(NULL);
  if (pvc->GetActorTclName() != NULL)
    {
    pvApp->BroadcastScript("%s RemoveProp %s", this->RendererTclName,
			   pvc->GetActorTclName());
    }
  this->Composites->RemoveItem(c);
}


//----------------------------------------------------------------------------
void vtkPVRenderView::Render()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->Update();

  //this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
    
  this->RenderWindow->Render();
}

