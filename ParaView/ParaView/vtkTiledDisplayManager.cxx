/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTiledDisplayManager.cxx
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

#include "vtkTiledDisplayManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkTiledDisplayManager, "1.5");
vtkStandardNewMacro(vtkTiledDisplayManager);

vtkCxxSetObjectMacro(vtkTiledDisplayManager, RenderView, vtkObject);

// Structures to communicate render info.
struct vtkTiledDisplayRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
  float DesiredUpdateRate;
};

struct vtkTiledDisplayRendererInfo 
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
#define vtkInitializeTiledDisplayRendererInfoMacro(r)      \
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
vtkTiledDisplayManager::vtkTiledDisplayManager()
{
  this->RenderWindow = NULL;
  this->RenderWindowInteractor = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();

  if (this->Controller)
    {
    this->Controller->Register(this);
    }

  this->StartTag = this->EndTag = 0;
  this->StartInteractorTag = 0;
  this->EndInteractorTag = 0;
  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;
  this->RenderView = NULL;
}

  
//-------------------------------------------------------------------------
vtkTiledDisplayManager::~vtkTiledDisplayManager()
{
  this->SetRenderWindow(NULL);
  
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }
}

//-------------------------------------------------------------------------
// We may want to pass the render window as an argument for a sanity check.
void vtkTiledDisplayManagerStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkTiledDisplayManager *self = (vtkTiledDisplayManager *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->StartRender();
}

