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
#include "vtkMesaRenderWindow.h"
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
  vtkMesaRenderWindow *mesaRenderWindow;
  vtkMesaRenderer *mesaRenderer;

  mesaRenderWindow = vtkMesaRenderWindow::New();
  //mesaRenderWindow->DoubleBufferOff();
  //mesaRenderWindow->SwapBuffersOff();
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
  int length, numPixels;
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
  
  numPixels = (window_size[0] * window_size[1]);
  
  if (1)
    {
    float *pdata, *zdata;
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    vtkTreeComposite(this->RenderWindow, controller, 0, zdata, pdata);
    delete [] zdata;
    delete [] pdata;
    }
  else
    {
    length = 3*numPixels;  
    cerr << "    -Start GetData\n";  
    timer->StartTimer();
    pdata = this->RenderWindow->GetPixelData(0,0,window_size[0]-1, window_size[1]-1,1);
    timer->StopTimer();
    cerr << "    -Stop GetData: " << timer->GetElapsedTime() << endl;
    controller->Send((char*)pdata, length, 0, 99);
    }
  
  timer->Delete();  
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




//============================================================================
// Jim's composite stuff


#include "vtkMesaRenderWindow.h"
#include "vtkMultiProcessController.h"

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
  unsigned char *overlay;
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
    localPdata = (float*)((vtkMesaRenderWindow *)renWin)-> \
      GetRGBACharPixelData(0,0,windowSize[0]-1,windowSize[1]-1,0);    
    pdata_size = total_pixels;
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
	  cerr << "phase " << i << " receiver: " << myId 
	       << " receives data from " << id << endl;
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
	  cerr << i << " sender: " << myId << " sends data to "
	         << id << endl;
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
      ((vtkMesaRenderWindow *)renWin)-> \
	SetRGBACharPixelData(0,0, windowSize[0]-1, \
			     windowSize[1]-1,(unsigned char*)localPdata,0);
      }
    }
}















