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

#include "vtkPVRenderView.h"
#include "vtkPVApplication.h"
#include "vtkMultiProcessController.h"
#include "vtkDummyRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderSlave.h"

#ifdef WIN32
#include "vtkRenderWindow.h"
#else
#include "vtkMesaRenderWindow.h"
#endif

#include "vtkTimerLog.h"

vtkTimerLog *TIMER = NULL;


void vtkPVRenderViewStartRender(void *arg)
{
  vtkPVRenderView *rv = (vtkPVRenderView*)arg;
  vtkPVApplication *pvApp = (vtkPVApplication*)(rv->GetApplication());
  struct vtkPVRenderSlaveInfo info;
  int id, num;
  int *windowSize;
  
  if (TIMER == NULL)
    {  
    TIMER = vtkTimerLog::New();
    }
  TIMER->StartTimer();
  cerr << "  -Start Remote Render\n";

  // Get a global (across all processes) clipping range.
  rv->ResetCameraClippingRange();
  
  // Make sure the render slave has the same camera we do.
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
    pvApp->RemoteSimpleScript(id, "RenderSlave Render");
    controller->Send((char*)(&info), sizeof(struct vtkPVRenderSlaveInfo), id, 133);
    }
  
  // Turn swap buffers off before the render so the end render method has a chance
  // to add to the back buffer.
  rv->GetRenderWindow()->SwapBuffersOff();
}

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
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  this->Interactor->Delete();
  this->Interactor = NULL;
  this->SetInteractorStyle(NULL);
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
  
  this->vtkKWRenderView::Create(app, args);

  // Styles need motion events.
  this->Script("bind %s <Motion> {%s MotionCallback %%x %%y}", 
               this->VTKWidget->GetWidgetName(), this->GetTclName());
  
  
  
  // Setup the remote renderers.
  vtkPVApplication *pvApp = (vtkPVApplication*)app;
  num = pvApp->GetController()->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    pvApp->RemoteSimpleScript(id, "vtkPVRenderSlave RenderSlave");
    // The global "Slave" was setup when the process was initiated.
    pvApp->RemoteSimpleScript(id, "RenderSlave SetPVSlave Slave");
    }
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
    pvApp->RemoteSimpleScript(id, "RenderSlave TransmitBounds");
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



