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
#include "vtkRayCaster.h"

#include "vtkTimerLog.h"

vtkTimerLog *TIMER = NULL;


void vtkPVRenderViewStartRender(void *arg)
{
  vtkPVRenderView *rv = (vtkPVRenderView*)arg;
  vtkPVApplication *pvApp = (vtkPVApplication*)(rv->GetApplication());
  int *windowSize;
  
  if (TIMER == NULL)
    {  
    TIMER = vtkTimerLog::New();
    }
  TIMER->StartTimer();
  cerr << "  -Start Remote Render\n";
  // Make sure the render slave size matches our size
  windowSize = rv->GetRenderWindow()->GetSize();
  pvApp->RemoteScript(0, "[RenderSlave GetRenderWindow] SetSize %d %d", 
		      windowSize[0], windowSize[1]);

  pvApp->RemoteSimpleScript(0, "RenderSlave Render");

  // Make sure the render slave has the same camera we do.
  float message[15];
  vtkMultiProcessController *controller = pvApp->GetController();
  vtkCamera *cam = rv->GetRenderer()->GetActiveCamera();
  vtkLightCollection *lc = rv->GetRenderer()->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  
  cam->GetPosition(message);
  cam->GetFocalPoint(message+3);
  cam->GetViewUp(message+6);
  light->GetPosition(message+9);
  light->GetFocalPoint(message+12);
  controller->Send(message, 15, 0, 133);
  
  rv->GetRenderWindow()->SwapBuffersOff();
}

void vtkPVRenderViewEndRender(void *arg)
{
  vtkPVRenderView *rv = (vtkPVRenderView*)arg;
  vtkPVApplication *pvApp = (vtkPVApplication*)(rv->GetApplication());
  vtkRenderWindow *renWin = rv->GetRenderWindow();
  vtkMultiProcessController *controller;
  int *windowSize;
  int length;
  unsigned char *pdata, *pTmp;
  unsigned char *overlay, *endPtr;

  windowSize = renWin->GetSize();
  length = 3*(windowSize[0] * windowSize[1]);


  // Get the results from the local process.
  TIMER->StartTimer();
  cerr << "  -Start Get Overlay\n";
  overlay = renWin->GetPixelData(0,0,windowSize[0]-1, \
				 windowSize[1]-1,0);
  TIMER->StopTimer();
  cerr << "  -End GetOverlay: " << TIMER->GetElapsedTime() << endl;
  

  // Get the results from the remote processes.
  pdata = new unsigned char[length];
  
  controller = pvApp->GetController();
  controller->Receive((char*)pdata, length, 0, 99);
  
  TIMER->StopTimer();
  cerr << "  -End Remote Render: " << TIMER->GetElapsedTime() << endl;
  

  // merge the two images.
  TIMER->StartTimer();
  cerr << "  -Start Merge\n";  
  pTmp = pdata;
  endPtr = overlay+length;
  while (overlay != endPtr)
    {
    if (*overlay)
      {
      *pTmp = *overlay;
      }
    ++pTmp;
    ++overlay;
    }
  TIMER->StopTimer();
  cerr << "  -End Merge: " << TIMER->GetElapsedTime() << endl;
  
  // For now just put this image in the renderer.
  TIMER->StartTimer();
  cerr << "  -Start Put Image\n";
  renWin->SetPixelData(0,0,windowSize[0]-1, windowSize[1]-1,pdata,0);
  TIMER->StopTimer();
  cerr << "  -End Put Image: " << TIMER->GetElapsedTime() << endl;

  renWin->SwapBuffersOn();  
  renWin->Frame();
  
  delete [] pdata;
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
  pvApp->RemoteSimpleScript(0, "vtkPVRenderSlave RenderSlave");
  // The global "Slave" was setup when the process was initiated.
  pvApp->RemoteSimpleScript(0, "RenderSlave SetPVSlave Slave");
  
  // lets show something as a test.
  pvApp->RemoteSimpleScript(0, "vtkConeSource ConeSource");
  pvApp->RemoteSimpleScript(0, "vtkPolyDataMapper ConeMapper");
  pvApp->RemoteSimpleScript(0, "ConeMapper SetInput [ConeSource GetOutput]");
  pvApp->RemoteSimpleScript(0, "vtkActor ConeActor");
  pvApp->RemoteSimpleScript(0, "ConeActor SetMapper ConeMapper");
  pvApp->RemoteSimpleScript(0, "[RenderSlave GetRenderer] AddActor ConeActor");
  
  // The start and end methods merge the processes renderers.
  this->GetRenderer()->SetStartRenderMethod(vtkPVRenderViewStartRender, this);
  this->GetRenderer()->SetEndRenderMethod(vtkPVRenderViewEndRender, this);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::ResetCamera()
{
  float bounds[6];
  vtkPVApplication *pvApp = (vtkPVApplication*)(this->Application);
  vtkMultiProcessController *controller = pvApp->GetController();
  
  pvApp->RemoteSimpleScript(0, "RenderSlave TransmitBounds");

  controller->Receive(bounds, 6, 0, 112);
  
  this->GetRenderer()->ResetCamera(bounds);
  this->Render();
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



