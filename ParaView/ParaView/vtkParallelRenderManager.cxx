/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelRenderManager.cxx
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

#include "vtkParallelRenderManager.h"

#include <vtkMultiProcessController.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkCallbackCommand.h>
#include <vtkActorCollection.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkCamera.h>
#include <vtkLightCollection.h>
#include <vtkLight.h>
#include <vtkUnsignedCharArray.h>
#include <vtkFloatArray.h>
#include <vtkTimerLog.h>

static void AbortRenderCheck(vtkObject *caller, unsigned long vtkNotUsed(event),
           void *clientData, void *);
static void StartRender(vtkObject *caller, unsigned long vtkNotUsed(event),
      void *clientData, void *);
static void EndRender(vtkObject *caller, unsigned long vtkNotUsed(event),
      void *clientData, void *);
static void SatelliteStartRender(vtkObject *caller,
        unsigned long vtkNotUsed(event),
        void *clientData, void *);
static void SatelliteEndRender(vtkObject *caller,
            unsigned long vtkNotUsed(event),
            void *clientData, void *);
static void ResetCamera(vtkObject *caller,
            unsigned long vtkNotUsed(event),
            void *clientData, void *);
static void ResetCameraClippingRange(vtkObject *caller,
             unsigned long vtkNotUsed(event),
             void *clientData, void *);
static void RenderRMI(void *arg, void *, int, int);
static void ComputeVisiblePropBoundsRMI(void *arg, void *, int, int);

struct RenderWindowInfoInt {
  int FullSize[2];
  int ReducedSize[2];
  int NumberOfRenderers;
  int ImageReductionFactor;
  int UseCompositing;
};
struct RenderWindowInfoFloat {
  float DesiredUpdateRate;
};

struct RendererInfoInt {
  int NumberOfLights;
};
struct RendererInfoFloat {
  float Viewport[4];
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float Background[3];
};

struct LightInfoFloat {
  float Position[3];
  float FocalPoint[3];
};

const int WIN_INFO_INT_SIZE = sizeof(RenderWindowInfoInt)/sizeof(int);
const int WIN_INFO_FLOAT_SIZE = sizeof(RenderWindowInfoFloat)/sizeof(float);
const int REN_INFO_INT_SIZE = sizeof(RendererInfoInt)/sizeof(int);
const int REN_INFO_FLOAT_SIZE = sizeof(RendererInfoFloat)/sizeof(float);
const int LIGHT_INFO_FLOAT_SIZE = sizeof(LightInfoFloat)/sizeof(float);

vtkCxxRevisionMacro(vtkParallelRenderManager, "1.2.2.2");

vtkParallelRenderManager::vtkParallelRenderManager()
{
  this->RenderWindow = NULL;
  this->ObservingRenderWindow = false;
  this->ObservingRenderer = false;

  this->Controller = NULL;
  this->RootProcessId = 0;

  this->Lock = 0;

  this->ImageReductionFactor = 1;
  this->MaxImageReductionFactor = 6;
  this->AutoImageReductionFactor = 0;
  this->AverageTimePerPixel = 0.0;

  this->ParallelRendering = 1;
  this->WriteBackImages = 1;
  this->MagnifyImages = 1;
  this->RenderEventPropagation = 1;
  this->UseCompositing = 1;

  this->FullImage = vtkUnsignedCharArray::New();
  this->ReducedImage = vtkUnsignedCharArray::New();
  this->FullImageUpToDate = false;
  this->ReducedImageUpToDate = false;
  this->RenderWindowImageUpToDate = false;

  this->Viewports = vtkFloatArray::New();
  this->Viewports->SetNumberOfComponents(4);

  this->Timer = vtkTimerLog::New();
}

vtkParallelRenderManager::~vtkParallelRenderManager()
{
  this->SetRenderWindow(NULL);
  this->SetController(NULL);
  this->FullImage->Delete();
  this->ReducedImage->Delete();
  this->Viewports->Delete();
  this->Timer->Delete();
}

void vtkParallelRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParallelRendering: "
     << (this->ParallelRendering ? "on" : "off") << endl;
  os << indent << "RenderEventPropagation: "
     << (this->RenderEventPropagation ? "on" : "off") << endl;
  os << indent << "UseCompositing: "
     << (this->UseCompositing ? "on" : "off") << endl;

  os << indent << "ObservingRendererWindow: "
     << (this->ObservingRenderWindow ? "yes" : "no") << endl;
  os << indent << "ObservingRenderer: "
     << (this->ObservingRenderer ? "yes" : "no") << endl;
  os << indent << "Locked: " << (this->Lock ? "yes" : "no") << endl;

  os << indent << "ImageReductionFactor: "
     << this->ImageReductionFactor << endl;
  os << indent << "MaxImageReductionFactor: "
     << this->MaxImageReductionFactor << endl;
  os << indent << "AutoImageReductionFactor: "
     << (this->AutoImageReductionFactor ? "on" : "off") << endl;

  os << indent << "WriteBackImages: "
     << (this->WriteBackImages ? "on" : "off") << endl;
  os << indent << "MagnifyImages: "
     << (this->MagnifyImages ? "on" : "off") << endl;

  os << indent << "FullImageSize: ("
     << this->FullImageSize[0] << ", " << this->FullImageSize[1] << ")" << endl;
  os << indent << "ReducedImageSize: ("
     << this->ReducedImageSize[0] << ", "
     << this->ReducedImageSize[1] << ")" << endl;

  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "RootProcessId: " << this->RootProcessId << endl;

  //os << indent << "Last render time: " << this->GetRenderTime() << endl;
  //os << indent << "Last image processing time: "
  //   << this->GetImageProcessingTime() << endl;
}

