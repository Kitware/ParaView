/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSlaveProp.cxx
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
#include "vtkPVSlaveProp.h"
#include "vtkMultiProcessController.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkPVSlaveProp* vtkPVSlaveProp::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSlaveProp");
  if(ret)
    {
    return (vtkPVSlaveProp*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSlaveProp;
}


vtkPVSlaveProp::vtkPVSlaveProp()
{
  this->Application = NULL;
  this->RenderWindow = NULL;
  this->RGBAImage = NULL;
}

vtkPVSlaveProp::~vtkPVSlaveProp()
{
  this->SetApplication(NULL);
  this->SetRenderWindow(NULL);
  
  if (this->RGBAImage)
    {
    delete [] this->RGBAImage;
    }
}

// We are manipulating the renderer directly.  This could be a bad thing.
int vtkPVSlaveProp::RenderIntoImage(vtkViewport *viewport)
{
  vtkMultiProcessController *controller;
  int *window_size;
  int total_pixels, zdata_size, pdata_size;
  float *pdata, *zdata;
 
  if (this->RGBAImage != NULL)
    {
    //return 0;
    }
  
  window_size = viewport->GetSize();
  total_pixels = window_size[0] * window_size[1];
  zdata_size = total_pixels;
  pdata_size = 4*total_pixels;

  zdata = new float[zdata_size];
  pdata = new float[pdata_size];

  // Make sure the render slave size matches our size
  this->Application->RemoteScript(0, "[RenderSlave GetRenderWindow] SetSize %d %d", 
		      window_size[0], window_size[1]);
  
  
  vtkTimerLog *timer = vtkTimerLog::New();
  
  timer->StartTimer();
  cerr << "  -Start Remote Render\n";
  // Render and get the data.
  this->Application->RemoteSimpleScript(0, "RenderSlave Render");
  
  controller = this->Application->GetController();
  controller->Receive(zdata, zdata_size, 0, 99);
  controller->Receive(pdata, pdata_size, 0, 99);

  timer->StopTimer();
  cerr << "  -End Remote Render: " << timer->GetElapsedTime() << endl;

  timer->Delete();
  
  if (this->RGBAImage)
    {
    delete [] this->RGBAImage;
    }
  this->RGBAImage = pdata;
  delete [] zdata;
  
  return 0;
}


void vtkPVSlaveProp::SetupTest()
{
  // Here we are going to create a single slave renderer in another process.
  // (As a test)
  
  cerr << "--- SetupTest (renderSlave slave) \n";
  
  this->Application->RemoteSimpleScript(0, "vtkPVRenderSlave RenderSlave");
  // The global "Slave" was setup when the process was initiated.
  this->Application->RemoteSimpleScript(0, "RenderSlave SetPVSlave Slave");
  
  // lets show something as a test.
  this->Application->RemoteSimpleScript(0, "vtkConeSource ConeSource");
  this->Application->RemoteSimpleScript(0, "vtkPolyDataMapper ConeMapper");
  this->Application->RemoteSimpleScript(0, "ConeMapper SetInput [ConeSource GetOutput]");
  this->Application->RemoteSimpleScript(0, "vtkActor ConeActor");
  this->Application->RemoteSimpleScript(0, "ConeActor SetMapper ConeMapper");
  this->Application->RemoteSimpleScript(0, "[RenderSlave GetRenderer] AddActor ConeActor");
}







