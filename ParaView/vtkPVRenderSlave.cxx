/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderSlave.cxx
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

#include "vtkPVRenderSlave.h"
#include "vtkPVMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkPVRenderSlave* vtkPVRenderSlave::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVRenderSlave");
  if(ret)
    {
    return (vtkPVRenderSlave*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVRenderSlave;
}


int vtkPVRenderSlaveCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRenderSlave::vtkPVRenderSlave()
{
  vtkPVMesaRenderWindow *mesaRenderWindow;
  vtkMesaRenderer *mesaRenderer;

  mesaRenderWindow = vtkPVMesaRenderWindow::New();
  mesaRenderWindow->SetOffScreenRendering(1);
  mesaRenderer = vtkMesaRenderer::New();
  mesaRenderWindow->AddRenderer(mesaRenderer);

  //this->CommandFunction = vtkPVRenderSlaveCommand;
  this->PVSlave = NULL;
  this->RenderWindow = mesaRenderWindow;
  this->Renderer = mesaRenderer;
}

//----------------------------------------------------------------------------
vtkPVRenderSlave::~vtkPVRenderSlave()
{
  this->RenderWindow->Delete();
  this->RenderWindow = NULL;
  this->Renderer->Delete();
  this->Renderer = NULL;  
}

//----------------------------------------------------------------------------
void vtkPVRenderSlave::Render()
{
  unsigned char *pdata;
  int *window_size;
  int length;
  int myId, numProcs;
  vtkPVRenderSlaveInfo info;
  vtkMultiProcessController *controller;

  controller = this->PVSlave->GetController();
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  // Makes an assumption about how the tasks are setup (UI id is 0).
  // Receive the camera information.
  controller->Receive((char*)(&info), sizeof(struct vtkPVRenderSlaveInfo), 0, 133);
  vtkCamera *cam = this->Renderer->GetActiveCamera();
  vtkLightCollection *lc = this->Renderer->GetLights();
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
  
  this->RenderWindow->SetSize(info.WindowSize);
 
  vtkTimerLog *timer = vtkTimerLog::New();
  cerr << "    -Start Render\n";
  timer->StartTimer();
  this->RenderWindow->Render();
  timer->StopTimer();
  cerr << "    -Stop Render: " << timer->GetElapsedTime() << endl;
  
  
  window_size = this->RenderWindow->GetSize();
  length = 3*(window_size[0] * window_size[1]);

  cerr << "    -Start GetData\n";  
  timer->StartTimer();
  pdata = this->RenderWindow->GetPixelData(0,0,window_size[0]-1, \
					   window_size[1]-1,1);
  timer->StopTimer();
  cerr << "    -Stop GetData: " << timer->GetElapsedTime() << endl;
  timer->Delete();
  
  controller->Send((char*)pdata, length, 0, 99);
}



//----------------------------------------------------------------------------
void vtkPVRenderSlave::TransmitBounds()
{
  float bounds[6];
  vtkMultiProcessController *controller;
  
  this->Renderer->ComputeVisiblePropBounds(bounds);
  controller = this->PVSlave->GetController();

  // Makes an assumption about how the tasks are setup (UI id is 0).
  controller->Send(bounds, 6, 0, 112);  
}
