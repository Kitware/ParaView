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

#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"

#include "vtkPVRenderView.h"
#include "vtkPVApplication.h"
#include "vtkMultiProcessController.h"
#include "vtkDummyRenderWindowInteractor.h"
#include "vtkObjectFactory.h"

#ifdef WIN32
#include "vtkRenderWindow.h"
#else
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#endif

#include "vtkTimerLog.h"

vtkTimerLog *TIMER = NULL;


// A structure to communicate renderer info.
struct vtkPVRenderInfo 
{
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float LightPosition[3];
  float LightFocalPoint[3];
  int WindowSize[2];
};

// Jim's composite stuff

//----------------------------------------------------------------------------
// Results are put in the local data.
void vtkCompositeImagePair(float *localZdata, float *localPdata, 
			   float *remoteZdata, float *remotePdata, 
			   int total_pixels, int flag) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;

  if (flag) 
    {
    pixel_data_size = 4;
    for (i = 0; i < total_pixels; i++) 
      {
      if (remoteZdata[i] < localZdata[i]) 
	{
	localZdata[i] = remoteZdata[i];
	for (j = 0; j < pixel_data_size; j++) 
	  {
	  localPdata[i*pixel_data_size+j] = remotePdata[i*pixel_data_size+j];
	  }
	}
      }
    } 
  else 
    {
    pEnd = remoteZdata + total_pixels;
    while(remoteZdata != pEnd) 
      {
      if (*remoteZdata < *localZdata) 
	{
	*localZdata++ = *remoteZdata++;
	*localPdata++ = *remotePdata++;
	}
      else
	{
	++localZdata;
	++remoteZdata;
	++localPdata;
	++remotePdata;
	}
      }
    }
}


#define pow2(j) (1 << j)


//----------------------------------------------------------------------------
void vtkTreeComposite(vtkRenderWindow *renWin, 
		      vtkMultiProcessController *controller,
		      int flag, float *remoteZdata, 
		      float *remotePdata) 
{
  float *localZdata, *localPdata;
  int *windowSize;
  int total_pixels;
  int pdata_size, zdata_size;
  int myId, numProcs;
  int i, id;
  

  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  windowSize = renWin->GetSize();
  total_pixels = windowSize[0] * windowSize[1];

  // Get the z buffer.
  localZdata = renWin->GetZbufferData(0,0,windowSize[0]-1, windowSize[1]-1);
  zdata_size = total_pixels;

  // Get the pixel data.
  if (flag) 
    { 
    localPdata = renWin->GetRGBAPixelData(0,0,windowSize[0]-1, \
				     windowSize[1]-1,0);
    pdata_size = 4*total_pixels;
    } 
  else 
    {
#ifndef WIN32
    // Condition is here until we fix the resize bug in vtkMesarenderWindow.
    localPdata = (float*)((vtkMesaRenderWindow *)renWin)-> \
      	GetRGBACharPixelData(0,0,windowSize[0]-1,windowSize[1]-1,0);    
    pdata_size = total_pixels;
#endif
    }
  
  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

  for (i = 0; i < logProcs; i++) 
    {
    if ((myId % (int)pow2(i)) == 0) 
      { // Find participants
      if ((myId % (int)pow2(i+1)) < pow2(i)) 
        {
	// receivers
	id = myId+pow2(i);
	
	// only send or receive if sender or receiver id is valid
	// (handles non-power of 2 cases)
	if (id < numProcs) 
          {
	  //cerr << "phase " << i << " receiver: " << myId 
	  //     << " receives data from " << id << endl;
	  controller->Receive(remoteZdata, zdata_size, id, 99);
	  controller->Receive(remotePdata, pdata_size, id, 99);
	  
	  // notice the result is stored as the local data
	  vtkCompositeImagePair(localZdata, localPdata, remoteZdata, remotePdata, 
				total_pixels, flag);
	  }
	}
      else 
	{
	id = myId-pow2(i);
	if (id < numProcs) 
	  {
	  //cerr << i << " sender: " << myId << " sends data to "
	  //       << id << endl;
	  controller->Send(localZdata, zdata_size, id, 99);
	  controller->Send(localPdata, pdata_size, id, 99);
	  }
	}
      }
    }

  if (myId ==0) 
    {
    if (flag) 
      {
      renWin->SetRGBAPixelData(0,0,windowSize[0]-1, 
			       windowSize[1]-1,localPdata,0);
      } 
    else 
      {
#ifndef WIN32
      ((vtkMesaRenderWindow *)renWin)-> \
	         SetRGBACharPixelData(0,0, windowSize[0]-1, \
			     windowSize[1]-1,(unsigned char*)localPdata,0);
#endif
      }
    }
}

