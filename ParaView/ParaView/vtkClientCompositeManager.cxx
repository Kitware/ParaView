/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientCompositeManager.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClientCompositeManager.h"
#include "vtkCompositeManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkSocketController.h"
#include "vtkCompressCompositer.h"
#include "vtkTreeCompositer.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkTimerLog.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#elif defined(VTK_USE_MESA)
#include "vtkMesaRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkClientCompositeManager, "1.2");
vtkStandardNewMacro(vtkClientCompositeManager);

vtkCxxSetObjectMacro(vtkClientCompositeManager,Compositer,vtkCompositer);

// Structures to communicate render info.
struct vtkClientRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
  float DesiredUpdateRate;
  int ReductionFactor;
};

struct vtkClientRendererInfo 
{
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float LightPosition[3];
  float LightFocalPoint[3];
  float Background[3];
  float ParallelScale;
  float CameraViewAngle;
};

#define vtkInitializeVector3(v) { v[0] = 0; v[1] = 0; v[2] = 0; }
#define vtkInitializeVector2(v) { v[0] = 0; v[1] = 0; }
#define vtkInitializeClientRendererInfoMacro(r)      \
  {                                                     \
  vtkInitializeVector3(r.CameraPosition);               \
  vtkInitializeVector3(r.CameraFocalPoint);             \
  vtkInitializeVector3(r.CameraViewUp);                 \
  vtkInitializeVector2(r.CameraClippingRange);          \
  vtkInitializeVector3(r.LightPosition);                \
  vtkInitializeVector3(r.LightFocalPoint);              \
  vtkInitializeVector3(r.Background);                   \
  r.ParallelScale = 0.0;                                \
  r.CameraViewAngle = 0.0;                              \
  }
  


//-------------------------------------------------------------------------
vtkClientCompositeManager::vtkClientCompositeManager()
{
  this->RenderWindow = NULL;
  this->CompositeController = vtkMultiProcessController::GetGlobalController();
  if (this->CompositeController)
    {
    this->CompositeController->Register(this);
    }

  this->ClientController = NULL;
  this->ClientFlag = 1;

  this->StartTag = 0;
  this->RenderView = NULL;

  this->ReductionFactor = 2;
  this->PDataSize[0] = this->PDataSize[1] = 0;
  this->MagnifiedPDataSize[0] = this->MagnifiedPDataSize[1] = 0;
  this->PData = NULL;
  this->ZData = NULL;
  this->PData2 = NULL;
  this->ZData2 = NULL;
  this->MagnifiedPData = NULL;
  this->RenderView = NULL;

  this->Compositer = vtkCompressCompositer::New();
  //this->Compositer = vtkTreeCompositer::New();

  this->UseChar = 1;
  this->UseRGB = 1;
}

  
//-------------------------------------------------------------------------
vtkClientCompositeManager::~vtkClientCompositeManager()
{
  this->SetRenderWindow(NULL);

  this->SetPDataSize(0,0);
  
  this->SetCompositeController(NULL);
  this->SetClientController(NULL);

  if (this->PData)
    {
    vtkCompositeManager::DeleteArray(this->PData);
    this->PData = NULL;
    }
  if (this->ZData)
    {
    vtkCompositeManager::DeleteArray(this->ZData);
    this->ZData = NULL;
    }

  if (this->PData2)
    {
    vtkCompositeManager::DeleteArray(this->PData2);
    this->PData2 = NULL;
    }
  if (this->ZData2)
    {
    vtkCompositeManager::DeleteArray(this->ZData2);
    this->ZData2 = NULL;
    }

  if (this->MagnifiedPData)
    {
    vtkCompositeManager::DeleteArray(this->MagnifiedPData);
    this->MagnifiedPData = NULL;
    }
  this->SetRenderView(NULL);
  this->SetCompositer(NULL);
}


//=======================  Client ========================