vtkRenderWindow *vtkParallelRenderManager::MakeRenderWindow()
{
  vtkDebugMacro("MakeRenderWindow");

  return vtkRenderWindow::New();
}

vtkRenderer *vtkParallelRenderManager::MakeRenderer()
{
  vtkDebugMacro("MakeRenderer");

  return vtkRenderer::New();
}

void vtkParallelRenderManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  vtkDebugMacro("SetRenderWindow");

  vtkRendererCollection *rens;
  vtkRenderer *ren;

  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (this->ObservingRenderWindow)
      {
      this->RenderWindow->RemoveObserver(this->StartRenderTag);
      this->RenderWindow->RemoveObserver(this->EndRenderTag);
      
      // Will make do with first renderer. (Assumes renderer does not change.)
      if (this->ObservingRenderer)
  {
  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  ren = rens->GetNextItem();
  if (ren)
    {
    ren->RemoveObserver(this->ResetCameraTag);
    ren->RemoveObserver(this->ResetCameraClippingRangeTag);
    }
  this->ObservingRenderer = false;
  }

      this->ObservingRenderWindow = false;
      }
    this->RenderWindow->RemoveObserver(this->AbortRenderCheckTag);

    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow = NULL;
    }

  this->RenderWindow = renWin;
  if (this->RenderWindow)
    {
    vtkCallbackCommand *cbc;

    this->RenderWindow->Register(this);

    // In case a subclass wants to raise aborts.
    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::AbortRenderCheck);
    cbc->SetClientData((void*)this);
    // renWin will delete the cbc when the observer is removed.
    this->AbortRenderCheckTag = renWin->AddObserver(vtkCommand::AbortCheckEvent,
                cbc);
    cbc->Delete();

    if (this->Controller)
      {
      if (this->Controller->GetLocalProcessId() == this->RootProcessId)
        {
  this->ObservingRenderWindow = true;
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::StartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartRenderTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::EndRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->EndRenderTag = renWin->AddObserver(vtkCommand::EndEvent,cbc);
        cbc->Delete();
        
        // Will make do with first renderer. (Assumes renderer does
        // not change.)
        rens = this->RenderWindow->GetRenderers();
        rens->InitTraversal();
        ren = rens->GetNextItem();
        if (ren)
          {
    this->ObservingRenderer = true;

          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(::ResetCameraClippingRange);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->ResetCameraClippingRangeTag = 
      ren->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
          cbc->Delete();

          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(::ResetCamera);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->ResetCameraTag =
      ren->AddObserver(vtkCommand::ResetCameraEvent,cbc);
          cbc->Delete();
          }
        }
      else // LocalProcessId != RootProcessId
        {
        vtkCallbackCommand *cbc;
        
  this->ObservingRenderWindow = true;
        
        cbc= vtkCallbackCommand::New();
        cbc->SetCallback(::SatelliteStartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartRenderTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::SatelliteEndRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->EndRenderTag = renWin->AddObserver(vtkCommand::EndEvent,cbc);
        cbc->Delete();
        }
      }
    }
}

void vtkParallelRenderManager
    ::SetController(vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController");

  if (this->Controller == controller)
    {
    return;
    }
  this->Controller = controller;
  this->Modified();

  // We've changed the controller.  This may change how observers are attached
  // to the render window.
  if (this->RenderWindow)
    {
    vtkRenderWindow *saveRenWin = this->RenderWindow;
    saveRenWin->Register(this);
    this->SetRenderWindow(NULL);
    this->SetRenderWindow(saveRenWin);
    saveRenWin->UnRegister(this);
    }
}

void vtkParallelRenderManager::InitializePieces()
{
  vtkDebugMacro("InitializePieces");

  vtkRendererCollection *rens;
  vtkRenderer *ren;
  vtkActorCollection *actors;
  vtkActor *actor;
  vtkMapper *mapper;
  vtkPolyDataMapper *pdMapper;
  int piece, numPieces;

  if ((this->RenderWindow == NULL) || (this->Controller == NULL))
    {
    vtkWarningMacro("Called InitializePieces before setting RenderWindow or Controller");
    return;
    }
  piece = this->Controller->GetLocalProcessId();
  numPieces = this->Controller->GetNumberOfProcesses();

  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  while ( (ren = rens->GetNextItem()) )
    {
    actors = ren->GetActors();
    actors->InitTraversal();
    while ( (actor = actors->GetNextItem()) )
      {
      mapper = actor->GetMapper();
      pdMapper = vtkPolyDataMapper::SafeDownCast(mapper);
      if (pdMapper)
        {
        pdMapper->SetPiece(piece);
        pdMapper->SetNumberOfPieces(numPieces);
        }
      }
    }
}

