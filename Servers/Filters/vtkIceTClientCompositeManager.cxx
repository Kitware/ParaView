/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTClientCompositeManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

#include "vtkIceTClientCompositeManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkSocketController.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkTimerLog.h"
#include "vtkByteSwap.h"
#include "vtkIceTRenderManager.h"


#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#elif defined(VTK_USE_MESA)
#include "vtkMesaRenderWindow.h"
#endif


vtkCxxRevisionMacro(vtkIceTClientCompositeManager, "1.13");
vtkStandardNewMacro(vtkIceTClientCompositeManager);

vtkCxxSetObjectMacro(vtkIceTClientCompositeManager,IceTManager,vtkIceTRenderManager);
vtkCxxSetObjectMacro(vtkIceTClientCompositeManager,CompositeController,vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkIceTClientCompositeManager,ClientController,vtkSocketController);

// Structures to communicate render info.
struct vtkClientRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
  int ImageReductionFactor;
  int UseCompositing;
};

struct vtkClientRendererInfo 
{
  double CameraPosition[3];
  double CameraFocalPoint[3];
  double CameraViewUp[3];
  double CameraClippingRange[2];
  double LightPosition[3];
  double LightFocalPoint[3];
  double Background[3];
  double ParallelScale;
  double CameraViewAngle;
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
vtkIceTClientCompositeManager::vtkIceTClientCompositeManager()
{
  this->RenderWindow = NULL;

  this->ClientController = NULL;
  this->ClientFlag = 1;

  this->StartTag = 0;
  this->RenderView = NULL;

  this->ImageReductionFactor = 2;
  this->RenderView = NULL;

  this->Tiled = 1;
  this->TiledDimensions[0] = this->TiledDimensions[1] = 1;


  this->UseCompositing = 0;
  this->IceTManager = NULL;
}

  
//-------------------------------------------------------------------------
vtkIceTClientCompositeManager::~vtkIceTClientCompositeManager()
{
  this->SetRenderWindow(NULL);
  
  this->SetClientController(NULL);

  this->SetRenderView(NULL);

  this->SetIceTManager(NULL);
}


//----------------------------------------------------------------------------
// Called only on the client.
float vtkIceTClientCompositeManager::GetZBufferValue(int x, int y)
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
                                vtkIceTClientCompositeManager::GATHER_Z_RMI_TAG);
  this->ClientController->Receive(&z, 1, 1, vtkIceTClientCompositeManager::CLIENT_Z_TAG);
  return z;
}