//----------------------------------------------------------------------------
// Start method of renderer.  Called before a render.
void vtkPVRenderViewStartRender(void *arg)
{
  vtkPVRenderView *rv = (vtkPVRenderView*)arg;
  vtkPVApplication *pvApp = (vtkPVApplication*)(rv->GetApplication());
  struct vtkPVRenderInfo info;
  int id, num;
  int *windowSize;
  
  if (TIMER == NULL)
    {  
    TIMER = vtkTimerLog::New();
    }
  TIMER->StartTimer();

  // Get a global (across all processes) clipping range.
  rv->ResetCameraClippingRange();
  
  // Make sure the satellite renderers have the same camera I do.
  vtkMultiProcessController *controller = pvApp->GetController();
  vtkCamera *cam = rv->GetRenderer()->GetActiveCamera();
  vtkLightCollection *lc = rv->GetRenderer()->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  
  cam->GetPosition(info.CameraPosition);
  cam->GetFocalPoint(info.CameraFocalPoint);
  cam->GetViewUp(info.CameraViewUp);
  cam->GetClippingRange(info.CameraClippingRange);
  light->GetPosition(info.LightPosition);
  light->GetFocalPoint(info.LightFocalPoint);

  // Make sure the render slave size matches our size
  windowSize = rv->GetRenderWindow()->GetSize();
  info.WindowSize[0] = windowSize[0];
  info.WindowSize[1] = windowSize[1];

  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteScript(id, "%s RenderHack", rv->GetTclName());
    controller->Send((char*)(&info), sizeof(struct vtkPVRenderInfo), id, 133);
    }
  
  // Turn swap buffers off before the render so the end render method has a chance
  // to add to the back buffer.
  rv->GetRenderWindow()->SwapBuffersOff();
}

//----------------------------------------------------------------------------
// End method of renderer.  Called after a render.
void vtkPVRenderViewEndRender(void *arg)
{
  vtkPVRenderView *rv = (vtkPVRenderView*)arg;
  vtkPVApplication *pvApp = (vtkPVApplication*)(rv->GetApplication());
  vtkRenderWindow *renWin = rv->GetRenderWindow();
  vtkMultiProcessController *controller;
  int *windowSize;
  int numPixels;
  int numProcs;
  float *pdata, *zdata;    
  
  windowSize = renWin->GetSize();
  controller = pvApp->GetController();
  numProcs = controller->GetNumberOfProcesses();
  numPixels = (windowSize[0] * windowSize[1]);

  if (numProcs > 1)
    {
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    vtkTreeComposite(rv->GetRenderWindow(), controller, 0, zdata, pdata);
    
    delete [] zdata;
    delete [] pdata;    
    }
  
  // Force swap buffers here.
  renWin->SwapBuffersOn();  
  renWin->Frame();
}