//-------------------------------------------------------------------------
void vtkClientCompositeManagerResetCamera(vtkObject *caller,
                                          unsigned long vtkNotUsed(event), 
                                          void *clientData, void *)
{
  vtkClientCompositeManager *self = (vtkClientCompositeManager *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCamera(ren);
}

//-------------------------------------------------------------------------
void vtkClientCompositeManagerResetCameraClippingRange(vtkObject *caller, 
                   unsigned long vtkNotUsed(event),void *clientData, void *)
{
  vtkClientCompositeManager *self = (vtkClientCompositeManager *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCameraClippingRange(ren);
}


//-------------------------------------------------------------------------
// We may want to pass the render window as an argument for a sanity check.
void vtkClientCompositeManagerStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkClientCompositeManager *self = (vtkClientCompositeManager *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }
  
  self->StartRender();
}

//-------------------------------------------------------------------------
// Only called in process 0.
void vtkClientCompositeManager::StartRender()
{
  struct vtkClientRenderWindowInfo winInfo;
  struct vtkClientRendererInfo renInfo;
  int *size;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  static int firstRender = 1;
  
  if (firstRender)
    {
    firstRender = 0;
    return;
    }
  
  vtkDebugMacro("StartRender");
  
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->ClientController;

  if (controller == NULL)
    {
    return;
    }

  // Make sure they all swp buffers at the same time.
  //renWin->SwapBuffersOff();

  // Trigger the satellite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  size = this->RenderWindow->GetSize();
  winInfo.Size[0] = size[0]/this->ReductionFactor;
  winInfo.Size[1] = size[1]/this->ReductionFactor;
  winInfo.ReductionFactor = this->ReductionFactor;
  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfo.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();
  this->SetPDataSize(winInfo.Size[0], winInfo.Size[1]);
  
  controller->TriggerRMI(1, vtkClientCompositeManager::RENDER_RMI_TAG);

  // Synchronize the size of the windows.
  controller->Send((char*)(&winInfo), 
                   sizeof(vtkClientRenderWindowInfo), 1, 
                   vtkClientCompositeManager::WIN_INFO_TAG);
  
  // Make sure the satellite renderers have the same camera I do.
  // Note: This will lockup unless every process has the same number
  // of renderers.
  rens->InitTraversal();
  while ( (ren = rens->GetNextItem()) )
    {
    cam = ren->GetActiveCamera();
    lc = ren->GetLights();
    lc->InitTraversal();
    light = lc->GetNextItem();
    cam->GetPosition(renInfo.CameraPosition);
    cam->GetFocalPoint(renInfo.CameraFocalPoint);
    cam->GetViewUp(renInfo.CameraViewUp);
    cam->GetClippingRange(renInfo.CameraClippingRange);
    renInfo.CameraViewAngle = cam->GetViewAngle();
    if (cam->GetParallelProjection())
      {
      renInfo.ParallelScale = cam->GetParallelScale();
      }
    else
      {
      renInfo.ParallelScale = 0.0;
      }
    if (light)
      {
      light->GetPosition(renInfo.LightPosition);
      light->GetFocalPoint(renInfo.LightFocalPoint);
      }
    ren->GetBackground(renInfo.Background);
    ren->Clear();
    controller->Send((char*)(&renInfo),
                     sizeof(struct vtkClientRendererInfo), 1, 
                     vtkCompositeManager::REN_INFO_TAG);
    }
  
  
  this->ReceiveAndSetColorBuffer();
  this->RenderWindow->EraseOff();
}

//----------------------------------------------------------------------------
// Method executed only on client.
void vtkClientCompositeManager::ReceiveAndSetColorBuffer()
{
  this->ClientController->Receive(this->PData, 1, 123451);
 
  if (this->ReductionFactor > 1)
    {
    vtkTimerLog::MarkStartEvent("Magnify Buffer");
    this->MagnifyBuffer(this->PData, this->MagnifiedPData, this->PDataSize);
    vtkTimerLog::MarkEndEvent("Magnify Buffer");
    
    // I do not know if this is necessary !!!!!!!
    vtkRenderer* renderer =
      ((vtkRenderer*)
       this->RenderWindow->GetRenderers()->GetItemAsObject(0));
    renderer->SetViewport(0, 0, 1.0, 1.0);
    renderer->GetActiveCamera()->UpdateViewport(renderer);
    // We have to set the color buffer as an even multiple of factor.
    }
   
  // Set the color buffer
  if (this->UseChar) 
    {
    vtkUnsignedCharArray* buf;
    if (this->ReductionFactor > 1)
      {
      buf = static_cast<vtkUnsignedCharArray*>(this->MagnifiedPData);
      }
    else
      {
      buf = static_cast<vtkUnsignedCharArray*>(this->PData);
      }
    if (this->PData->GetNumberOfComponents() == 4)
      {
      vtkTimerLog::MarkStartEvent("Set RGBA Char Buffer");
      this->RenderWindow->SetRGBACharPixelData(0, 0, 
                            this->MagnifiedPDataSize[0]-1, 
                            this->MagnifiedPDataSize[1]-1, buf, 0);
      vtkTimerLog::MarkEndEvent("Set RGBA Char Buffer");
      }
    else if (this->PData->GetNumberOfComponents() == 3)
      {
      vtkTimerLog::MarkStartEvent("Set RGB Char Buffer");
      this->RenderWindow->SetPixelData(0, 0, 
                            this->MagnifiedPDataSize[0]-1, 
                            this->MagnifiedPDataSize[1]-1, buf, 0);
      vtkTimerLog::MarkEndEvent("Set RGB Char Buffer");
      }
    } 
  else 
    {
    if (this->ReductionFactor)
      {
      vtkTimerLog::MarkStartEvent("Set RGBA Float Buffer");
      this->RenderWindow->SetRGBAPixelData(0, 0, 
                     this->MagnifiedPDataSize[0]-1, 
                     this->MagnifiedPDataSize[1]-1,
                     static_cast<vtkFloatArray*>(this->MagnifiedPData), 
                     0);
      vtkTimerLog::MarkEndEvent("Set RGBA Float Buffer");
      }
    else
      {
      vtkTimerLog::MarkStartEvent("Set RGBA Float Buffer");
      this->RenderWindow->SetRGBAPixelData(
                   0, 0, this->MagnifiedPDataSize[0]-1, 
                   this->MagnifiedPDataSize[1]-1,
                   static_cast<vtkFloatArray*>(this->PData), 0);
      vtkTimerLog::MarkEndEvent("Set RGBA Float Buffer");
      }
    }
}


//----------------------------------------------------------------------------
// We change this to work backwards so we can make it inplace. !!!!!!!     
void vtkClientCompositeManager::MagnifyBuffer(vtkDataArray* localP, 
                                              vtkDataArray* magP,
                                              int windowSize[2])
{
  float *rowp, *subp;
  float *pp1;
  float *pp2;
  int   x, y, xi, yi;
  int   xInDim, yInDim;
  // Local increments for input.
  int   pInIncY; 
  float *newLocalPData;
  int numComp = localP->GetNumberOfComponents();
  
  xInDim = windowSize[0];
  yInDim = windowSize[1];

  newLocalPData = reinterpret_cast<float*>(magP->GetVoidPointer(0));
  float* localPdata = reinterpret_cast<float*>(localP->GetVoidPointer(0));

  if (this->UseChar)
    {
    if (numComp == 4)
      {
      // Get the last pixel.
      rowp = localPdata;
      pp2 = newLocalPData;
      for (y = 0; y < yInDim; y++)
        {
        // Duplicate the row rowp N times.
        for (yi = 0; yi < this->ReductionFactor; ++yi)
          {
          pp1 = rowp;
          for (x = 0; x < xInDim; x++)
            {
            // Duplicate the pixel p11 N times.
            for (xi = 0; xi < this->ReductionFactor; ++xi)
              {
              *pp2++ = *pp1;
              }
            ++pp1;
            }
          }
        rowp += xInDim;
        }
      }
    else if (numComp == 3)
      { // RGB char pixel data.
      // Get the last pixel.
      pInIncY = numComp * xInDim;
      unsigned char* crowp = reinterpret_cast<unsigned char*>(localPdata);
      unsigned char* cpp2 = reinterpret_cast<unsigned char*>(newLocalPData);
      unsigned char *cpp1, *csubp;
      for (y = 0; y < yInDim; y++)
        {
        // Duplicate the row rowp N times.
        for (yi = 0; yi < this->ReductionFactor; ++yi)
          {
          cpp1 = crowp;
          for (x = 0; x < xInDim; x++)
            {
            // Duplicate the pixel p11 N times.
            for (xi = 0; xi < this->ReductionFactor; ++xi)
              {
              csubp = cpp1;
              *cpp2++ = *csubp++;
              *cpp2++ = *csubp++;
              *cpp2++ = *csubp;
              }
            cpp1 += numComp;
            }
          }
        crowp += pInIncY;
        }
      }
    }
  else
    {
    // Get the last pixel.
    pInIncY = numComp * xInDim;
    rowp = localPdata;
    pp2 = newLocalPData;
    for (y = 0; y < yInDim; y++)
      {
      // Duplicate the row rowp N times.
      for (yi = 0; yi < this->ReductionFactor; ++yi)
        {
        pp1 = rowp;
        for (x = 0; x < xInDim; x++)
          {
          // Duplicate the pixel p11 N times.
          for (xi = 0; xi < this->ReductionFactor; ++xi)
            {
            subp = pp1;
            if (numComp == 4)
              {
              *pp2++ = *subp++;
              }
            *pp2++ = *subp++;
            *pp2++ = *subp++;
            *pp2++ = *subp;
            }
          pp1 += numComp;
          }
        }
      rowp += pInIncY;
      }
    }
  
}
  


//-------------------------------------------------------------------------
void vtkClientCompositeManager::ResetCamera(vtkRenderer *ren)
{
  float bounds[6];

  if (this->ClientController == NULL)
    {
    return;
    }
  
  this->ComputeVisiblePropBounds(ren, bounds);
  // Keep from setting camera from some outrageous value.
  if (bounds[0]>bounds[1] || bounds[2]>bounds[3] || bounds[4]>bounds[5])
    {
    // See if the not pickable values are better.
    ren->ComputeVisiblePropBounds(bounds);
    if (bounds[0]>bounds[1] || bounds[2]>bounds[3] || bounds[4]>bounds[5])
      {
      return;
      }
    }
  ren->ResetCamera(bounds);
}

//-------------------------------------------------------------------------
void vtkClientCompositeManager::ResetCameraClippingRange(vtkRenderer *ren)
{
  float bounds[6];

  if (this->ClientController == NULL)
    {
    return;
    }
  
  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCameraClippingRange(bounds);
}

//----------------------------------------------------------------------------
void vtkClientCompositeManager::ComputeVisiblePropBounds(vtkRenderer *ren, 
                                                         float bounds[6])
{
  float tmp[6];

  // Make this into a real tag !!!!!
  this->ClientController->TriggerRMI(1,8883);

  // Make this ignore the rotation cursor. (not pickable).
  ren->ComputeVisiblePropBounds(bounds);

  this->ClientController->Receive(tmp, 6, 1, vtkCompositeManager::BOUNDS_TAG);
  if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
  if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
  if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
  if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
  if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
  if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
}




//=======================  Server ========================




//----------------------------------------------------------------------------
void vtkClientCompositeManagerRenderRMI(void *arg, void *, int, int)
{
  vtkClientCompositeManager* self = (vtkClientCompositeManager*) arg;
  
  self->RenderRMI();
}

//----------------------------------------------------------------------------
// Only Called by the satellite processes.
void vtkClientCompositeManager::RenderRMI()
{
  int i;

  if (this->ClientFlag)
    {
    vtkErrorMacro("Expecting the server side to call this method.");
    return;
    }

  // If this is root of server, trigger RenderRMI on satellites.
  if (this->CompositeController->GetLocalProcessId() == 0)
    {
    int numProcs = this->CompositeController->GetNumberOfProcesses();
    for (i = 1; i < numProcs; ++i)
      {
      this->CompositeController->TriggerRMI(i, 
                                    vtkClientCompositeManager::RENDER_RMI_TAG);
      }
    }

  // Get and redistribute renderer information (camera ...)
  this->SatelliteStartRender();
  this->RenderWindow->Render();
  this->SatelliteEndRender();
}

//-------------------------------------------------------------------------
void vtkClientCompositeManager::SatelliteStartRender()
{
  int i, j, myId, numProcs;
  vtkClientRenderWindowInfo winInfo;
  vtkClientRendererInfo renInfo;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam = 0;
  vtkLightCollection *lc;
  vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller; 
  int otherId;

  myId = this->CompositeController->GetLocalProcessId();
  numProcs = this->CompositeController->GetNumberOfProcesses();

  if (myId == 0)
    { // server root receives from client.
    controller = this->ClientController;
    otherId = 1;
    }
  else
    { // Server satellite processes receive from server root.
    controller = this->CompositeController;
    otherId = 0;
    }
  
  vtkInitializeClientRendererInfoMacro(renInfo);
  
  // Receive the window size.
  controller->Receive((char*)(&winInfo), 
                      sizeof(struct vtkClientRenderWindowInfo), 
                      otherId, vtkCompositeManager::WIN_INFO_TAG);
  if (myId == 0)
    {  // Relay info to server satellite processes.
    for (j = 1; j < numProcs; ++j)
      {
      this->CompositeController->Send((char*)(&winInfo), 
                      sizeof(struct vtkClientRenderWindowInfo), j,
                      vtkCompositeManager::WIN_INFO_TAG);
      }
    }

  // This should be fixed.  Round off will cause window to resize. !!!!!
  renWin->SetSize(winInfo.Size[0] * winInfo.ReductionFactor,
                  winInfo.Size[1] * winInfo.ReductionFactor);
  renWin->SetDesiredUpdateRate(winInfo.DesiredUpdateRate);
  this->ReductionFactor = winInfo.ReductionFactor;

  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  for (i = 0; i < winInfo.NumberOfRenderers; ++i)
    {
    // Receive the camera information.

    // We put this before receive because we want the pipeline to be
    // updated the first time if the camera does not exist and we want
    // it to happen before we block in receive
    ren = rens->GetNextItem();
    if (ren)
      {
      cam = ren->GetActiveCamera();
      }

    controller->Receive((char*)(&renInfo), 
                        sizeof(struct vtkClientRendererInfo), 
                        otherId, vtkCompositeManager::REN_INFO_TAG);
    if (myId == 0)
      {  // Relay info to server satellite processes.
      for (j = 1; j < numProcs; ++j)
        {
        this->CompositeController->Send((char*)(&renInfo), 
                        sizeof(struct vtkClientRendererInfo), 
                        j, vtkCompositeManager::REN_INFO_TAG);
        }
      }
    if (ren == NULL)
      {
      vtkErrorMacro("Renderer mismatch.");
      }
    else
      {
      lc = ren->GetLights();
      lc->InitTraversal();
      light = lc->GetNextItem();
  
      cam->SetPosition(renInfo.CameraPosition);
      cam->SetFocalPoint(renInfo.CameraFocalPoint);
      cam->SetViewUp(renInfo.CameraViewUp);
      cam->SetClippingRange(renInfo.CameraClippingRange);
      if (renInfo.ParallelScale != 0.0)
        {
        cam->ParallelProjectionOn();
        cam->SetParallelScale(renInfo.ParallelScale);
        }
      else
        {
        cam->ParallelProjectionOff();   
        }
      if (light)
        {
        light->SetPosition(renInfo.LightPosition);
        light->SetFocalPoint(renInfo.LightFocalPoint);
        }
      ren->SetBackground(renInfo.Background);
      ren->SetViewport(0, 0, 1.0/(float)this->ReductionFactor, 
                       1.0/(float)this->ReductionFactor);
      }
    }
  // This makes sure the arrays are large enough.
  this->SetPDataSize(winInfo.Size[0], winInfo.Size[1]);
}

//-------------------------------------------------------------------------
void vtkClientCompositeManager::SatelliteEndRender()
{  
  int numProcs, myId;
  int front = 1;

  myId = this->CompositeController->GetLocalProcessId();
  numProcs = this->CompositeController->GetNumberOfProcesses();

  // Get the color buffer (pixel data).
  if (this->UseChar) 
    {
    if (this->PData->GetNumberOfComponents() == 4)
      {
      vtkTimerLog::MarkStartEvent("Get RGBA Char Buffer");
      this->RenderWindow->GetRGBACharPixelData(
        0,0,this->PDataSize[0]-1, this->PDataSize[1]-1, 
        front,static_cast<vtkUnsignedCharArray*>(this->PData));
      vtkTimerLog::MarkEndEvent("Get RGBA Char Buffer");
      }
    else if (this->PData->GetNumberOfComponents() == 3)
      {
      vtkTimerLog::MarkStartEvent("Get RGB Char Buffer");
      this->RenderWindow->GetPixelData(
        0,0,this->PDataSize[0]-1, this->PDataSize[1]-1, 
        front,static_cast<vtkUnsignedCharArray*>(this->PData));
      vtkTimerLog::MarkEndEvent("Get RGB Char Buffer");
      }
    } 
  else 
    {
    vtkTimerLog::MarkStartEvent("Get RGBA Float Buffer");
    this->RenderWindow->GetRGBAPixelData(
      0,0,this->PDataSize[0]-1,this->PDataSize[1]-1, 
      front,static_cast<vtkFloatArray*>(this->PData));
    vtkTimerLog::MarkEndEvent("Get RGBA Float Buffer");
    }

  // Do not bother getting Z buffer and compositing if only one proc.
  if (numProcs > 1)
    { 
    // GetZBuffer.
    vtkTimerLog::MarkStartEvent("GetZBuffer");
    this->RenderWindow->GetZbufferData(0,0,
                                       this->PDataSize[0]-1, 
                                       this->PDataSize[1]-1,
                                       this->ZData);  
    vtkTimerLog::MarkEndEvent("GetZBuffer");

    // Let the subclass use its owns composite algorithm to
    // collect the results into "localPData" on process 0.
    vtkTimerLog::MarkStartEvent("Composite Buffers");
    this->Compositer->CompositeBuffer(this->PData, this->ZData,
                                      this->PData2, this->ZData2);
    vtkTimerLog::MarkEndEvent("Composite Buffers");

    // I believe the results end up in PData.
    }

  if (myId == 0)
    {
    this->ClientController->Send(this->PData, 1, 123451);
    }
}



//----------------------------------------------------------------------------
void vtkClientCompositeManagerComputeVisiblePropBoundsRMI(void *arg, void *, 
                                                          int, int)
{
  vtkClientCompositeManager* self = (vtkClientCompositeManager*) arg;
  
  self->ComputeVisiblePropBoundsRMI();
}
//----------------------------------------------------------------------------
// Only called by satellite processes.
void vtkClientCompositeManager::ComputeVisiblePropBoundsRMI()
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  float bounds[6];
  
  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  ren = rens->GetNextItem();
  ren->ComputeVisiblePropBounds(bounds);

  if (this->CompositeController->GetLocalProcessId() == 0)
    {
    float tmp[6];
    int id;
    int numProc = this->CompositeController->GetNumberOfProcesses();
    for (id = 1; id < numProc; ++id)
      {
      this->CompositeController->TriggerRMI(id, 8883);
      this->CompositeController->Receive(tmp, 6, 1, vtkCompositeManager::BOUNDS_TAG);
      if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
      if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
      if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
      if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
      if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
      if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
      }
    // Send total server results to client. 
    this->ClientController->Send(bounds, 6, 1, vtkCompositeManager::BOUNDS_TAG);
    }
  else
    {
    this->CompositeController->Send(bounds, 6, 0, vtkCompositeManager::BOUNDS_TAG);    
    }
}