//----------------------------------------------------------------------------
void vtkIceTClientCompositeManagerGatherZBufferValueRMI(void *local, void *pArg, 
                                                    int pLength, int)
{
  vtkIceTClientCompositeManager* self = (vtkIceTClientCompositeManager*)local;
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
void vtkIceTClientCompositeManager::GatherZBufferValueRMI(int , int ){}



//=======================  Client ========================



//-------------------------------------------------------------------------
// We may want to pass the render window as an argument for a sanity check.
void vtkIceTClientCompositeManagerStartRender(vtkObject *,
                                              unsigned long, 
                                              void *clientData, 
                                              void *)
{
  vtkIceTClientCompositeManager *self = (vtkIceTClientCompositeManager *)clientData;
  
  self->StartRender();
}

//-------------------------------------------------------------------------
// Only called in process 0.
void vtkIceTClientCompositeManager::StartRender()
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
  //float updateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  if (firstRender)
    {
    firstRender = 0;
    return;
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
  winInfo.Size[0] = size[0]/this->ImageReductionFactor;
  winInfo.Size[1] = size[1]/this->ImageReductionFactor;
  winInfo.ImageReductionFactor = this->ImageReductionFactor;
  winInfo.UseCompositing = this->UseCompositing;
//  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfo.NumberOfRenderers = 1;
  
  controller->TriggerRMI(1, vtkIceTClientCompositeManager::RENDER_RMI_TAG);

  // Synchronize the size of the windows.
  //controller->Send((char*)(&winInfo), 
  //                 sizeof(vtkClientRenderWindowInfo), 1, 
  //                 vtkIceTClientCompositeManager::WIN_INFO_TAG);
  // Let the socket controller deal with byte swapping.
  controller->Send((int*)(&winInfo), 5, 1, 
                   vtkIceTClientCompositeManager::WIN_INFO_TAG);
  
  // Make sure the satellite renderers have the same camera I do.
  // Note: This will lockup unless every process has the same number
  // of renderers.
  rens->InitTraversal();
//  while ( (ren = rens->GetNextItem()) )
//    {
  ren = rens->GetNextItem();
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
    // Let the socket controller deal with byte swapping.
    controller->Send((double*)(&renInfo), 
                     sizeof(struct vtkClientRendererInfo)/sizeof(double), 
                     1, vtkIceTClientCompositeManager::REN_INFO_TAG);
//    }
  int i = 0;
  controller->Receive(&i, 1, 1, vtkIceTClientCompositeManager::ACKNOWLEDGE_RMI);
}




//=======================  Server ========================




//----------------------------------------------------------------------------
void vtkIceTClientCompositeManagerRenderRMI(void *arg, void *, int, int)
{
  vtkIceTClientCompositeManager* self = (vtkIceTClientCompositeManager*) arg;
  
  self->RenderRMI();
}

//----------------------------------------------------------------------------
// Only Called by the satellite processes.
void vtkIceTClientCompositeManager::RenderRMI()
{
  if (this->ClientFlag)
    {
    vtkErrorMacro("Expecting the server side to call this method.");
    return;
    }

  // Get and redistribute renderer information (camera ...)
  this->SatelliteStartRender();
  this->RenderWindow->Render();
  int i = 837;
  this->ClientController->Send(&i, 1, 1, vtkIceTClientCompositeManager::ACKNOWLEDGE_RMI);
}

//-------------------------------------------------------------------------
void vtkIceTClientCompositeManager::SatelliteStartRender()
{
  int renIdx;
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

    controller = this->ClientController;
    otherId = 1;
  
  vtkInitializeClientRendererInfoMacro(renInfo);
  
  // Receive the window size.
  controller->Receive((int*)(&winInfo), 5, otherId, 
                      vtkIceTClientCompositeManager::WIN_INFO_TAG);

  this->ImageReductionFactor = winInfo.ImageReductionFactor;
  this->UseCompositing = winInfo.UseCompositing;
  if (this->IceTManager)
    {
    this->IceTManager->SetImageReductionFactor(this->ImageReductionFactor);
    this->IceTManager->SetUseCompositing(this->UseCompositing);
    }

  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  // This is misleading.  We do not support multiple renderers.
  for (renIdx = 0; renIdx < winInfo.NumberOfRenderers; ++renIdx)
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
    controller->Receive((double*)(&renInfo), 
                        sizeof(struct vtkClientRendererInfo)/sizeof(double),
                        otherId, vtkIceTClientCompositeManager::REN_INFO_TAG);
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
        cam->SetViewAngle(renInfo.CameraViewAngle);
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
      }
    }
}

//-------------------------------------------------------------------------
void vtkIceTClientCompositeManager::SatelliteEndRender()
{
}



//-------------------------------------------------------------------------
void vtkIceTClientCompositeManager::InitializeOffScreen()
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
void vtkIceTClientCompositeManager::SetRenderWindow(vtkRenderWindow *renWin)
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
      // Will make do with first renderer. (Assumes renderer does
      // not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
      if (ren)
        {
        ren->RemoveObserver(this->StartTag);
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
      // Will make do with first renderer. (Assumes renderer does
      // not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();

      if (ren)
        {
        vtkCallbackCommand *cbc;
      
        cbc= vtkCallbackCommand::New();
        cbc->SetCallback(vtkIceTClientCompositeManagerStartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartTag = ren->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
        }
      
      }
     if (this->Tiled && this->ClientFlag == 0)
      { 
      renWin->FullScreenOn();
      }
    }
}


//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkIceTClientCompositeManager::InitializeRMIs()
{
  if (this->ClientFlag)
    { // Just in case.
    return;
    }
  if (this->ClientController == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }
  this->ClientController->AddRMI(vtkIceTClientCompositeManagerRenderRMI, (void*)this, 
    vtkIceTClientCompositeManager::RENDER_RMI_TAG); 
  this->ClientController->AddRMI(vtkIceTClientCompositeManagerGatherZBufferValueRMI, 
    (void*)this, 
    vtkIceTClientCompositeManager::GATHER_Z_RMI_TAG); 
}





//----------------------------------------------------------------------------
void vtkIceTClientCompositeManager::PrintSelf(ostream& os, vtkIndent indent)
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
  
  os << indent << "CompositeController: (" << this->CompositeController << ")\n"; 
  os << indent << "ClientController: (" << this->ClientController << ")\n"; 

  if (this->Tiled)
    {
    os << indent << "Tiled display with dimensions: " 
       << this->TiledDimensions[0] << ", " << this->TiledDimensions[1] << endl;
    }
  
  os << indent << "UseCompositing: " << this->UseCompositing << endl;
  os << indent << "ClientFlag: " << this->ClientFlag << endl;
  os << indent << "UseCompositeCompression: " << this->UseCompositeCompression << endl;

  if (this->IceTManager)
    {
    os << indent << "IceTManager: " << this->IceTManager << endl;
    }
}