//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVRenderView::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVRenderView");
  if(ret)
    {
    return (vtkPVRenderView*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVRenderView;
}


int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  this->CommandFunction = vtkPVRenderViewCommand;
  this->InteractorStyle = NULL;
  this->Interactor = vtkDummyRenderWindowInteractor::New();
  
#ifdef WIN32
  vtkWin32OpenGLRenderWindow *renderWindow;
  vtkRenderer *renderer;

  renderWindow = vtkWin32OpenGLRenderWindow::New();
  // Win32 have SetupmemoryRendering.
  // We are not going to run this app on win32 in parallel.
  //renderWindow->SetOffScreenRendering(1);
  renderer = vtkRenderer::New();
  renderWindow->AddRenderer(renderer);

  this->RenderWindowHack = renderWindow;
  this->RendererHack = renderer;
#else
  vtkMesaRenderWindow *mesaRenderWindow;
  vtkMesaRenderer *mesaRenderer;

  mesaRenderWindow = vtkMesaRenderWindow::New();
  mesaRenderWindow->SetOffScreenRendering(1);
  mesaRenderer = vtkMesaRenderer::New();
  mesaRenderWindow->AddRenderer(mesaRenderer);

  this->RenderWindowHack = mesaRenderWindow;
  this->RendererHack = mesaRenderer;
#endif 
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  this->Interactor->Delete();
  this->Interactor = NULL;
  this->SetInteractorStyle(NULL);
  
  this->RenderWindowHack->Delete();
  this->RenderWindowHack = NULL;
  this->RendererHack->Delete();
  this->RendererHack = NULL;    
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Clone(vtkPVApplication *pvApp)
{
  this->Application = pvApp;
  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());
  
  this->Application = NULL;
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetApplication(vtkKWApplication *app)
{
  this->vtkKWView::SetApplication(app);
  
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetApplication %s", this->GetTclName(),
			   pvApp->GetTclName());
    }
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
void vtkPVRenderView::Create(vtkKWApplication *app, char *args)
{
  int id, num;
  
  if (this->Application)
    {
    vtkErrorMacro("RenderView already created");
    return;
    }
  
#ifndef WIN32  
  if (app->GetWidgetVisibility() == 0)
    {
    this->Renderer->Delete();
    this->Renderer = NULL;
    this->RenderWindow->Delete();
    this->RenderWindow = NULL;
    
    vtkMesaRenderWindow *mesaRenderWindow;
    vtkMesaRenderer *mesaRenderer;

    mesaRenderWindow = vtkMesaRenderWindow::New();
    mesaRenderWindow->SetOffScreenRendering(1);
    mesaRenderer = vtkMesaRenderer::New();
    mesaRenderWindow->AddRenderer(mesaRenderer);
    
    this->RenderWindow = mesaRenderWindow;
    this->Renderer = mesaRenderer;    
    }
#endif
  
  this->vtkKWRenderView::Create(app, args);

  // Styles need motion events.
  this->Script("bind %s <Motion> {%s MotionCallback %%x %%y}", 
               this->VTKWidget->GetWidgetName(), this->GetTclName());
  
  // The start and end methods merge the processes renderers.
  this->GetRenderer()->SetStartRenderMethod(vtkPVRenderViewStartRender, this);
  this->GetRenderer()->SetEndRenderMethod(vtkPVRenderViewEndRender, this);  
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ComputeVisiblePropBounds(float bounds[6])
{
  float tmp[6];
  int id, num;
  
  vtkPVApplication *pvApp = (vtkPVApplication*)(this->Application);
  vtkMultiProcessController *controller = pvApp->GetController();
  
  num = controller->GetNumberOfProcesses();  
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteScript(id, "%s TransmitBounds", this->GetTclName());
    }

  this->GetRenderer()->ComputeVisiblePropBounds(bounds);

  for (id = 1; id < num; ++id)
    {
    controller->Receive(tmp, 6, id, 112);
    if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
    if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
    if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
    if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
    if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
    if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCamera()
{
  float bounds[6];

  this->ComputeVisiblePropBounds(bounds);
  this->GetRenderer()->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCameraClippingRange()
{
  float bounds[6];

  this->ComputeVisiblePropBounds(bounds);
  this->GetRenderer()->ResetCameraClippingRange(bounds);
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
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Button1Motion(int x, int y)
{
  int *size = this->GetRenderer()->GetSize();
  y = size[1] - y;

  if (this->InteractorStyle)
    {
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
void vtkPVRenderView::TransmitBounds()
{
  float bounds[6];
  vtkMultiProcessController *controller;
  vtkPVApplication *pvApp;
  
  pvApp = (vtkPVApplication*)(this->Application);
  controller = pvApp->GetController();
  
  this->RendererHack->ComputeVisiblePropBounds(bounds);

  // Makes an assumption about how the tasks are setup (UI id is 0).
  controller->Send(bounds, 6, 0, 112);  
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
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s AddCompositeHack %s",
			   this->GetTclName(), c->GetTclName());
    }

  this->vtkKWRenderView::AddComposite(c);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::AddCompositeHack(vtkKWComposite *c)
{
  this->RendererHack->AddProp(c->GetProp());
}


//----------------------------------------------------------------------------
void vtkPVRenderView::RenderHack()
{
  unsigned char *pdata;
  int *window_size;
  int length, numPixels;
  int myId, numProcs;
  vtkPVRenderInfo info;
  vtkPVApplication *pvApp;
  vtkMultiProcessController *controller;
  vtkRenderer *ren;
  vtkRenderWindow *renWin;
  
  pvApp = (vtkPVApplication*)(this->Application);
  controller = pvApp->GetController();
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  ren = this->RendererHack;
  renWin = this->RenderWindowHack;
  
  
  // Makes an assumption about how the tasks are setup (UI id is 0).
  // Receive the camera information.
  controller->Receive((char*)(&info), sizeof(struct vtkPVRenderInfo), 0, 133);
  vtkCamera *cam = ren->GetActiveCamera();
  vtkLightCollection *lc = ren->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  
  cam->SetPosition(info.CameraPosition);
  cam->SetFocalPoint(info.CameraFocalPoint);
  cam->SetViewUp(info.CameraViewUp);
  cam->SetClippingRange(info.CameraClippingRange);
  if (light)
    {
    light->SetPosition(info.LightPosition);
    light->SetFocalPoint(info.LightFocalPoint);
    }
  
  renWin->SetSize(info.WindowSize);
  renWin->Render();

  renWin->SetFileName("/home/lawcc/Views/ParaView/partial0.ppm");
  renWin->SaveImageAsPPM();
  
  window_size = renWin->GetSize();
  
  numPixels = (window_size[0] * window_size[1]);
  
  if (1)
    {
    float *pdata, *zdata;
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    vtkTreeComposite(renWin, controller, 0, zdata, pdata);
    delete [] zdata;
    delete [] pdata;
    }
  else
    {
    length = 3*numPixels;  
    pdata = renWin->GetPixelData(0,0,window_size[0]-1, window_size[1]-1,1);
    controller->Send((char*)pdata, length, 0, 99);
    }
}