void vtkParallelRenderManager::InitializeOffScreen()
{
  vtkDebugMacro("InitializeOffScreen");

  if ((this->RenderWindow == NULL) || (this->Controller == NULL))
    {
    vtkWarningMacro("Called InitializeOffScreen before setting RenderWindow or Controller");
    return;
    }

  if (   (this->Controller->GetLocalProcessId() != this->RootProcessId)
      || !this->WriteBackImages)
    {
    this->RenderWindow->OffScreenRenderingOn();
    }
  else
    {
    this->RenderWindow->OffScreenRenderingOff();
    }
}

void vtkParallelRenderManager::StartInteractor()
{
  vtkDebugMacro("StartInteractor");

  if ((this->Controller == NULL) || (this->RenderWindow == NULL))
    {
    vtkErrorMacro("Must set Controller and RenderWindow before starting interactor.");
    return;
    }

  if (this->Controller->GetLocalProcessId() == this->RootProcessId)
    {
    vtkRenderWindowInteractor *inter = this->RenderWindow->GetInteractor();
    if (!inter)
      {
      vtkErrorMacro("Render window does not have an interactor.");
      }
    else
      {
      inter->Initialize();
      inter->Start();
      }
    //By the time we reach here, the interaction is finished.
    this->StopService();
    }
  else
    {
    this->StartService();
    }
}

void vtkParallelRenderManager::StartService()
{
    vtkDebugMacro("StartService");
  
  if (!this->Controller)
    {
    vtkErrorMacro("Must set Controller before starting service");
    return;
    }
  if (this->Controller->GetLocalProcessId() == this->RootProcessId)
    {
    vtkWarningMacro("Starting service on root process (probably not what you wanted to do)");
    }

  this->InitializeRMIs();
  this->Controller->ProcessRMIs();
}

void vtkParallelRenderManager::StopService()
{
  vtkDebugMacro("StopService");

  if (!this->Controller)
    {
    vtkErrorMacro("Must set Controller before stopping service");
    return;
    }
  if (this->Controller->GetLocalProcessId() != this->RootProcessId)
    {
    vtkErrorMacro("Can only stop services on root node");
    return;
    }

  int numProcs = this->Controller->GetNumberOfProcesses();
  for (int id = 0; id < numProcs; id++)
    {
    if (id == this->RootProcessId) continue;
    this->Controller->TriggerRMI(id,vtkMultiProcessController::BREAK_RMI_TAG);
    }
}