//-------------------------------------------------------------------------
void vtkClientCompositeManager::InitializeOffScreen()
{
  if (this->RenderWindow)
    { 
    if ( ! this->ClientFlag)
      {
      this->RenderWindow->OffScreenRenderingOn();
      }
    }
}




//-------------------------------------------------------------------------
// Only process 0 needs start and end render callbacks.
void vtkClientCompositeManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  vtkRendererCollection *rens;
  vtkRenderer *ren = 0;

  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (this->ClientFlag)
      {
      this->RenderWindow->RemoveObserver(this->StartTag);
      // Will make do with first renderer. (Assumes renderer does not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
      if (ren)
        {
        ren->RemoveObserver(this->ResetCameraTag);
        ren->RemoveObserver(this->ResetCameraClippingRangeTag);
        }
      }
    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    if (this->ClientFlag)
      {
      vtkCallbackCommand *cbc;
      
      cbc= vtkCallbackCommand::New();
      cbc->SetCallback(vtkClientCompositeManagerStartRender);
      cbc->SetClientData((void*)this);
      // renWin will delete the cbc when the observer is removed.
      this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
      cbc->Delete();

      // Will make do with first renderer. (Assumes renderer does
      // not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
      if (ren)
        {
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(vtkClientCompositeManagerResetCameraClippingRange);
        cbc->SetClientData((void*)this);
        // ren will delete the cbc when the observer is removed.
        this->ResetCameraClippingRangeTag = 
        ren->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
        cbc->Delete();
        
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(vtkClientCompositeManagerResetCamera);
        cbc->SetClientData((void*)this);
        // ren will delete the cbc when the observer is removed.
        this->ResetCameraTag = 
        ren->AddObserver(vtkCommand::ResetCameraEvent,cbc);
        cbc->Delete();
        }
      }
    }
}


