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

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

#include "vtkClientCompositeManager.h"
#include "vtkCompositeManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCompressCompositer.h"
#include "vtkFloatArray.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkTreeCompositer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedCharArray.h"
// Until we trigger LOD from AllocatedRenderTime ...
#include "vtkPVApplication.h"
#include "vtkByteSwap.h"

#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkBMPWriter.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#elif defined(VTK_USE_MESA)
#include "vtkMesaRenderWindow.h"
#endif


vtkCxxRevisionMacro(vtkClientCompositeManager, "1.18.2.5");
vtkStandardNewMacro(vtkClientCompositeManager);

vtkCxxSetObjectMacro(vtkClientCompositeManager,Compositer,vtkCompositer);

// Structures to communicate render info.
struct vtkClientRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
  int ImageReductionFactor;
  int SquirtLevel;
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
  this->SquirtLevel = 0;
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

  this->InternalReductionFactor = 2;
  this->ImageReductionFactor = 2;
  this->PDataSize[0] = this->PDataSize[1] = 0;
  this->MagnifiedPDataSize[0] = this->MagnifiedPDataSize[1] = 0;
  this->PData = NULL;
  this->ZData = NULL;
  this->PData2 = NULL;
  this->ZData2 = NULL;
  this->SquirtArray = NULL;
  this->MagnifiedPData = NULL;
  this->RenderView = NULL;

  this->Compositer = vtkCompressCompositer::New();
  //this->Compositer = vtkTreeCompositer::New();

  this->Tiled = 0;
  this->TiledDimensions[0] = this->TiledDimensions[1] = 1;

  this->UseChar = 1;
  this->UseRGB = 0;

  this->BaseArray = NULL;

  this->UseCompositing = 0;
  
  this->RenWin = vtkRenderWindow::New();
  this->CompositeData = vtkImageData::New();
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

  if (this->SquirtArray)
    {
    vtkCompositeManager::DeleteArray(this->SquirtArray);
    this->SquirtArray = NULL;
    }
  if (this->MagnifiedPData)
    {
    vtkCompositeManager::DeleteArray(this->MagnifiedPData);
    this->MagnifiedPData = NULL;
    }
  this->SetRenderView(NULL);
  this->SetCompositer(NULL);


  if (this->BaseArray)
    {
    this->BaseArray->Delete();
    }
  
  this->RenWin->Delete();
  this->CompositeData->Delete();
}


//----------------------------------------------------------------------------
// Called only on the client.
float vtkClientCompositeManager::GetZBufferValue(int x, int y)
{
  float z;
  int pArg[3];

  if (this->UseCompositing == 0)
    {
    // This could cause a problem between setting this ivar and rendering.
    // We could always composite, and always consider client z.
    float *pz;
    pz = this->RenderWindow->GetZbufferData(x, y, x, y);
    z = *pz;
    delete [] pz;
    return z;
    }
  
  // This first int is to check for byte swapping.
  pArg[0] = 1;
  pArg[1] = x;
  pArg[2] = y;
  this->ClientController->TriggerRMI(1, (void*)pArg, sizeof(int)*3, 
                                vtkClientCompositeManager::GATHER_Z_RMI_TAG);
  this->ClientController->Receive(&z, 1, 1, vtkClientCompositeManager::CLIENT_Z_TAG);
  return z;
}

//----------------------------------------------------------------------------
void vtkClientCompositeManagerGatherZBufferValueRMI(void *local, void *pArg, 
                                                    int pLength, int)
{
  vtkClientCompositeManager* self = (vtkClientCompositeManager*)local;
  int *p;
  int x, y;

  if (pLength != sizeof(int)*3)
    {
    vtkGenericWarningMacro("Integer sizes differ.");
    }

  p = (int*)pArg;
  if (p[0] != 1)
    { // Need to swap
    vtkByteSwap::SwapVoidRange(pArg, 3, sizeof(int));
    if (p[0] != 1)
      { // Swapping did not work.
      vtkGenericWarningMacro("Swapping failed.");
      }
    }
  x = p[1];
  y = p[2];
  
  self->GatherZBufferValueRMI(x, y);
}