void vtkParallelRenderManager::StartRender()
{
  int i;
  struct RenderWindowInfoInt winInfoInt;
  struct RenderWindowInfoFloat winInfoFloat;
  struct RendererInfoInt renInfoInt;
  struct RendererInfoFloat renInfoFloat;
  struct LightInfoFloat lightInfoFloat;

  vtkDebugMacro("StartRender");

  if ((this->Controller == NULL) || (this->Lock))
    {
    return;
    }
  this->Lock = true;

  this->FullImageUpToDate = false;
  this->ReducedImageUpToDate = false;
  this->RenderWindowImageUpToDate = false;

  if (this->FullImage->GetPointer(0) == this->ReducedImage->GetPointer(0))
    {
    // "Un-share" pointer for full/reduced images in case we need separate
    // arrays this run.
    this->ReducedImage->Initialize();
    }

  if (!this->ParallelRendering)
    {
    this->Lock = false;
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent, NULL);

  // Used to time the total render (without compositing).
  this->Timer->StartTimer();

  if (this->AutoImageReductionFactor)
    {
    this->SetImageReductionFactorForUpdateRate
      (this->RenderWindow->GetDesiredUpdateRate());
    }

  int id;
  int numProcs = this->Controller->GetNumberOfProcesses();

  // Make adjustments for window size.
  int *size = this->RenderWindow->GetSize();
  if ((size[0] == 0) || (size[1] == 0))
    {
    // It helps to have a real window size.
    vtkDebugMacro("Resetting window size to 300x300");
    size[0] = size[1] = 300;
    this->RenderWindow->SetSize(size[0], size[1]);
    }
  this->FullImageSize[0] = size[0];
  this->FullImageSize[1] = size[1];
  this->ReducedImageSize[0] = size[0]/this->ImageReductionFactor;
  this->ReducedImageSize[1] = size[1]/this->ImageReductionFactor;

  // Collect and distribute information about current state of RenderWindow
  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
  winInfoInt.FullSize[0] = this->FullImageSize[0];
  winInfoInt.FullSize[1] = this->FullImageSize[1];
  winInfoInt.ReducedSize[0] = this->ReducedImageSize[0];
  winInfoInt.ReducedSize[1] = this->ReducedImageSize[1];
  winInfoInt.NumberOfRenderers = rens->GetNumberOfItems();
  winInfoInt.ImageReductionFactor = this->ImageReductionFactor;
  winInfoInt.UseCompositing = this->UseCompositing;
  winInfoFloat.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();

  for (id = 0; id < numProcs; id++)
    {
    if (id == this->RootProcessId) continue;
    if (this->RenderEventPropagation)
      {
      this->Controller->TriggerRMI(id, NULL, 0,
           vtkParallelRenderManager::RENDER_RMI_TAG);
      }
    this->Controller->Send((int *)(&winInfoInt), WIN_INFO_INT_SIZE, id,
         vtkParallelRenderManager::WIN_INFO_INT_TAG);
    this->Controller->Send((float *)(&winInfoFloat), WIN_INFO_FLOAT_SIZE, id,
         vtkParallelRenderManager::WIN_INFO_FLOAT_TAG);
    this->SendWindowInformation();
    }

  if (this->ImageReductionFactor > 1)
    {
    this->Viewports->SetNumberOfTuples(rens->GetNumberOfItems());
    }
  vtkRenderer *ren;
  for (rens->InitTraversal(), i = 0; (ren = rens->GetNextItem()); i++)
    {
    ren->GetViewport(renInfoFloat.Viewport);

    // Adjust Renderer viewports to get reduced size image.
    if (this->ImageReductionFactor > 1)
      {
      this->Viewports->SetTuple(i, renInfoFloat.Viewport);
      renInfoFloat.Viewport[0] /= this->ImageReductionFactor;
      renInfoFloat.Viewport[1] /= this->ImageReductionFactor;
      renInfoFloat.Viewport[2] /= this->ImageReductionFactor;
      renInfoFloat.Viewport[3] /= this->ImageReductionFactor;
      ren->SetViewport(renInfoFloat.Viewport);
      }

    vtkCamera *cam = ren->GetActiveCamera();
    cam->GetPosition(renInfoFloat.CameraPosition);
    cam->GetFocalPoint(renInfoFloat.CameraFocalPoint);
    cam->GetViewUp(renInfoFloat.CameraViewUp);
    cam->GetClippingRange(renInfoFloat.CameraClippingRange);
    ren->GetBackground(renInfoFloat.Background);

    cam->SetWindowCenter(0.0, 0.0);


    vtkLightCollection *lc = ren->GetLights();
    renInfoInt.NumberOfLights = lc->GetNumberOfItems();

    for (id = 0; id < numProcs; id++)
      {
      if (id == this->RootProcessId) continue;
      this->Controller->Send((int *)(&renInfoInt), REN_INFO_INT_SIZE, id,
           vtkParallelRenderManager::REN_INFO_INT_TAG);
      this->Controller->Send((float *)(&renInfoFloat), REN_INFO_FLOAT_SIZE, id,
           vtkParallelRenderManager::REN_INFO_FLOAT_TAG);
      }

    vtkLight *light;
    for (lc->InitTraversal(); (light = lc->GetNextItem()); )
      {
      light->GetPosition(lightInfoFloat.Position);
      light->GetFocalPoint(lightInfoFloat.FocalPoint);

      for (id = 0; id < numProcs; id++)
        {
        if (id == this->RootProcessId) continue;
        this->Controller->Send((float *)(&lightInfoFloat),
             LIGHT_INFO_FLOAT_SIZE, id,
             vtkParallelRenderManager::LIGHT_INFO_FLOAT_TAG);
        }
      }

    this->SendRendererInformation(ren);
    }

  this->PreRenderProcessing();
}

void vtkParallelRenderManager::EndRender()
{
  if (!this->ParallelRendering)
    {
    return;
    }

  this->Timer->StopTimer();
  this->RenderTime = this->Timer->GetElapsedTime();
  this->ImageProcessingTime = 0;

  if (!this->UseCompositing)
    {
    this->Lock = 0;
    return;
    }

  this->PostRenderProcessing();

  // Restore renderer viewports, if necessary.
  if (this->ImageReductionFactor > 1)
    {
    vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
    vtkRenderer *ren;
    int i;
    for (rens->InitTraversal(), i = 0; (ren = rens->GetNextItem()); i++)
      {
      ren->SetViewport(this->Viewports->GetPointer(i*4));
      }
    }

  WriteFullImage();

  this->InvokeEvent(vtkCommand::EndEvent, NULL);

  this->Lock = 0;
}