//-------------------------------------------------------------------------
void vtkClientCompositeManager::SetCompositeController(
                                          vtkMultiProcessController *mpc)
{
  if (this->CompositeController == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->CompositeController)
    {
    this->CompositeController->UnRegister(this);
    }
  this->CompositeController = mpc;
}


//-------------------------------------------------------------------------
void vtkClientCompositeManager::SetClientController(
                                          vtkSocketController *mpc)
{
  if (this->ClientController == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->ClientController)
    {
    this->ClientController->UnRegister(this);
    }
  this->ClientController = mpc;
}

//-------------------------------------------------------------------------
void vtkClientCompositeManager::SetUseChar(int useChar)
{
  if (useChar == this->UseChar)
    {
    return;
    }
  this->Modified();
  this->UseChar = useChar;

  // Cannot use float RGB (must be float RGBA).
  if (this->UseChar == 0)
    {
    this->UseRGB = 0;
    }
  
  this->ReallocPDataArrays();
}


//-------------------------------------------------------------------------
void vtkClientCompositeManager::SetUseRGB(int useRGB)
{
  if (useRGB == this->UseRGB)
    {
    return;
    }
  this->Modified();
  this->UseRGB = useRGB;

  // Cannot use float RGB (must be char RGB).
  if (useRGB)
    {  
    this->UseChar = 1;
    }

  this->ReallocPDataArrays();
}