//----------------------------------------------------------------------------
void vtkClientCompositeManager::GatherZBufferValueRMI(int x, int y)
{
  float z, otherZ;
  int pArg[3];

  // Get the z value.
  int *size = this->RenderWindow->GetSize();
  if (x < 0 || x >= size[0] || y < 0 || y >= size[1])
    {
    vtkErrorMacro("Point not contained in window.");
    z = 0;
    }
  else
    {
    float *tmp;
    tmp = this->RenderWindow->GetZbufferData(x, y, x, y);
    z = *tmp;
    delete [] tmp;
    }

  int myId = this->CompositeController->GetLocalProcessId();
  if (myId == 0)
    {
    int numProcs = this->CompositeController->GetNumberOfProcesses();
    int idx;
    pArg[0] = 1;
    pArg[1] = x;
    pArg[2] = y;
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->CompositeController->TriggerRMI(1, (void*)pArg, sizeof(int)*3, 
                          vtkClientCompositeManager::GATHER_Z_RMI_TAG);
      }
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->CompositeController->Receive(&otherZ, 1, idx, vtkClientCompositeManager::SERVER_Z_TAG);
      if (otherZ < z)
        {
        z = otherZ;
        }
      }
    // Send final result to the client.
    this->ClientController->Send(&z, 1, 1, vtkClientCompositeManager::CLIENT_Z_TAG);
    }
  else
    {
    // Send z to the root server node..
    this->CompositeController->Send(&z, 1, 1, vtkClientCompositeManager::SERVER_Z_TAG);
    }
}



//=======================  Client ========================



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
  float updateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  if ( ! this->UseCompositing)
    {
    this->RenderWindow->EraseOn();
    this->RenderWindow->Render();
    return;
    }

  if (firstRender)
    {
    firstRender = 0;
    return;
    }

  this->InternalReductionFactor = this->ImageReductionFactor;
  if (this->InternalReductionFactor < 1)
    {
    this->InternalReductionFactor = 1;
    }
  if (updateRate <= 2.0)
    {
    this->InternalReductionFactor = 1;
    }
  
  vtkDebugMacro("StartRender");
  
  vtkMultiProcessController *controller = this->ClientController;

  if (controller == NULL)
    {
    this->RenderWindow->EraseOn();
    return;
    }

  // Make sure they all swp buffers at the same time.
  //vtkRenderWindow* renWin = this->RenderWindow;
  //renWin->SwapBuffersOff();

  // Trigger the satellite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  size = this->RenderWindow->GetSize();
  winInfo.Size[0] = size[0]/this->InternalReductionFactor;
  winInfo.Size[1] = size[1]/this->InternalReductionFactor;
  winInfo.ImageReductionFactor = this->InternalReductionFactor;
  winInfo.SquirtLevel = this->SquirtLevel;
  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  this->SetPDataSize(winInfo.Size[0], winInfo.Size[1]);
  
  controller->TriggerRMI(1, vtkClientCompositeManager::RENDER_RMI_TAG);

  // Synchronize the size of the windows.
  //controller->Send((char*)(&winInfo), 
  //                 sizeof(vtkClientRenderWindowInfo), 1, 
  //                 vtkClientCompositeManager::WIN_INFO_TAG);
  // Let the socket controller deal with byte swapping.
  controller->Send((int*)(&winInfo), 5, 1, 
                   vtkClientCompositeManager::WIN_INFO_TAG);
  
  // Make sure the satellite renderers have the same camera I do.
  // Note: This will lockup unless every process has the same number
  // of renderers.
  rens->InitTraversal();
  ren = rens->GetNextItem();
//  while ( (ren = rens->GetNextItem()) )
//    {
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
    //controller->Send((char*)(&renInfo),
    //                 sizeof(struct vtkClientRendererInfo), 1, 
    //                 vtkCompositeManager::REN_INFO_TAG);
    // Let the socket controller deal with byte swapping.
    controller->Send((float*)(&renInfo), 22, 1,
                     vtkCompositeManager::REN_INFO_TAG);