void vtkParallelRenderManager::SatelliteStartRender()
{
  struct RenderWindowInfoInt winInfoInt;
  struct RenderWindowInfoFloat winInfoFloat;
  struct RendererInfoInt renInfoInt;
  struct RendererInfoFloat renInfoFloat;
  struct LightInfoFloat lightInfoFloat;
  int i, j;

  vtkDebugMacro("SatelliteStartRender");

  this->FullImageUpToDate = false;
  this->ReducedImageUpToDate = false;
  this->RenderWindowImageUpToDate = false;

  if (this->FullImage->GetPointer(0) == this->ReducedImage->GetPointer(0))
    {
    // "Un-share" pointer for full/reduced images in case we need separate
    // arrays this run.
    this->ReducedImage->Initialize();
    }

  if (!this->ParallelRendering)
    {
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent, NULL);

  this->Controller->Receive((int *)(&winInfoInt), WIN_INFO_INT_SIZE,
          this->RootProcessId,
          vtkParallelRenderManager::WIN_INFO_INT_TAG);
  this->Controller->Receive((float *)(&winInfoFloat), WIN_INFO_FLOAT_SIZE,
          this->RootProcessId,
          vtkParallelRenderManager::WIN_INFO_FLOAT_TAG);
  this->ReceiveWindowInformation();

  this->RenderWindow->SetDesiredUpdateRate(winInfoFloat.DesiredUpdateRate);
  this->UseCompositing = winInfoInt.UseCompositing;
  this->ImageReductionFactor = winInfoInt.ImageReductionFactor;
  this->FullImageSize[0] = winInfoInt.FullSize[0];
  this->FullImageSize[1] = winInfoInt.FullSize[1];
  this->ReducedImageSize[0] = winInfoInt.ReducedSize[0];
  this->ReducedImageSize[1] = winInfoInt.ReducedSize[1];
  this->SetRenderWindowSize();

  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  for (i = 0; i < winInfoInt.NumberOfRenderers; i++)
    {
    this->Controller->Receive((int *)(&renInfoInt), REN_INFO_INT_SIZE,
            this->RootProcessId,
            vtkParallelRenderManager::REN_INFO_INT_TAG);
    this->Controller->Receive((float *)(&renInfoFloat), REN_INFO_FLOAT_SIZE,
            this->RootProcessId,
            vtkParallelRenderManager::REN_INFO_FLOAT_TAG);

    vtkLightCollection *lc;
    vtkRenderer *ren = rens->GetNextItem();
    if (ren == NULL)
      {
      vtkErrorMacro("Not enough renderers");
      }
    else
      {
      ren->SetViewport(renInfoFloat.Viewport);
      ren->SetBackground(renInfoFloat.Background);
      vtkCamera *cam = ren->GetActiveCamera();
      cam->SetPosition(renInfoFloat.CameraPosition);
      cam->SetFocalPoint(renInfoFloat.CameraFocalPoint);
      cam->SetViewUp(renInfoFloat.CameraViewUp);
      cam->SetClippingRange(renInfoFloat.CameraClippingRange);
      cam->SetWindowCenter(0.0, 0.0);
      lc = ren->GetLights();
      lc->InitTraversal();
      }

    for (j = 0; j < renInfoInt.NumberOfLights; j++)
      {
      if (ren != NULL)
  {
  vtkLight *light = lc->GetNextItem();
  if (light == NULL)
    {
    // Not enough lights?  Just create them.
    vtkDebugMacro("Adding light");
    light = vtkLight::New();
    ren->AddLight(light);
    light->Delete();
    }

  this->Controller->Receive((float *)(&lightInfoFloat),
          LIGHT_INFO_FLOAT_SIZE, this->RootProcessId,
          vtkParallelRenderManager
          ::LIGHT_INFO_FLOAT_TAG);
  light->SetPosition(lightInfoFloat.Position);
  light->SetFocalPoint(lightInfoFloat.FocalPoint);
  }
      }

    if (ren != NULL)
      {
      vtkLight *light;
      while ((light = lc->GetNextItem()))
        {
        // To many lights?  Just remove the extras.
        ren->RemoveLight(light);
        }
      }

    this->ReceiveRendererInformation(ren);
    }

  this->PreRenderProcessing();
}

void vtkParallelRenderManager::SatelliteEndRender()
{
  if (!this->ParallelRendering)
    {
    return;
    }
  if (!this->UseCompositing)
    {
    return;
    }

  this->PostRenderProcessing();

  WriteFullImage();

  this->InvokeEvent(vtkCommand::EndEvent, NULL);
}

void vtkParallelRenderManager::RenderRMI()
{
  this->RenderWindow->Render();
}

void vtkParallelRenderManager::ResetCamera(vtkRenderer *ren)
{
  return;
  vtkDebugMacro("ResetCamera");

  float bounds[6];

  if (this->Lock)
    {
    // Can't query other processes in the middle of a render.
    // Just grab local value instead.
    this->LocalComputeVisiblePropBounds(ren, bounds);
    ren->ResetCamera(bounds);
    return;
    }

  this->Lock = true;

  this->ComputeVisiblePropBounds(ren, bounds);
  // Keep from setting camera from some outrageous value.
  if (bounds[0]>bounds[1] || bounds[2]>bounds[3] || bounds[4]>bounds[5])
    {
    // See if the not pickable values are better.
    ren->ComputeVisiblePropBounds(bounds);
    if (bounds[0]>bounds[1] || bounds[2]>bounds[3] || bounds[4]>bounds[5])
      {
      this->Lock = 0;
      return;
      }
    }
  ren->ResetCamera(bounds);
  
  this->Lock = 0;
}

void vtkParallelRenderManager::ResetCameraClippingRange(vtkRenderer *ren)
{
  return;
  vtkDebugMacro("ResetCameraClippingRange");

  float bounds[6];

  if (this->Lock)
    {
    // Can't query other processes in the middle of a render.
    // Just grab local value instead.
    this->LocalComputeVisiblePropBounds(ren, bounds);
    ren->ResetCameraClippingRange(bounds);
    return;
    }

  this->Lock = 1;
  
  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCameraClippingRange(bounds);

  this->Lock = 0;
}

