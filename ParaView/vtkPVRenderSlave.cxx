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
#include "vtkMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkObjectFactory.h"



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
  vtkMesaRenderWindow *mesaRenderWindow;
  vtkMesaRenderer *mesaRenderer;

  mesaRenderWindow = vtkMesaRenderWindow::New();
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
  float *zdata, *pdata;
  int *window_size;
  int total_pixels;
  int pdata_size, zdata_size;
  int myId, numProcs, masterId;
  vtkMultiProcessController *controller;

  this->RenderWindow->Render();

  controller = this->PVSlave->GetController();
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  // Makes an assumption about how the tasks are setup.
  masterId = numProcs - 1;
  
  window_size = this->RenderWindow->GetSize();
  total_pixels = window_size[0] * window_size[1];

  zdata = this->RenderWindow->GetZbufferData(0,0,window_size[0]-1, window_size[1]-1);
  zdata_size = total_pixels;

  pdata = this->RenderWindow->GetRGBAPixelData(0,0,window_size[0]-1, \
				   window_size[1]-1,1);
  pdata_size = 4*total_pixels;
  // pdata = ((vtkMesaRenderWindow *)renWin)->GetRGBACharPixelData(0,0,window_size[0]-1,window_size[1]-1,1);    
  // pdata_size = total_pixels;

  controller->Send(zdata, zdata_size, masterId, 99);
  controller->Send(pdata, pdata_size, masterId, 99);

}