//    }
  
  if (this->Tiled == 0)
    {
    this->ReceiveAndSetColorBuffer();
//    this->RenderWindow->EraseOff();
    this->EndRender();
    cam->SetPosition(renInfo.CameraPosition);
    cam->SetFocalPoint(renInfo.CameraFocalPoint);
    cam->SetViewUp(renInfo.CameraViewUp);
    ren->InvokeEvent(vtkCommand::StartEvent, NULL);
    }
}

//----------------------------------------------------------------------------
// Method executed only on client.
void vtkClientCompositeManager::ReceiveAndSetColorBuffer()
{
  if (this->UseChar && ! this->UseRGB && this->SquirtLevel)
    {
    int length;
    this->ClientController->Receive(&length, 1, 1, 123450);
    this->SquirtArray->SetNumberOfTuples(length / (this->SquirtArray->GetNumberOfComponents()));
    this->ClientController->Receive((unsigned char*)(this->SquirtArray->GetVoidPointer(0)),
                                                    length, 1, 123451);
    this->SquirtDecompress(this->SquirtArray,
                           static_cast<vtkUnsignedCharArray*>(this->PData));
    //this->DeltaDecode(static_cast<vtkUnsignedCharArray*>(this->PData));
    }
  else
    {
    //this->ClientController->Receive(this->PData, 1, 123451);
    int length = this->PData->GetMaxId() + 1;
    this->ClientController->Receive((unsigned char*)(this->PData->GetVoidPointer(0)),
                                    length, 1, 123451);
    }
 
  /*
  if (this->InternalReductionFactor == 2)
    {
    vtkTimerLog::MarkStartEvent("Double Buffer");
    this->DoubleBuffer(this->PData, this->MagnifiedPData, this->PDataSize);
    vtkTimerLog::MarkEndEvent("Double Buffer");
    
    // I do not know if this is necessary !!!!!!!
    vtkRenderer* renderer =
      ((vtkRenderer*)
       this->RenderWindow->GetRenderers()->GetItemAsObject(0));
    renderer->SetViewport(0, 0, 1.0, 1.0);
    renderer->GetActiveCamera()->UpdateViewport(renderer);
    // We have to set the color buffer as an even multiple of factor.
    }
  else 
  */
  if (this->InternalReductionFactor > 1)
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
   
  this->CompositeData->Initialize();
  
  // Set the color buffer
  if (this->UseChar) 
    {
    vtkUnsignedCharArray* buf;
    if (this->InternalReductionFactor > 1)
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
//      this->RenderWindow->SetRGBACharPixelData(0, 0, 
//                            this->MagnifiedPDataSize[0]-1, 
//                            this->MagnifiedPDataSize[1]-1, buf, 0);
      vtkTimerLog::MarkEndEvent("Set RGBA Char Buffer");
      }
    else if (this->PData->GetNumberOfComponents() == 3)
      {
      vtkTimerLog::MarkStartEvent("Set RGB Char Buffer");
//      this->RenderWindow->SetPixelData(0, 0, 
//                            this->MagnifiedPDataSize[0]-1, 
//                            this->MagnifiedPDataSize[1]-1, buf, 0);
      vtkTimerLog::MarkEndEvent("Set RGB Char Buffer");
      }
    this->CompositeData->GetPointData()->SetScalars(buf);
    this->CompositeData->SetScalarType(VTK_UNSIGNED_CHAR);
    this->CompositeData->SetNumberOfScalarComponents(buf->GetNumberOfComponents());
    }
  else 
    {
    if (this->InternalReductionFactor)
      {
      vtkTimerLog::MarkStartEvent("Set RGBA Float Buffer");
//      this->RenderWindow->SetRGBAPixelData(0, 0, 
//                     this->MagnifiedPDataSize[0]-1, 
//                     this->MagnifiedPDataSize[1]-1,
//                     static_cast<vtkFloatArray*>(this->MagnifiedPData), 
//                     0);
      vtkTimerLog::MarkEndEvent("Set RGBA Float Buffer");
      this->CompositeData->SetNumberOfScalarComponents(
        this->MagnifiedPData->GetNumberOfComponents());
      }
    else
      {
      vtkTimerLog::MarkStartEvent("Set RGBA Float Buffer");
//      this->RenderWindow->SetRGBAPixelData(
//                   0, 0, this->MagnifiedPDataSize[0]-1, 
//                   this->MagnifiedPDataSize[1]-1,
//                   static_cast<vtkFloatArray*>(this->PData), 0);
      vtkTimerLog::MarkEndEvent("Set RGBA Float Buffer");
      this->CompositeData->SetNumberOfScalarComponents(
        this->PData->GetNumberOfComponents());
      }
    this->CompositeData->GetPointData()->SetScalars(this->PData);
    this->CompositeData->SetScalarType(VTK_FLOAT);
    }
  this->CompositeData->SetDimensions(this->MagnifiedPDataSize[0],
                                     this->MagnifiedPDataSize[1], 1);
}