void vtkParallelRenderManager::ComputeVisiblePropBoundsRMI()
{
  vtkDebugMacro("ComputeVisiblePropBoundsRMI");
  int i;

  // Get proper renderer.
  int renderId;
  this->Controller->Receive(&renderId, 1, this->RootProcessId,
          vtkParallelRenderManager::REN_ID_TAG);
  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
  vtkRenderer *ren;
  rens->InitTraversal();
  for (i = 0; i <= renderId; i++)
    {
    ren = rens->GetNextItem();
    }

  if (ren == NULL)
    {
    vtkWarningMacro("Client requested invalid renderer in "
        "ComputeVisiblePropBoundsRMI\n"
        "Defaulting to first renderer");
    rens->InitTraversal();
    ren = rens->GetNextItem();
    }

  float bounds[6];
  this->LocalComputeVisiblePropBounds(ren, bounds);

  this->Controller->Send(bounds, 6, this->RootProcessId,
       vtkParallelRenderManager::BOUNDS_TAG);
}

void vtkParallelRenderManager::LocalComputeVisiblePropBounds(vtkRenderer *ren,
                   float bounds[6])
{
  ren->ComputeVisiblePropBounds(bounds);
}


void vtkParallelRenderManager::ComputeVisiblePropBounds(vtkRenderer *ren,
              float bounds[6])
{
  vtkDebugMacro("ComputeVisiblePropBounds");

  ren->ComputeVisiblePropBounds(bounds);

  if (!this->ParallelRendering)
    {
    return;
    }
  if (!this->UseCompositing)
    {
    return;
    }

  if (this->Controller)
    {
    if (this->Controller->GetLocalProcessId() != this->RootProcessId)
      {
      vtkErrorMacro("ComputeVisiblePropBounds/ResetCamera can only be called on root process");
      return;
      }

    vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
    rens->InitTraversal();
    int renderId = 0;
    while (true)
      {
      vtkRenderer *myren = rens->GetNextItem();
      if (myren == NULL)
  {
  vtkWarningMacro("ComputeVisiblePropBounds called with unregistered renderer " << ren << "\nDefaulting to first renderer.");
  renderId = 0;
  break;
  }
      if (myren == ren)
  {
  //Found right renderer.
  break;
  }
      renderId++;
      }

    int numProcs = this->Controller->GetNumberOfProcesses();
    for (int id = 0; id < numProcs; id++)
      {
      float tmp[6];
      if (id == this->RootProcessId) continue;
      this->Controller->TriggerRMI
  (id, vtkParallelRenderManager::COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
      this->Controller->Send(&renderId, 1, id,
           vtkParallelRenderManager::REN_ID_TAG);
      this->Controller->Receive(tmp, 6, id, vtkParallelRenderManager::BOUNDS_TAG);
      
      if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
      if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
      if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
      if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
      if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
      if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
      }
    }
  else
    {
    vtkWarningMacro("ComputeVisiblePropBounds/ResetCamera called before Controller set");
    }
}

void vtkParallelRenderManager::InitializeRMIs()
{
  vtkDebugMacro("InitializeRMIs");


  if (this->Controller == NULL)
    {
    vtkErrorMacro("InitializeRMIs requires a controller.");
    return;
    }

  this->Controller->AddRMI(::RenderRMI, this,
         vtkParallelRenderManager::RENDER_RMI_TAG);
  this->Controller->AddRMI(::ComputeVisiblePropBoundsRMI, this,
         vtkParallelRenderManager::
         COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
}

void vtkParallelRenderManager::ResetAllCameras()
{
  vtkDebugMacro("ResetAllCameras");

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Called ResetAllCameras before RenderWindow set");
    return;
    }

  vtkRendererCollection *rens;
  vtkRenderer *ren;

  rens = this->RenderWindow->GetRenderers();
  for (rens->InitTraversal(); (ren = rens->GetNextItem()); )
    {
    this->ResetCamera(ren);
    }
}

void vtkParallelRenderManager
    ::SetImageReductionFactorForUpdateRate(float DesiredUpdateRate)
{
  vtkDebugMacro("Setting reduction factor for update rate of "
    << DesiredUpdateRate);

  if (DesiredUpdateRate == 0.0)
    {
    this->SetImageReductionFactor(1);
    return;
    }

  int *size = this->RenderWindow->GetSize();
  int NumPixels = size[0]*size[1];
  int NumReducedPixels
    = NumPixels/(this->ImageReductionFactor*this->ImageReductionFactor);

  double RenderTime = this->GetRenderTime();
  double PixelTime = this->GetImageProcessingTime();

  double TimePerPixel;
  if (NumReducedPixels > 0)
    {
    TimePerPixel = PixelTime/NumReducedPixels;
    }
  else
    {
    // Must be before first render.
    this->SetImageReductionFactor(1);
    return;
    }

  this->AverageTimePerPixel = (3*this->AverageTimePerPixel + TimePerPixel)/4;

  double AllottedPixelTime = 1.0/DesiredUpdateRate - RenderTime;
  // Give ourselves at least 15% of render time.
  if (AllottedPixelTime < 0.15*RenderTime)
    {
    AllottedPixelTime = 0.15*RenderTime;
    }

  vtkDebugMacro("TimePerPixel: " << TimePerPixel
    << ", AverageTimePerPixel: " << this->AverageTimePerPixel
    << ", AllottedPixelTime: " << AllottedPixelTime);

  double PixelsToUse = AllottedPixelTime/this->AverageTimePerPixel;

  if (   (PixelsToUse < 1)
      || (NumPixels/PixelsToUse > this->MaxImageReductionFactor) )
    {
    this->SetImageReductionFactor(this->MaxImageReductionFactor);
    }
  else if (PixelsToUse >= NumPixels)
    {
    this->SetImageReductionFactor(1);
    }
  else
    {
    this->SetImageReductionFactor((int)(NumPixels/PixelsToUse));
    }
}

void vtkParallelRenderManager::SetRenderWindowSize()
{
  this->RenderWindow->SetSize(this->FullImageSize[0], this->FullImageSize[1]);
}

int vtkParallelRenderManager::LastRenderInFrontBuffer()
{
  return this->RenderWindow->GetSwapBuffers();
}

void vtkParallelRenderManager::MagnifyReducedImage()
{
  if ((this->FullImageUpToDate))
    {
    return;
    }

  this->ReadReducedImage();

  if (this->FullImage->GetPointer(0) != this->ReducedImage->GetPointer(0))
    {
    this->FullImage->SetNumberOfComponents(3);
    this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
               * this->FullImageSize[1]);

    this->Timer->StartTimer();

    // Inflate image.
    float xstep = (float)this->ReducedImageSize[0]/this->FullImageSize[0];
    float ystep = (float)this->ReducedImageSize[1]/this->FullImageSize[1];
    unsigned char *lastsrcline = NULL;
    for (int y = 0; y < this->FullImageSize[1]; y++)
      {
      unsigned char *destline
  = this->FullImage->GetPointer(3*this->FullImageSize[0]*y);
      unsigned char *srcline
  = this->ReducedImage->GetPointer(  3*this->ReducedImageSize[0]
           * (int)(ystep*y));
      if (srcline == lastsrcline)
  {
  // This line same as last one.
  memcpy(destline,
         (const unsigned char *)(destline - 3*this->FullImageSize[0]),
         3*this->FullImageSize[0]);
  }
      else
  {
  for (int x = 0; x < this->FullImageSize[0]; x++)
    {
    int srcloc = 3*(int)(x*xstep);
    int destloc = 3*x;
    destline[destloc + 0] = srcline[srcloc + 0];
    destline[destloc + 1] = srcline[srcloc + 1];
    destline[destloc + 2] = srcline[srcloc + 2];
    }
  lastsrcline = srcline;
  }
      }

    this->Timer->StopTimer();
    // We log the image inflation under render time because it is inversely
    // proportional to the image size.  This makes the auto image reduction
    // calculation work better.
    this->RenderTime += this->Timer->GetElapsedTime();
    }

  this->FullImageUpToDate = true;
}