//-------------------------------------------------------------------------
// Only reallocs arrays if they have been allocated already.
// This method is only used when buffer options have been changed:
// Char vs. float, or RGB vs. RGBA.
void vtkClientCompositeManager::ReallocPDataArrays()
{
  int numComps = 4;
  int numTuples = this->PDataSize[0] * this->PDataSize[1];
  int magNumTuples = this->MagnifiedPDataSize[0] * this->MagnifiedPDataSize[1];
  int numProcs = 1;

  if ( ! this->ClientFlag)
    {
    numProcs = this->CompositeController->GetNumberOfProcesses();
    }

  if (this->UseRGB)
    {
    numComps = 3;
    }

  if (this->PData)
    {
    vtkCompositeManager::DeleteArray(this->PData);
    this->PData = NULL;
    } 
  if (this->PData2)
    {
    vtkCompositeManager::DeleteArray(this->PData2);
    this->PData2 = NULL;
    } 
  if (this->MagnifiedPData)
    {
    vtkCompositeManager::DeleteArray(this->MagnifiedPData);
    this->MagnifiedPData = NULL;
    } 

  if (this->UseChar)
    {
    this->PData = vtkUnsignedCharArray::New();
    vtkCompositeManager::ResizeUnsignedCharArray(
        static_cast<vtkUnsignedCharArray*>(this->PData),
        numComps, numTuples);
    if (numProcs > 1)
      { // Not client (numProcs == 1)
      this->PData2 = vtkUnsignedCharArray::New();
      vtkCompositeManager::ResizeUnsignedCharArray(
          static_cast<vtkUnsignedCharArray*>(this->PData2),
          numComps, numTuples);
      }
    if (this->ClientFlag)
      {
      this->MagnifiedPData = vtkUnsignedCharArray::New();
      vtkCompositeManager::ResizeUnsignedCharArray(
          static_cast<vtkUnsignedCharArray*>(this->MagnifiedPData),
          numComps, magNumTuples);
      }
    }
  else 
    {
    this->PData = vtkFloatArray::New();
    vtkCompositeManager::ResizeFloatArray(
            static_cast<vtkFloatArray*>(this->PData),
            numComps, numTuples);
    if (numProcs > 1)
      { // Not client (numProcs == 1)
      this->PData2 = vtkFloatArray::New();
      vtkCompositeManager::ResizeFloatArray(
            static_cast<vtkFloatArray*>(this->PData2),
            numComps, numTuples);
      }
    if (this->ClientFlag)
      {
      this->MagnifiedPData = vtkFloatArray::New();
      vtkCompositeManager::ResizeFloatArray(
              static_cast<vtkFloatArray*>(this->MagnifiedPData),
              numComps, magNumTuples);
      }
    }
}