void vtkClientCompositeManager::EndRender()
{
  if (this->CompositeData->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    return;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkRenderer *firstRen =
    static_cast<vtkRenderer*>(renderers->GetItemAsObject(0));
  
  vtkRenderer *ren = vtkRenderer::New();
  ren->SetRenderWindow(this->RenderWindow);
  ren->SetLayer(1);
  renderers->ReplaceItem(0, ren);
  
  vtkImageActor *imageActor = vtkImageActor::New();
  imageActor->SetInput(this->CompositeData);
  imageActor->SetDisplayExtent(0, this->MagnifiedPDataSize[0]-1,
                               0, this->MagnifiedPDataSize[1]-1, 0, 0);
  
  ren->AddProp(imageActor);

  vtkCamera *cam = ren->GetActiveCamera();
  cam->ParallelProjectionOn();
  cam->SetParallelScale(
    (this->MagnifiedPDataSize[1]-1)*0.5);
  ren->ResetCameraClippingRange();

  ren->SetBackground(firstRen->GetBackground());

  this->RenderWindow->Render();
  
  renderers->ReplaceItem(0, firstRen);
  
  ren->RemoveProp(imageActor);  
  imageActor->Delete();
  ren->Delete();
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
        for (yi = 0; yi < this->InternalReductionFactor; ++yi)
          {
          pp1 = rowp;
          for (x = 0; x < xInDim; x++)
            {
            // Duplicate the pixel p11 N times.
            for (xi = 0; xi < this->InternalReductionFactor; ++xi)
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
        for (yi = 0; yi < this->InternalReductionFactor; ++yi)
          {
          cpp1 = crowp;
          for (x = 0; x < xInDim; x++)
            {
            // Duplicate the pixel p11 N times.
            for (xi = 0; xi < this->InternalReductionFactor; ++xi)
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
      for (yi = 0; yi < this->InternalReductionFactor; ++yi)
        {
        pp1 = rowp;
        for (x = 0; x < xInDim; x++)
          {
          // Duplicate the pixel p11 N times.
          for (xi = 0; xi < this->InternalReductionFactor; ++xi)
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
  

//----------------------------------------------------------------------------
// We change this to work backwards so we can make it inplace. !!!!!!!     
void vtkClientCompositeManager::DoubleBuffer(vtkDataArray* localP, 
                                             vtkDataArray* magP,
                                             int windowSize[2])
{
  int   x, y, i;
  int   xInDim, yInDim;
  // Local increments for input.
  int   outYInc; 
  unsigned char *localPdata;
  unsigned char *newLocalPData;
  unsigned char half, quarter;
  
  xInDim = windowSize[0];
  yInDim = windowSize[1];

  outYInc = 2 * xInDim * 4;

  if (this->InternalReductionFactor != 2)
    {
    vtkErrorMacro("double does not match reduction factor.");
    return;
    }
  // Assume rgba char.
  if (localP->GetNumberOfComponents() != 4)
    {
    vtkErrorMacro("Expecting RGBA");
    return;
    }
  if ( ! this->UseChar)
    {
    vtkErrorMacro("Expecting char data");
    return;
    }

  newLocalPData = reinterpret_cast<unsigned char*>(magP->GetVoidPointer(0));
  memset(newLocalPData, 0, magP->GetMaxId()+1);
  localPdata = reinterpret_cast<unsigned char*>(localP->GetVoidPointer(0));

  for (y = 0; y < yInDim; y++)
    {
    for (x = 0; x < xInDim; x++)
      {
      for (i = 0; i < 4; ++i)
        {
        half = *localPdata >> 1;
        quarter = half >> 1;
        newLocalPData[0] = *localPdata;
        newLocalPData[4] += half;
        newLocalPData[outYInc] += half;
        newLocalPData[outYInc+4] += quarter;
        if (x > 0)
          {
          newLocalPData[-4] += half;
          newLocalPData[outYInc-4] += quarter;
          if (y > 0)
            {
            newLocalPData[-outYInc-4] += quarter;
            }
          }
        if (y > 0)
          {
          newLocalPData[-outYInc] += half;
          newLocalPData[-outYInc+4] += quarter;
          }
      
        ++localPdata;
        ++newLocalPData;
        }
      newLocalPData += 4;
      }
    newLocalPData += outYInc;
   }
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
  int renIdx;
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
  //controller->Receive((char*)(&winInfo), 
  //                    sizeof(struct vtkClientRenderWindowInfo), 
  //                    otherId, vtkCompositeManager::WIN_INFO_TAG);
  controller->Receive((int*)(&winInfo), 5, otherId, 
                      vtkCompositeManager::WIN_INFO_TAG);
  if (myId == 0)
    {  // Relay info to server satellite processes.
    for (j = 1; j < numProcs; ++j)
      {
      //this->CompositeController->Send((char*)(&winInfo), 
      //                sizeof(struct vtkClientRenderWindowInfo), j,
      //                vtkCompositeManager::WIN_INFO_TAG);
      this->CompositeController->Send((float*)(&winInfo), 5, j,
                      vtkCompositeManager::WIN_INFO_TAG);
      }
    }


  // This should be fixed.  Round off will cause window to resize. !!!!!
  renWin->SetSize(winInfo.Size[0] * winInfo.ImageReductionFactor,
                  winInfo.Size[1] * winInfo.ImageReductionFactor);
  this->InternalReductionFactor = winInfo.ImageReductionFactor;
  this->SquirtLevel = winInfo.SquirtLevel;

  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  // This is misleading.  We do not support multiple renderers.
//  for (renIdx = 0; renIdx < winInfo.NumberOfRenderers; ++renIdx)
  for (renIdx = 0; renIdx < 1; ++renIdx) // only consider 1st renderer
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

    //controller->Receive((char*)(&renInfo), 
    //                    sizeof(struct vtkClientRendererInfo), 
    //                    otherId, vtkCompositeManager::REN_INFO_TAG);
    controller->Receive((float*)(&renInfo), 22, otherId, 
                        vtkCompositeManager::REN_INFO_TAG);
    if (myId == 0)
      {  // Relay info to server satellite processes.
      for (j = 1; j < numProcs; ++j)
        {
        //this->CompositeController->Send((char*)(&renInfo), 
        //                sizeof(struct vtkClientRendererInfo), 
        //                j, vtkCompositeManager::REN_INFO_TAG);
        this->CompositeController->Send((float*)(&renInfo), 22, j, 
                                        vtkCompositeManager::REN_INFO_TAG);
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

      if (this->Tiled)
        {
        int x, y;
        // Figure out the tile indexes.
        i = this->CompositeController->GetLocalProcessId() - 1;
        y = i/this->TiledDimensions[0];
        x = i - y*this->TiledDimensions[0];

        cam->SetWindowCenter(1.0-(double)(this->TiledDimensions[0]) + 2.0*(double)x,
                             1.0-(double)(this->TiledDimensions[1]) + 2.0*(double)y);
        cam->SetViewAngle(asin(sin(renInfo.CameraViewAngle*3.1415926/360.0)/(double)(this->TiledDimensions[0])) * 360.0 / 3.1415926);
        cam->SetPosition(renInfo.CameraPosition);
        cam->SetFocalPoint(renInfo.CameraFocalPoint);
        cam->SetViewUp(renInfo.CameraViewUp);
        cam->SetClippingRange(renInfo.CameraClippingRange);
        if (renInfo.ParallelScale != 0.0)
          {
          cam->ParallelProjectionOn();
          cam->SetParallelScale(renInfo.ParallelScale/(double)(this->TiledDimensions[0]));
          }
        else
          {
          cam->ParallelProjectionOff();   
          }
        }
      else
        { // Not tiled display.
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
        }
      if (light)
        {
        light->SetPosition(renInfo.LightPosition);
        light->SetFocalPoint(renInfo.LightFocalPoint);
        }
      ren->SetBackground(renInfo.Background);
      // Should not have reduction when using tiled display.
      ren->SetViewport(0, 0, 1.0/(float)this->InternalReductionFactor, 
                       1.0/(float)this->InternalReductionFactor);
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

  if (this->Tiled)
    { // Do not need to composite if we are rendering a tiled display. 
    return;
    }

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
    if (this->UseChar && ! this->UseRGB && this->SquirtLevel)
      {
      //this->DeltaEncode(static_cast<vtkUnsignedCharArray*>(this->PData));
      this->SquirtCompress(static_cast<vtkUnsignedCharArray*>(this->PData),
                           this->SquirtArray, this->SquirtLevel);
      int length = this->SquirtArray->GetMaxId() + 1;
      this->ClientController->Send(&length, 1, 1, 123450);
      this->ClientController->Send((unsigned char*)(this->SquirtArray->GetVoidPointer(0)),
                                                    length, 1, 123451);
      }
    else
      {
      //this->ClientController->Send(this->PData, 1, 123451);
      int length = this->PData->GetMaxId() + 1;
      this->ClientController->Send((unsigned char*)(this->PData->GetVoidPointer(0)),
                                                    length, 1, 123451);
      }
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
//      this->RenderWindow->RemoveObserver(this->StartTag);
      // Will make do with first renderer. (Assumes renderer does not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
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
//      this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
      cbc->Delete();

      // Will make do with first renderer. (Assumes renderer does
      // not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
      }
     if (this->Tiled && this->ClientFlag == 0)
      { 
      renWin->FullScreenOn();
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
  if (this->SquirtArray)
    {
    vtkCompositeManager::DeleteArray(this->SquirtArray);
    this->SquirtArray = NULL;
    } 
  if (this->MagnifiedPData)
    {
    vtkCompositeManager::DeleteArray(this->MagnifiedPData);
    this->MagnifiedPData = NULL;
    } 

  // Allocate squirt compressed array.
  if (this->UseChar && ! this->UseRGB)
    {
    if (this->ClientFlag || this->CompositeController->GetLocalProcessId() == 0)
      {
      if (this->SquirtArray == NULL)
        {
        this->SquirtArray = vtkUnsignedCharArray::New();
        }
      vtkCompositeManager::ResizeUnsignedCharArray(
          this->SquirtArray, 4, numTuples);
      }
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
  this->MagnifiedPDataSize[0] = x * this->InternalReductionFactor;
  this->MagnifiedPDataSize[1] = y * this->InternalReductionFactor;

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
    if (this->SquirtArray)
      {
      vtkCompositeManager::DeleteArray(this->SquirtArray);
      this->SquirtArray = NULL;
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


  // Allocate squirt compressed array.
  if (this->UseChar && ! this->UseRGB)
    {
    if (this->ClientFlag || this->CompositeController->GetLocalProcessId() == 0)
      {
      if ( this->SquirtArray == NULL)
        {
        this->SquirtArray = vtkUnsignedCharArray::New();
        }
      vtkCompositeManager::ResizeUnsignedCharArray(
          this->SquirtArray, 4, numPixels);
      }
    }

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
    this->ClientController->AddRMI(vtkClientCompositeManagerGatherZBufferValueRMI, 
                                   (void*)this, 
                                   vtkClientCompositeManager::GATHER_Z_RMI_TAG); 
    }
  else
    { // Other satellite processes wait for RMIs for root.
    this->CompositeController->AddRMI(vtkClientCompositeManagerRenderRMI, (void*)this, 
                                      vtkClientCompositeManager::RENDER_RMI_TAG); 
    this->CompositeController->AddRMI(vtkClientCompositeManagerGatherZBufferValueRMI, 
                                      (void*)this, 
                                      vtkClientCompositeManager::GATHER_Z_RMI_TAG); 
    }
}

//-------------------------------------------------------------------------
void vtkClientCompositeManager::SquirtCompress(vtkUnsignedCharArray *in,
                                               vtkUnsignedCharArray *out,
                                               int compress_level)
{

  if (in->GetNumberOfComponents() != 4)
    {
    vtkErrorMacro("Squirt only works with RGBA");
    return;
    }

  int count=0;
  int index=0;
  int comp_index=0;
  int end_index;
  unsigned int current_color;
  unsigned char compress_masks[6][4] = {  {0xFF, 0xFF, 0xFF, 0xFF},
                                          {0xFE, 0xFF, 0xFE, 0xFF},
                                          {0xFC, 0xFE, 0xFC, 0xFF},
                                          {0xF8, 0xFC, 0xF8, 0xFF},
                                          {0xF0, 0xF8, 0xF0, 0xFF},
                                          {0xE0, 0xF0, 0xE0, 0xFF}};

  // Set bitmask based on compress_level
  unsigned int compress_mask;
  memcpy(&compress_mask, &compress_masks[compress_level], 4);

  // Access raw arrays directly
  unsigned int* _rawColorBuffer;
  unsigned int* _rawCompressedBuffer;
  int numPixels = in->GetNumberOfTuples();
  _rawColorBuffer = (unsigned int*)in->GetPointer(0);
  _rawCompressedBuffer = (unsigned int*)out->WritePointer(0,numPixels*4);
  end_index = numPixels;

  // Go through color buffer and put RLE format into compressed buffer
  while((index < end_index) && (comp_index < end_index)) 
    {
                
    // Record color
    current_color = _rawCompressedBuffer[comp_index] =_rawColorBuffer[index];
    index++;

    // Compute Run
    while(((current_color&compress_mask) == (_rawColorBuffer[index]&compress_mask)) &&
          (index<end_index) && (count<255))
      { 
      index++; count++;   
      }

    // Record Run length
    *((unsigned char*)_rawCompressedBuffer+comp_index*4+3) =(unsigned char)count;
    comp_index++;

    count = 0;
    
    }

    // Back to vtk arrays :)
    out->SetNumberOfTuples(comp_index);
 
}

//------------------------------------------------------------
void vtkClientCompositeManager::SquirtDecompress(vtkUnsignedCharArray *in,
                                                  vtkUnsignedCharArray *out)
{
  int count=0;
  int index=0;
  unsigned int current_color;
  unsigned int* _rawColorBuffer;
  unsigned int* _rawCompressedBuffer;

  // Get compressed buffer size
  int CompSize = in->GetNumberOfTuples();

  // Access raw arrays directly
  _rawColorBuffer = (unsigned int*)out->GetPointer(0);
  _rawCompressedBuffer = (unsigned int*)in->GetPointer(0);

  // Go through compress buffer and extract RLE format into color buffer
  for(int i=0; i<CompSize; i++)
    {
    // Get color and count
    current_color = _rawCompressedBuffer[i];

    // Get run length count;
    count = *((unsigned char*)&current_color+3);

    // Fixed Alpha
    *((unsigned char*)&current_color+3) = 0xFF;

    // Set color
    _rawColorBuffer[index++] = current_color;

    // Blast color into color buffer
    for(int j=0; j< count; j++)
      _rawColorBuffer[index++] = current_color;
    }

    // Save out compression stats.
    vtkTimerLog::FormatAndMarkEvent("Squirt ratio: %f", (float)CompSize/(float)index);
}


//-------------------------------------------------------------------------
void vtkClientCompositeManager::DeltaEncode(vtkUnsignedCharArray *buf)
{
  int idx;
  int numPixels = buf->GetNumberOfTuples();
  unsigned char* ptr1;
  unsigned char* ptr2;
  short a, b, c;

  if (this->BaseArray == NULL)
    {
    this->BaseArray = vtkUnsignedCharArray::New();
    this->BaseArray->SetNumberOfComponents(4);
    this->BaseArray->SetNumberOfTuples(numPixels);
    ptr1 = this->BaseArray->GetPointer(0);
    memset(ptr1, 0, numPixels*4);
    }
  if (this->BaseArray->GetNumberOfTuples() != numPixels)
    {
    this->BaseArray->SetNumberOfTuples(numPixels);
    ptr1 = this->BaseArray->GetPointer(0);
    memset(ptr1, 0, numPixels*4);
    }
  ptr1 = this->BaseArray->GetPointer(0);  
  ptr2 = buf->GetPointer(0);
  for (idx = 0; idx < numPixels; ++idx)
    {
    a = ptr1[0];
    b = ptr2[0];
    c = b-a + 256;
    c = c >> 1;
    if (c > 255) {c = 255;}
    if (c < 0) {c = 0;} 
    ptr2[0] = (unsigned char)(c);
    c = c << 1;
    ptr1[0] = (unsigned char)(c + a - 255);

    a = ptr1[1];
    b = ptr2[1];
    c = b-a + 256;
    c = c >> 1;
    if (c > 255) {c = 255;}
    if (c < 0) {c = 0;}
    ptr2[1] = (unsigned char)(c);
    c = c << 1;
    ptr1[1] = (unsigned char)(c + a - 255);

    a = ptr1[2];
    b = ptr2[2];
    c = b-a + 256;
    c = c >> 1;
    if (c > 255) {c = 255;}
    if (c < 0) {c = 0;}
    ptr2[2] = (unsigned char)(c);
    c = c << 1;
    ptr1[2] = (unsigned char)(c + a - 255);

    ptr1 += 4;
    ptr2 += 4;
    }
}

//-------------------------------------------------------------------------
void vtkClientCompositeManager::DeltaDecode(vtkUnsignedCharArray *buf)
{
  int idx;
  int numPixels = buf->GetNumberOfTuples();
  unsigned char* ptr1;
  unsigned char* ptr2;
  short dif;

  if (this->BaseArray == NULL)
    {
    this->BaseArray = vtkUnsignedCharArray::New();
    this->BaseArray->SetNumberOfComponents(4);
    this->BaseArray->SetNumberOfTuples(numPixels);
    ptr1 = this->BaseArray->GetPointer(0);
    memset(ptr1, 0, numPixels*4);
    }
  if (this->BaseArray->GetNumberOfTuples() != numPixels)
    {
    this->BaseArray->SetNumberOfTuples(numPixels);
    ptr1 = this->BaseArray->GetPointer(0);
    memset(ptr1, 0, numPixels*4);
    }
  ptr1 = this->BaseArray->GetPointer(0);  
  ptr2 = buf->GetPointer(0);
  for (idx = 0; idx < numPixels; ++idx)
    {
    dif = (short)(ptr2[0]);
    dif = dif << 1;
    dif = dif + (short)(ptr1[0]) - 255;
    ptr2[0] = ptr1[0] = (unsigned char)(dif);

    dif = (short)(ptr2[1]);
    dif = dif << 1;
    dif = dif + (short)(ptr1[1]) - 255;
    ptr2[1] = ptr1[1] = (unsigned char)(dif);

    dif = (short)(ptr2[2]);
    dif = dif << 1;
    dif = dif + (short)(ptr1[2]) - 255;
    ptr2[2] = ptr1[2] = (unsigned char)(dif);

    ptr1 += 4;
    ptr2 += 4;
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
  os << indent << "ImageReductionFactor: " 
     << this->ImageReductionFactor << endl;
  
  os << indent << "CompositeController: (" 
     << this->CompositeController << ")\n"; 
  os << indent << "ClientController: (" << this->ClientController << ")\n"; 

  if (this->Tiled)
    {
    os << indent << "Tiled display with dimensions: " 
       << this->TiledDimensions[0] << ", " << this->TiledDimensions[1] << endl;
    }
  
  os << indent << "UseCompositing: " << this->UseCompositing << endl;
  os << indent << "UseChar: " << this->UseChar << endl;
  os << indent << "UseRGB: " << this->UseRGB << endl;
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;
  os << indent << "ClientFlag: " << this->ClientFlag << endl;

  os << indent << "Compositer: " << this->Compositer << endl;

}