void vtkParallelRenderManager::WriteFullImage()
{
  if (this->RenderWindowImageUpToDate || !this->WriteBackImages)
    {
    return;
    }

  if (this->MagnifyImages && (this->ImageReductionFactor > 1))
    {
    this->MagnifyReducedImage();
    this->RenderWindow->SetPixelData(0, 0,
             this->FullImageSize[0]-1,
             this->FullImageSize[1]-1,
             this->FullImage,
             this->LastRenderInFrontBuffer());
    }
  else
    {
    // Only write back image if it has already been read and potentially
    // changed.
    if (this->ReducedImageUpToDate)
      {
      this->RenderWindow->SetPixelData(0, 0,
               this->ReducedImageSize[0]-1,
               this->ReducedImageSize[1]-1,
               this->ReducedImage,
               this->LastRenderInFrontBuffer());
      }
    }

  this->RenderWindowImageUpToDate = true;
}

void vtkParallelRenderManager::ReadReducedImage()
{
  if (this->ReducedImageUpToDate)
    {
    return;
    }

  this->Timer->StartTimer();

  if (this->ImageReductionFactor > 1)
    {
    this->RenderWindow->GetPixelData(0, 0, this->ReducedImageSize[0]-1,
             this->ReducedImageSize[1]-1,
             this->LastRenderInFrontBuffer(),
             this->ReducedImage);
    }
  else
    {
    this->RenderWindow->GetPixelData(0, 0, this->FullImageSize[0]-1,
             this->FullImageSize[1]-1,
             this->LastRenderInFrontBuffer(),
             this->FullImage);
    this->FullImageUpToDate = true;
    this->ReducedImage
      ->SetNumberOfComponents(this->FullImage->GetNumberOfComponents());
    this->ReducedImage->SetArray(this->FullImage->GetPointer(0),
         this->FullImage->GetSize(), 1);
    this->ReducedImage->SetNumberOfTuples(this->FullImage->GetNumberOfTuples());
    }

  this->Timer->StopTimer();
  this->ImageProcessingTime += this->Timer->GetElapsedTime();

  this->ReducedImageUpToDate = true;
}