// Work this and realloc PData into compositer.
//-------------------------------------------------------------------------
void vtkClientCompositeManager::SetPDataSize(int x, int y)
{
  int numComps;  
  int numPixels;
  int magNumPixels;
  int numProcs = 1;

  if ( ! this->ClientFlag)
    {
    numProcs = this->CompositeController->GetNumberOfProcesses();
    }

  if (x < 0)
    {
    x = 0;
    }
  if (y < 0)
    {
    y = 0;
    }

  if (this->PDataSize[0] == x && this->PDataSize[1] == y)
    {
    return;
    }

  this->PDataSize[0] = x;
  this->PDataSize[1] = y;
  this->MagnifiedPDataSize[0] = x * this->ReductionFactor;
  this->MagnifiedPDataSize[1] = y * this->ReductionFactor;

  if (x == 0 || y == 0)
    {
    if (this->PData)
      {
      vtkCompositeManager::DeleteArray(this->PData);
      this->PData = NULL;
      }
    if (this->PData2)
      {
      vtkCompositeManager::DeleteArray(this->PData2);
      this->PData2 = NULL;
      }
    if (this->ZData)
      {
      vtkCompositeManager::DeleteArray(this->ZData);
      this->ZData = NULL;
      }
    if (this->ZData2)
      {
      vtkCompositeManager::DeleteArray(this->ZData2);
      this->ZData2 = NULL;
      }
    if (this->MagnifiedPData)
      {
      vtkCompositeManager::DeleteArray(this->MagnifiedPData);
      this->MagnifiedPData = NULL;
      }
    return;
    }    

  numPixels = x * y;
  magNumPixels = this->MagnifiedPDataSize[0] 
                  * this->MagnifiedPDataSize[1];


  if (numProcs > 1)
    { // Not client (numProcs == 1)
    if (!this->ZData)
      {
      this->ZData = vtkFloatArray::New();
      }
    vtkCompositeManager::ResizeFloatArray(
      static_cast<vtkFloatArray*>(this->ZData), 
      1, numPixels);
    if (!this->ZData2)
      {
      this->ZData2 = vtkFloatArray::New();
      }
    vtkCompositeManager::ResizeFloatArray(
      static_cast<vtkFloatArray*>(this->ZData2), 
      1, numPixels);
    }


  // 3 for RGB,  4 for RGBA (RGB option only for char).
  if (this->UseRGB)
    {
    numComps = 3;
    }
  else
    { // RGBA
    numComps = 4;
    }
  
  if (this->UseChar)
    {
    if (!this->PData)
      {
      this->PData = vtkUnsignedCharArray::New();
      }
    vtkCompositeManager::ResizeUnsignedCharArray(
      static_cast<vtkUnsignedCharArray*>(this->PData), 
      numComps, numPixels);
    if (numProcs > 1)
      { // Not client (numProcs == 1)
      if (!this->PData2)
        {
        this->PData2 = vtkUnsignedCharArray::New();
        }
      vtkCompositeManager::ResizeUnsignedCharArray(
        static_cast<vtkUnsignedCharArray*>(this->PData2), 
        numComps, numPixels);
      }
    if (this->ClientFlag)
      {
      if (!this->MagnifiedPData)
        {
        this->MagnifiedPData = vtkUnsignedCharArray::New();
        }
      vtkCompositeManager::ResizeUnsignedCharArray(
        static_cast<vtkUnsignedCharArray*>(this->MagnifiedPData), 
        numComps, magNumPixels);
      }
    }
  else
    {
    if (!this->PData)
      {
      this->PData = vtkFloatArray::New();
      }

    vtkCompositeManager::ResizeFloatArray(
      static_cast<vtkFloatArray*>(this->PData), 
      numComps, numPixels);
    if (numProcs > 1)
      { // Not client (numProcs == 1)
      if (!this->PData)
        {
        this->PData = vtkFloatArray::New();
        }
      vtkCompositeManager::ResizeFloatArray(
        static_cast<vtkFloatArray*>(this->PData2), 
        numComps, numPixels);
      }
    if (this->ClientFlag)
      {
      if (!this->MagnifiedPData)
        {
        this->MagnifiedPData = vtkFloatArray::New();
        }
      vtkCompositeManager::ResizeFloatArray(
        static_cast<vtkFloatArray*>(this->MagnifiedPData), 
        numComps, magNumPixels);
      }
    }
}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkClientCompositeManager::InitializeRMIs()
{
  if (this->ClientFlag)
    { // Just in case.
    return;
    }
  if (this->CompositeController->GetLocalProcessId() == 0)
    { // Root on server waits for RMIs triggered by client.
    if (this->ClientController == NULL)
      {
      vtkErrorMacro("Missing Controller.");
      return;
      }
    this->ClientController->AddRMI(vtkClientCompositeManagerRenderRMI, (void*)this, 
                                   vtkClientCompositeManager::RENDER_RMI_TAG); 
    this->ClientController->AddRMI(vtkClientCompositeManagerComputeVisiblePropBoundsRMI,
                                   (void*)this, 8883);  
    }
  else
    { // Other satellite processes wait for RMIs for root.
    this->CompositeController->AddRMI(vtkClientCompositeManagerRenderRMI, (void*)this, 
                                   vtkClientCompositeManager::RENDER_RMI_TAG); 
    this->CompositeController->AddRMI(vtkClientCompositeManagerComputeVisiblePropBoundsRMI,
                                   (void*)this, 8883);  
    }
}



//----------------------------------------------------------------------------
void vtkClientCompositeManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  if ( this->RenderWindow )
    {
    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    }
  else
    {
    os << indent << "RenderWindow: (none)\n";
    }
  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
  
  os << indent << "CompositeController: (" << this->CompositeController << ")\n"; 
  os << indent << "ClientController: (" << this->ClientController << ")\n"; 
}