//-------------------------------------------------------------------------
void vtkTiledDisplayManagerEndRender(vtkObject *caller,
                                  unsigned long vtkNotUsed(event), 
                                  void *clientData, void *)
{
  vtkTiledDisplayManager *self = (vtkTiledDisplayManager *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->EndRender();
}


//-------------------------------------------------------------------------
void vtkTiledDisplayManagerExitInteractor(vtkObject *vtkNotUsed(o),
                                       unsigned long vtkNotUsed(event), 
                                       void *clientData, void *)
{
  vtkTiledDisplayManager *self = (vtkTiledDisplayManager *)clientData;

  self->ExitInteractor();
}

//----------------------------------------------------------------------------
void vtkTiledDisplayManagerRenderRMI(void *arg, void *, int, int)
{
  vtkTiledDisplayManager* self = (vtkTiledDisplayManager*) arg;
  
  self->RenderRMI();
}


//-------------------------------------------------------------------------
// Only process 0 needs start and end render callbacks.
void vtkTiledDisplayManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (this->Controller && this->Controller->GetLocalProcessId() == 0)
      {
      this->RenderWindow->RemoveObserver(this->StartTag);
      this->RenderWindow->RemoveObserver(this->EndTag);
      }
    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    this->SetRenderWindowInteractor(NULL);
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    this->SetRenderWindowInteractor(renWin->GetInteractor());
    if (this->Controller)
      {
      if (this->Controller && this->Controller->GetLocalProcessId() == 0)
        {
        vtkCallbackCommand *cbc;
        
        cbc= vtkCallbackCommand::New();
        cbc->SetCallback(vtkTiledDisplayManagerStartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(vtkTiledDisplayManagerEndRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->EndTag = renWin->AddObserver(vtkCommand::EndEvent,cbc);
        cbc->Delete();
        }
      else
        {
        renWin->FullScreenOn();
        }
      }
    }
}


//-------------------------------------------------------------------------
void vtkTiledDisplayManager::SetController(vtkMultiProcessController *mpc)
{
  if (this->Controller == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    }
  this->Controller = mpc;
}

//-------------------------------------------------------------------------
// Only satellite processes process interactor loops specially.
// We only setup callbacks in those processes (not process 0).
void vtkTiledDisplayManager::SetRenderWindowInteractor(
                                           vtkRenderWindowInteractor *iren)
{
  if (this->RenderWindowInteractor == iren)
    {
    return;
    }

  if (this->Controller == NULL)
    {
    return;
    }
  
  if (this->RenderWindowInteractor)
    {
    if (!this->Controller->GetLocalProcessId())
      {
      this->RenderWindowInteractor->RemoveObserver(this->EndInteractorTag);
      }
    this->RenderWindowInteractor->UnRegister(this);
    this->RenderWindowInteractor =  NULL;
    }
  if (iren)
    {
    iren->Register(this);
    this->RenderWindowInteractor = iren;
    
    if (!this->Controller->GetLocalProcessId())
      {
      vtkCallbackCommand *cbc;
      cbc= vtkCallbackCommand::New();
      cbc->SetCallback(vtkTiledDisplayManagerExitInteractor);
      cbc->SetClientData((void*)this);
      // IRen will delete the cbc when the observer is removed.
      this->EndInteractorTag = iren->AddObserver(vtkCommand::ExitEvent,cbc);
      cbc->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkTiledDisplayManager::RenderRMI()
{
  // Start and end methods take care of synchronization and compositing
  vtkRenderWindow* renWin = this->RenderWindow;

  // Delay swapping buffers untill all processes are finished.
  if (this->Controller)
    {
    renWin->SwapBuffersOff();  
    }

  this->SatelliteStartRender();
  renWin->Render();

  // Force swap buffers here.
  if (this->Controller)
    {
    this->Controller->Barrier();
    renWin->SwapBuffersOn();  
    renWin->Frame();
    }
}


//-------------------------------------------------------------------------
void vtkTiledDisplayManager::SatelliteStartRender()
{
  int i;
  vtkTiledDisplayRenderWindowInfo winInfo;
  vtkTiledDisplayRendererInfo renInfo;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam = 0;
  vtkLightCollection *lc;
  vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;
  
  vtkInitializeTiledDisplayRendererInfoMacro(renInfo);
  // Initialize to get rid of a warning.
  winInfo.Size[0] = winInfo.Size[1] = 0;
  winInfo.NumberOfRenderers = 1;
  winInfo.DesiredUpdateRate = 10.0;

  // Receive the window size.
  controller->Receive((char*)(&winInfo), 
                      sizeof(struct vtkTiledDisplayRenderWindowInfo), 0, 
                      vtkTiledDisplayManager::WIN_INFO_TAG);
  renWin->SetDesiredUpdateRate(winInfo.DesiredUpdateRate);

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
                        sizeof(struct vtkTiledDisplayRendererInfo), 
                        0, vtkTiledDisplayManager::REN_INFO_TAG);
    if (ren == NULL)
      {
      vtkErrorMacro("Renderer mismatch.");
      }
    else
      {
      lc = ren->GetLights();
      lc->InitTraversal();
      light = lc->GetNextItem();
      int i, x, y;

      // Figure out the tile indexes.
      i = this->Controller->GetLocalProcessId() - 1;
      y = i/this->TileDimensions[0];
      x = i - y*this->TileDimensions[0];

      cam->SetWindowCenter(1.0-(double)(this->TileDimensions[0]) + 2.0*(double)x,
                           1.0-(double)(this->TileDimensions[1]) + 2.0*(double)y);
      cam->SetViewAngle(asin(sin(renInfo.CameraViewAngle*3.1415926/360.0)/(double)(this->TileDimensions[0])) * 360.0 / 3.1415926);
      cam->SetPosition(renInfo.CameraPosition);
      cam->SetFocalPoint(renInfo.CameraFocalPoint);
      cam->SetViewUp(renInfo.CameraViewUp);
      cam->SetClippingRange(renInfo.CameraClippingRange);
      if (renInfo.ParallelScale != 0.0)
        {
        cam->ParallelProjectionOn();
        cam->SetParallelScale(renInfo.ParallelScale/(double)(this->TileDimensions[0]));
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
void vtkTiledDisplayManager::SatelliteEndRender()
{  
  // Swap buffers.
}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkTiledDisplayManager::InitializeRMIs()
{
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  this->Controller->AddRMI(vtkTiledDisplayManagerRenderRMI, (void*)this, 
                           vtkTiledDisplayManager::RENDER_RMI_TAG); 

}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkTiledDisplayManager::StartInteractor()
{
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  this->InitializeRMIs();

  if (!this->Controller->GetLocalProcessId())
    {
    if (!this->RenderWindowInteractor)
      {
      vtkErrorMacro("Missing interactor.");
      this->ExitInteractor();
      return;
      }
    this->RenderWindowInteractor->Initialize();
    this->RenderWindowInteractor->Start();
    }
  else
    {
    this->Controller->ProcessRMIs();
    }
}

//-------------------------------------------------------------------------
// This is only called in process 0.
void vtkTiledDisplayManager::ExitInteractor()
{
  int numProcs, id;
  
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  numProcs = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    this->Controller->TriggerRMI(id, 
                                 vtkMultiProcessController::BREAK_RMI_TAG);
    }
}


//-------------------------------------------------------------------------
// Only called in process 0.
void vtkTiledDisplayManager::StartRender()
{
  struct vtkTiledDisplayRenderWindowInfo winInfo;
  struct vtkTiledDisplayRendererInfo renInfo;
  int id, numProcs;
  int *size;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  
  vtkDebugMacro("StartRender");
  
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;

  if (controller == NULL)
    {
    return;
    }

  // Make sure they all swp buffers at the same time.
  renWin->SwapBuffersOff();

  // Trigger the satellite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  numProcs = this->Controller->GetNumberOfProcesses();
  size = this->RenderWindow->GetSize();
  winInfo.Size[0] = size[0];
  winInfo.Size[1] = size[1];
  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfo.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  for (id = 1; id < numProcs; ++id)
    {
    controller->TriggerRMI(id, NULL, 0, 
                           vtkTiledDisplayManager::RENDER_RMI_TAG);

    // Synchronize the size of the windows.
    controller->Send((char*)(&winInfo), 
                     sizeof(vtkTiledDisplayRenderWindowInfo), id, 
                     vtkTiledDisplayManager::WIN_INFO_TAG);
    }
  
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
    
    for (id = 1; id < numProcs; ++id)
      {
      controller->Send((char*)(&renInfo),
                       sizeof(struct vtkTiledDisplayRendererInfo), id, 
                       vtkTiledDisplayManager::REN_INFO_TAG);
      }
    }
  
  // Turn swap buffers off before the render so the end render method
  // has a chance to add to the back buffer.
  //renWin->SwapBuffersOff();
}

//-------------------------------------------------------------------------
void vtkTiledDisplayManager::EndRender()
{
  vtkRenderWindow* renWin = this->RenderWindow;
  
  // Force swap buffers here.
  if (this->Controller)
    {
    this->Controller->Barrier();
    renWin->SwapBuffersOn();  
    renWin->Frame();
    }
}


//----------------------------------------------------------------------------
void vtkTiledDisplayManager::PrintSelf(ostream& os, vtkIndent indent)
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
  
  os << indent << "Tile Dimensions: " << this->TileDimensions[0] << ", "
     << this->TileDimensions[1] << endl;

  os << indent << "Controller: (" << this->Controller << ")\n"; 
}