void vtkParallelRenderManager::GetPixelData(vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  // Read image from RenderWindow and magnify if necessary.
  MagnifyReducedImage();

  data->SetNumberOfComponents(this->FullImage->GetNumberOfComponents());
  data->SetArray(this->FullImage->GetPointer(0),
     this->FullImage->GetSize(), 1);
  data->SetNumberOfTuples(this->FullImage->GetNumberOfTuples());
}

void vtkParallelRenderManager::GetPixelData(int x1, int y1, int x2, int y2,
              vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  MagnifyReducedImage();

  if (x1 > x2)
    {
    int tmp = x1;
    x1 = x2;
    x2 = tmp;
    }
  if (y1 > y2)
    {
    int tmp = y1;
    y1 = y2;
    y2 = tmp;
    }

  if (   (x1 < 0) || (x2 >= this->FullImageSize[0])
      || (y1 < 0) || (y2 >= this->FullImageSize[1]))
    {
    vtkErrorMacro("Requested pixel data out of RenderWindow bounds");
    return;
    }

  vtkIdType width = x2 - x1 + 1;
  vtkIdType height = y2 - y1 + 1;

  data->SetNumberOfComponents(3);
  data->SetNumberOfTuples(width*height);

  const unsigned char *src = this->FullImage->GetPointer(0);
  unsigned char *dest = data->WritePointer(0, width*height*3);

  for (int row = 0; row < height; row++)
    {
    memcpy(dest + row*width*3,
     src + (row+y1)*this->FullImageSize[0]*3 + x1*3, width*3);
    }
}

void vtkParallelRenderManager::GetReducedPixelData(vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  // Read image from RenderWindow and magnify if necessary.
  ReadReducedImage();

  data->SetNumberOfComponents(this->ReducedImage->GetNumberOfComponents());
  data->SetArray(this->ReducedImage->GetPointer(0),
     this->ReducedImage->GetSize(), 1);
  data->SetNumberOfTuples(this->ReducedImage->GetNumberOfTuples());
}

void vtkParallelRenderManager::GetReducedPixelData(int x1, int y1,
               int x2, int y2,
               vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  ReadReducedImage();

  if (x1 > x2)
    {
    int tmp = x1;
    x1 = x2;
    x2 = tmp;
    }
  if (y1 > y2)
    {
    int tmp = y1;
    y1 = y2;
    y2 = tmp;
    }

  if (   (x1 < 0) || (x2 >= this->ReducedImageSize[0])
      || (y1 < 0) || (y2 >= this->ReducedImageSize[1]))
    {
    vtkErrorMacro("Requested pixel data out of RenderWindow bounds");
    return;
    }

  vtkIdType width = x2 - x1 + 1;
  vtkIdType height = y2 - y1 + 1;

  data->SetNumberOfComponents(3);
  data->SetNumberOfTuples(width*height);

  const unsigned char *src = this->ReducedImage->GetPointer(0);
  unsigned char *dest = data->WritePointer(0, width*height*3);

  for (int row = 0; row < height; row++)
    {
    memcpy(dest + row*width*3,
     src + (row+y1)*this->ReducedImageSize[0]*3 + x1*3, width*3);
    }
}

// Static function prototypes --------------------------------------------

static void AbortRenderCheck(vtkObject *caller, unsigned long vtkNotUsed(event),
           void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkParallelRenderManager caller mismatch");
    return;
    }
  self->CheckForAbortRender();
}

static void StartRender(vtkObject *caller, unsigned long vtkNotUsed(event),
      void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkParallelRenderManager caller mismatch");
    return;
    }
  self->StartRender();
}

static void EndRender(vtkObject *caller, unsigned long vtkNotUsed(event),
      void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkParallelRenderManager caller mismatch");
    return;
    }
  self->EndRender();
}

static void SatelliteStartRender(vtkObject *caller,
        unsigned long vtkNotUsed(event),
        void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkParallelRenderManager caller mismatch");
    return;
    }
  self->SatelliteStartRender();
}

static void SatelliteEndRender(vtkObject *caller,
            unsigned long vtkNotUsed(event),
            void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkParallelRenderManager caller mismatch");
    return;
    }
  self->SatelliteEndRender();
}

static void ResetCamera(vtkObject *caller,
      unsigned long vtkNotUsed(event),
      void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  vtkRenderer *ren = (vtkRenderer *)caller;
  self->ResetCamera(ren);
}

static void ResetCameraClippingRange(vtkObject *caller,
             unsigned long vtkNotUsed(event),
             void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  vtkRenderer *ren = (vtkRenderer *)caller;
  self->ResetCameraClippingRange(ren);
}

static void RenderRMI(void *arg, void *, int, int)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)arg;
  self->RenderRMI();
}

static void ComputeVisiblePropBoundsRMI(void *arg, void *, int, int)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)arg;
  self->ComputeVisiblePropBoundsRMI();
}
