/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTreeComposite.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTreeComposite.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCompositer.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkImageActor.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#else
#include "vtkMultiProcessController.h"
#endif

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVTreeComposite);
vtkCxxRevisionMacro(vtkPVTreeComposite, "1.59");


//=========================================================================
// Stuff to avoid compositing if there is no data on satellite processes.


//----------------------------------------------------------------------------
void vtkPVTreeCompositeCheckForDataRMI(void *arg, void *, int, int)
{
  vtkPVTreeComposite* self = (vtkPVTreeComposite*) arg;
  
  self->CheckForDataRMI();
}

void vtkPVTreeCompositeExitInteractor(vtkObject *vtkNotUsed(o),
                                      unsigned long vtkNotUsed(event), 
                                      void *clientData, void *)
{
  vtkPVTreeComposite *self = (vtkPVTreeComposite *)clientData;

  self->ExitInteractor();
}

//-------------------------------------------------------------------------
void vtkPVTreeCompositeAbortRenderCheck(vtkObject*, unsigned long, void* arg,
                                        void*)
{
  vtkPVTreeComposite *self = (vtkPVTreeComposite*)arg;
  
  self->CheckForAbortRender();
}

//-------------------------------------------------------------------------
void vtkPVTreeCompositeStartRender(vtkObject *caller,
                                   unsigned long vtkNotUsed(event), 
                                   void *clientData, void *)
{
  vtkPVTreeComposite *self = (vtkPVTreeComposite *)clientData;
  
  if (caller != self->GetRenderWindow()->GetRenderers()->GetItemAsObject(0))
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  static int inRender = 0;
  if (inRender)
    {
    return;
    }
  inRender = 1;
  self->StartRender();
  inRender = 0;
}

//-------------------------------------------------------------------------
void vtkPVTreeCompositeEndRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkPVTreeComposite *self = (vtkPVTreeComposite *)clientData;
  
  if (caller != self->GetRenderWindow()->GetRenderers()->GetItemAsObject(0))
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  static int inRender = 0;
  if (inRender)
    {
    return;
    }
  inRender = 1;
  self->EndRender();
  inRender = 0;
}

//-------------------------------------------------------------------------
void vtkPVTreeCompositeResetCameraClippingRange(
  vtkObject *caller, unsigned long vtkNotUsed(event),void *clientData, void *)
{
  vtkPVTreeComposite *self = (vtkPVTreeComposite *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCameraClippingRange(ren);
}

//-------------------------------------------------------------------------
void vtkPVTreeCompositeResetCamera(vtkObject *caller,
                                   unsigned long vtkNotUsed(event), 
                                   void *clientData, void *)
{
  vtkPVTreeComposite *self = (vtkPVTreeComposite *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCamera(ren);
}

//----------------------------------------------------------------------------
void vtkPVTreeCompositeSatelliteStartRender(vtkObject* vtkNotUsed(caller),
                                            unsigned long vtkNotUsed(event), 
                                            void *clientData, void *)
{
  vtkPVTreeComposite* self = (vtkPVTreeComposite*) clientData;
  
  self->SatelliteStartRender();
}

//----------------------------------------------------------------------------
void vtkPVTreeCompositeSatelliteEndRender(vtkObject* vtkNotUsed(caller),
                                          unsigned long vtkNotUsed(event), 
                                          void *clientData, void *)
{
  vtkPVTreeComposite* self = (vtkPVTreeComposite*) clientData;
  
  self->SatelliteEndRender();
}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkPVTreeComposite::InitializeRMIs()
{
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  this->vtkCompositeRenderManager::InitializeRMIs();

  this->Controller->AddRMI(vtkPVTreeCompositeCheckForDataRMI, (void*)this, 
                           vtkPVTreeComposite::CHECK_FOR_DATA_TAG);
} 


//-------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForDataRMI()
{
  int dataFlag = this->CheckForData();
  this->Controller->Send(&dataFlag, 1, 0, 877630);
}



//-------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForData()
{
  int dataFlag = 0;
  vtkRendererCollection *rens;
  vtkRenderer *ren;
  vtkActorCollection *actors;
  vtkActor *actor;
  vtkMapper *mapper;

  if (this->RenderWindow == NULL || this->Controller == NULL)
    {
    vtkErrorMacro("Missing RenderWindow or Controller.");
    return 0;
    }

  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  while ( (ren = rens->GetNextItem()) )
    {
    actors = ren->GetActors();
    actors->InitTraversal();
    while ( (actor = actors->GetNextItem()) )
      {
      mapper = actor->GetMapper();
      if (actor->GetVisibility() && mapper)
        {
        mapper->Update();
        if (mapper->GetInput()->GetNumberOfCells() > 0)
          {
          dataFlag = 1;
          }
        }
      }
    }

  return dataFlag;
}


//-------------------------------------------------------------------------
int vtkPVTreeComposite::ShouldIComposite()
{
  int idx;
  int numProcs;
  int myId;
  int dataFlag = 0;
  int tmp = 0;

  if (this->Controller == NULL)
    {
    return 0;
    }
  numProcs = this->Controller->GetNumberOfProcesses();
  myId = this->Controller->GetLocalProcessId();
  if (myId != 0)
    {
    vtkErrorMacro("This method should only be called from process 0.");
    }

  for (idx = 1; idx < numProcs; ++idx)
    {
    this->Controller->TriggerRMI(idx, NULL, 0, 
                                 vtkPVTreeComposite::CHECK_FOR_DATA_TAG);
    }
  // To keep the updates balanced.
  this->CheckForData();
  for (idx = 1; idx < numProcs; ++idx)
    {
    this->Controller->Receive(&tmp, 1, idx, 877630);
    if (tmp)
      {
      dataFlag = 1;
      }  
    }
  
  return dataFlag;
}



//-------------------------------------------------------------------------
// Only called in process 0.
void vtkPVTreeComposite::StartRender()
{
  // I do not know if the composite manager check this flag.
  // I believe it does, but just in case ...
  if (this->UseCompositing)
    {
    this->InternalStartRender();
    }
}



//-------------------------------------------------------------------------
// No need to turn swap buffers off.
void vtkPVTreeComposite::PreRenderProcessing()
{
}



//-------------------------------------------------------------------------
void vtkPVTreeComposite::InternalStartRender()
{
  vtkParallelRenderManager::RenderWindowInfoInt winInfoInt;
  vtkParallelRenderManager::RenderWindowInfoDouble winInfoDouble;
  vtkParallelRenderManager::RendererInfoInt renInfoInt;
  vtkParallelRenderManager::RendererInfoDouble renInfoDouble;
  vtkParallelRenderManager::LightInfoDouble lightInfoDouble;
  int id, numProcs;
  int *size;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  
  vtkDebugMacro("StartRender");
  
  // Used to time the total render (without compositing.)
  this->Timer->StartTimer();

  if (!this->UseCompositing)
    {
    return;
    }  

  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;

  if (controller == NULL || this->Lock)
    {
    return;
    }
  
  // Lock here, unlock at end render.
  this->Lock = 1;

  this->FullImageUpToDate = 0;
  
  // Trigger the satellite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  numProcs = this->Controller->GetNumberOfProcesses();
  size = this->RenderWindow->GetSize();
  if (this->ImageReductionFactor > 0)
    {
    winInfoInt.FullSize[0] = size[0];
    winInfoInt.FullSize[1] = size[1];
    winInfoDouble.ImageReductionFactor = this->ImageReductionFactor;
    winInfoInt.ReducedSize[0] = (int)(size[0] / this->ImageReductionFactor);
    winInfoInt.ReducedSize[1] = (int)(size[1] / this->ImageReductionFactor);
    vtkRenderer* renderer =
      ((vtkRenderer*)this->RenderWindow->GetRenderers()->GetItemAsObject(0));
    renderer->SetViewport(0, 0, 1.0/this->ImageReductionFactor, 
                          1.0/this->ImageReductionFactor);
    }
  else
    {
    winInfoInt.FullSize[0] = winInfoInt.ReducedSize[0] = size[0];
    winInfoInt.FullSize[1] = winInfoInt.ReducedSize[1] = size[1];
    winInfoDouble.ImageReductionFactor = 1;
    }
  //  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfoInt.NumberOfRenderers = 1;
  winInfoInt.UseCompositing = this->UseCompositing;
  winInfoDouble.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  if ( size[0] == 0 || size[1] == 0 )
    {
    renWin->SwapBuffersOff();
    return;
    }

//  this->SetRendererSize(winInfoInt.FullSize[0]/this->ImageReductionFactor, 
//                        winInfoInt.FullSize[1]/this->ImageReductionFactor);
  this->FullImageSize[0] = winInfoInt.FullSize[0];
  this->FullImageSize[1] = winInfoInt.FullSize[1];
  this->ReducedImageSize[0] = winInfoInt.ReducedSize[0];
  this->ReducedImageSize[1] = winInfoInt.ReducedSize[1];
  
  this->ReallocDataArrays();
  
  for (id = 1; id < numProcs; ++id)
    {
    if (this->RenderEventPropagation)
      {
      controller->TriggerRMI(id, NULL, 0, 
                             vtkCompositeRenderManager::RENDER_RMI_TAG);
      }
    // Synchronize the size of the windows.
    controller->Send((int*)(&winInfoInt),
                     vtkParallelRenderManager::WIN_INFO_INT_SIZE,
                     id, 
                     vtkCompositeRenderManager::WIN_INFO_INT_TAG);
    controller->Send((double*)(&winInfoDouble),
                     vtkParallelRenderManager::WIN_INFO_DOUBLE_SIZE,
                     id, 
                     vtkCompositeRenderManager::WIN_INFO_DOUBLE_TAG);
    }
  
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
    renInfoInt.NumberOfLights = lc->GetNumberOfItems();
    ren->GetViewport(renInfoDouble.Viewport);
    if (this->ImageReductionFactor > 1)
      {
      this->Viewports->SetNumberOfTuples(1);
      this->Viewports->SetTuple(0, renInfoDouble.Viewport);
      }
    cam->GetPosition(renInfoDouble.CameraPosition);
    cam->GetFocalPoint(renInfoDouble.CameraFocalPoint);
    cam->GetViewUp(renInfoDouble.CameraViewUp);
    cam->GetWindowCenter(renInfoDouble.WindowCenter);
    renInfoDouble.CameraViewAngle = cam->GetViewAngle();
    cam->GetClippingRange(renInfoDouble.CameraClippingRange);
    if (cam->GetParallelProjection())
      {
      renInfoDouble.ParallelScale = cam->GetParallelScale();
      }
    else
      {
      renInfoDouble.ParallelScale = 0.0;
      }
    if (light)
      {
      lightInfoDouble.Type = (double)(light->GetLightType());
      light->GetPosition(lightInfoDouble.Position);
      light->GetFocalPoint(lightInfoDouble.FocalPoint);
      }
    ren->GetBackground(renInfoDouble.Background);
    
    for (id = 1; id < numProcs; ++id)
      {
      controller->Send((int*)(&renInfoInt),
                       vtkParallelRenderManager::REN_INFO_INT_SIZE,
                       id, 
                       vtkCompositeRenderManager::REN_INFO_INT_TAG);
      controller->Send(
        (double*)(&renInfoDouble),
        vtkParallelRenderManager::REN_INFO_DOUBLE_SIZE,
        id, 
        vtkCompositeRenderManager::REN_INFO_DOUBLE_TAG);
      controller->Send(
        (double*)(&lightInfoDouble),
        vtkParallelRenderManager::LIGHT_INFO_DOUBLE_SIZE,
        id, 
        vtkCompositeRenderManager::LIGHT_INFO_DOUBLE_TAG);
      }
  
  vtkTimerLog::MarkStartEvent("Render Geometry");
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::SatelliteStartRender()
{
  this->Superclass::SatelliteStartRender();
  this->ReallocDataArrays();
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::WriteFullImage()
{
  if (this->UseChar)
    {
    this->Superclass::WriteFullImage();
    }
  else
    {
    this->WriteFullFloatImage();
    }
}
  
//----------------------------------------------------------------------------
void vtkPVTreeComposite::WriteFullFloatImage()
{
  if (this->RenderWindowImageUpToDate || !this->WriteBackImages)
    {
    return;
    }

  if (this->MagnifyImages && (this->ImageReductionFactor > 1))
    {
    this->MagnifyReducedFloatImage();
    this->SetRenderWindowFloatPixelData(this->FullFloatImage,
                                        this->FullImageSize);
    }
  else
    {
    // Only write back image if it has already been read and potentially
    // changed.
    if (this->ReducedImageUpToDate)
      {
      this->SetRenderWindowFloatPixelData(this->ReducedFloatImage,
                                          this->ReducedImageSize);
      }
    }

  this->RenderWindowImageUpToDate = 1;
}

static void MagnifyFloatImageNearest(vtkFloatArray *fullImage,
                                     int fullImageSize[2],
                                     vtkFloatArray *reducedImage,
                                     int reducedImageSize[2],
                                     vtkTimerLog *timer)
{
  int numComp = reducedImage->GetNumberOfComponents();;

  fullImage->SetNumberOfComponents(numComp);
  fullImage->SetNumberOfTuples(fullImageSize[0]*fullImageSize[1]);

  timer->StartTimer();

  // Inflate image.
  float xstep = (float)reducedImageSize[0]/fullImageSize[0];
  float ystep = (float)reducedImageSize[1]/fullImageSize[1];
  float *lastsrcline = NULL;
  for (int y = 0; y < fullImageSize[1]; y++)
    {
    float *destline =
      fullImage->GetPointer(numComp*fullImageSize[0]*y);
    float *srcline =
      reducedImage->GetPointer(numComp*reducedImageSize[0]*(int)(ystep*y));
    for (int x = 0; x < fullImageSize[0]; x++)
      {
      int srcloc = numComp*(int)(x*xstep);
      int destloc = numComp*x;
      for (int i = 0; i < numComp; i++)
        {
        destline[destloc + i] = srcline[srcloc + i];
        }
      }
    lastsrcline = srcline;
    }

  timer->StopTimer();
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::MagnifyReducedFloatImage()
{
  if (this->FullImageUpToDate)
    {
    return;
    }

  this->ReadReducedImage();

  if (this->FullFloatImage->GetPointer(0) !=
      this->ReducedFloatImage->GetPointer(0))
    {
    // only supporting nearest right now; it's what was supported in
    // vtkCompositeManager (previous superclass) and linear doesn't seem to
    // work in new superclass (vtkParallelRenderManager), so not trying to do
    // it here
    MagnifyFloatImageNearest(this->FullFloatImage, this->FullImageSize,
                             this->ReducedFloatImage, this->ReducedImageSize,
                             this->Timer);

    // We log the image inflation under render time because it is inversely
    // proportional to the image size.  This makes the auto image reduction
    // calculation work better.
    this->RenderTime += this->Timer->GetElapsedTime();
    }

  this->FullImageUpToDate = 1;
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::ReadReducedImage()
{
  if (this->UseChar)
    {
    this->Superclass::ReadReducedImage();
    return;
    }
  
  if (this->ReducedImageUpToDate)
    {
    return;
    }

  this->Timer->StartTimer();

  if (this->ImageReductionFactor > 1)
    {
    this->RenderWindow->GetRGBAPixelData(0, 0, this->ReducedImageSize[0]-1,
                                         this->ReducedImageSize[1]-1,
                                         this->ChooseBuffer(),
                                         this->ReducedFloatImage);
    }
  else
    {
    this->RenderWindow->GetRGBAPixelData(0, 0, this->FullImageSize[0]-1,
                                         this->FullImageSize[1]-1,
                                         this->ChooseBuffer(),
                                         this->FullFloatImage);
    this->FullImageUpToDate = 1;
    this->ReducedFloatImage
      ->SetNumberOfComponents(this->FullFloatImage->GetNumberOfComponents());
    this->ReducedFloatImage->SetArray(this->FullFloatImage->GetPointer(0),
                                      this->FullFloatImage->GetSize(), 1);
    this->ReducedFloatImage->SetNumberOfTuples(this->FullFloatImage->GetNumberOfTuples());
    }

  this->Timer->StopTimer();
  this->ImageProcessingTime += this->Timer->GetElapsedTime();

  this->ReducedImageUpToDate = 1;  
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::SetRenderWindowFloatPixelData(vtkFloatArray *pixels,
                                                       const int pixelDimensions[2])
{
  this->RenderWindow->SetRGBAPixelData(0, 0,
                                       pixelDimensions[0]-1,
                                       pixelDimensions[1]-1,
                                       pixels,
                                       this->ChooseBuffer());
}

// Done disabling compositing when no data is on satelite processes.
//============================================================================



//----------------------------------------------------------------------------
void vtkPVTreeComposite::ComputeVisiblePropBounds(vtkRenderer *ren, 
                                                  double bounds[6])
{
  double tmp[6];
  double *pbds;
  int id, num;
  int numProps;
  vtkProp    *prop;
  vtkPropCollection *props;
  int foundOne = 0;
  
  if (!this->Controller)
    {
    return;
    }
  
  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->Controller->TriggerRMI(id,COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
    }

  bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
  bounds[1] = bounds[3] = bounds[5] = -VTK_DOUBLE_MAX;
  
  // Are there any pickable visible props?
  props = ren->GetProps();
  numProps = props->GetNumberOfItems();
  for (props->InitTraversal(); (prop = props->GetNextProp()); )
    {
    if ( prop->GetVisibility() && (prop->GetPickable()))
      {
      foundOne = 1;
      }
    }

  // Loop through all props collecting bounds.
  for (props->InitTraversal(); (prop = props->GetNextProp()); )
    {
    // Skip if invisible.  
    // Skip if it is not pickable and there are other pickable props. 
    if ( prop->GetVisibility() && (prop->GetPickable() || ! foundOne))
      {
      pbds = prop->GetBounds();
      // make sure we haven't got bogus bounds
      if ( pbds != NULL &&
           pbds[0] > -VTK_DOUBLE_MAX && pbds[1] < VTK_DOUBLE_MAX &&
           pbds[2] > -VTK_DOUBLE_MAX && pbds[3] < VTK_DOUBLE_MAX &&
           pbds[4] > -VTK_DOUBLE_MAX && pbds[5] < VTK_DOUBLE_MAX )
        {
        if (pbds[0] < bounds[0])
          {
          bounds[0] = pbds[0]; 
          }
        if (pbds[1] > bounds[1])
          {
          bounds[1] = pbds[1]; 
          }
        if (pbds[2] < bounds[2])
          {
          bounds[2] = pbds[2]; 
          }
        if (pbds[3] > bounds[3])
          {
          bounds[3] = pbds[3]; 
          }
        if (pbds[4] < bounds[4])
          {
          bounds[4] = pbds[4]; 
          }
        if (pbds[5] > bounds[5])
          {
          bounds[5] = pbds[5]; 
          }
        }//not bogus
      }
    }

  // Get the bounds from the rest of the processes.
  for (id = 1; id < num; ++id)
    {
    this->Controller->Receive(tmp, 6, id, vtkCompositeRenderManager::BOUNDS_TAG);
    if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
    if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
    if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
    if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
    if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
    if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
    }
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::ComputeVisiblePropBoundsRMI()
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  double bounds[6];
  
  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  ren = rens->GetNextItem();

  ren->ComputeVisiblePropBounds(bounds);

  this->Controller->Send(bounds, 6, 0, vtkCompositeRenderManager::BOUNDS_TAG);
}

//#########################################################################
// If we are not using MPI, just stub out this class so the super class
// will do every thing.
#ifdef VTK_USE_MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->MPIController = vtkMPIController::SafeDownCast(this->Controller);
  
  this->EnableAbort = 1;

  this->RootWaiting = 0;
  this->ReceivePending = 0;
  this->ReceiveMessage = 0;
  
  this->LocalProcessId = this->Controller ? 
                         this->Controller->GetLocalProcessId() : -1;

  this->RenderAborted = 0;
  
  this->Initialized = 0;
  
  this->UseChar = 1;
  this->UseRGB = 1;
  this->UseCompositing = 0;
  
  this->CompositeTime = 0;
  this->GetBuffersTime = 0;
  this->SetBuffersTime = 0;
  this->MaxRenderTime = 0;
  
  this->ExitInteractorTag = 0;
  
  this->ReducedFloatImage = vtkFloatArray::New();
  this->FullFloatImage = vtkFloatArray::New();
  this->TmpFloatPixelData = vtkFloatArray::New();
  
  this->RenderWindowInteractor = NULL;
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
  this->SetRenderWindow(NULL);
  this->MPIController = NULL;
    
  // sanity check
  if (this->ReceivePending)
    {
    vtkErrorMacro("A receive is still pending.");
    }  

  this->ReducedFloatImage->Delete();
  this->FullFloatImage->Delete();
  this->TmpFloatPixelData->Delete();
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForAbortRender()
{  
  if (this->EnableAbort == 0)
    {
    return;
    }
  
  if (this->LocalProcessId == 0)
    {
    this->RootAbortCheck();
    }
  else
    {
    this->SatelliteAbortCheck();
    }
}

//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  int abort;

  if (this->EnableAbort == 0)
    {
    return 0;
    }

  // Check for abort render has to be called at least once.
  if ( ! this->Initialized)
    {
    this->CheckForAbortRender();
    }
  
  if (this->LocalProcessId == 0)
    {
    this->RootFinalAbortCheck();
    }
  else
    {
    this->SatelliteFinalAbortCheck();
    }
  
  // Reset this for the next render.
  abort = this->RenderAborted;
  this->RenderAborted = 0;
  this->Initialized = 0;
  
  // Does this method really need to return a value?
  return abort;
}




#define VTK_STATUS_TAG           548934


// Root ------> Satellite
// Exaclty one of these two methods will be sent (by the root) to each
// satelite process.  They terminate the non blocking receive.  The second
// message is also used as a barrier to ensure all processes start
// compositing at the same time (Satellites wait for the request).
// Although this barrier is less efficient, I did not want the mess of
// cancelling compositing.
#define  VTK_ABORT_RENDER              0
#define  VTK_COMPOSITE                 1


// Root ------> Satellite
// When the root process has finished rendering, it waits for each of the
// satellite processes (one by one) to finish rendering.  This root sends
// this message to inform a satellite process that is waiting for it.  In a
// normal render (not aborted) each satellite process will get exactly one
// of these messages.  If rendering has been aborted, the the root does not
// bother sending this message to the remaining satellites.
#define  VTK_ROOT_WAITING              2

// Root <------ Satellite
// This message may be sent from any satellite processes to the root
// processes.  It is used to ping the root process when it has finished
// rendering and is waiting in a blocking receive for a "Finshed" message.
// Only the processes that is currently being waited on can send these
// messages.  Any number of them can be sent (including 0).
#define  VTK_CHECK_ABORT               3

// Root <------ Satellite
// This message is sent from the satellite (the root is actively waiting
// for) to signal that is done rendering and is waiting to composite.  In a
// normal (not aborted) render, every satellite process sends this message
// exactly once to the root.
#define  VTK_FINISHED                  4



//------------- Methods for Root Processes --------------

// Two satellites, possible message traces.
// Root:
// No Abort,
//----------
// send 1 ROOT_WAITING
// [rec 1 CHECK_ABORT]
// ... repeat any number of times ...               
// rec  1 FINISHED    
// send 2 ROOT_WAITING
// [rec 2 CHECK_ABORT]
// ... repeat any number of times ...               
// rec  2 FINISHED    
// send 1 COMPOSITE    
// send 2 COMPOSITE    

// Abort during waiting for 2.
//----------
// send 1 ROOT_WAITING
// [rec 1 CHECK_ABORT]
// ... repeat any number of times ...               
// rec  1 FINISHED    
// send 2 ROOT_WAITING
// [rec 2 CHECK_ABORT]
// ... repeat any number of times ...
// send 1 ABORT
// send 2 ABORT
// rec  2 FINISHED    

// Abort during waiting for 1.
//----------
// send 1 ROOT_WAITING
// [rec 1 CHECK_ABORT]
// ... repeat any number of times ...               
// send 1 ABORT
// send 2 ABORT
// rec  1 FINISHED    

// Abort during render.
//----------
// send 1 ABORT
// send 2 ABORT

//            Abort during waiting.    Abort during render


//-------------------------------------------------------------------------
// Count is temporary for testing.
void vtkPVTreeComposite::RootAbortCheck()
{
  //sleep(5);
  int abort;

  // If the render has already been aborted, then we need do nothing else.
  if (this->RenderAborted || !this->Controller)
    {
    return;
    }

  // This checks for events to decide whether to abort.
  this->InvokeEvent(vtkCommand::AbortCheckEvent);
  abort = this->RenderWindow->GetAbortRender();
  if (abort)
    { // Yes, abort.
    int idx;
    int message;
    int num = this->Controller->GetNumberOfProcesses();

    // Tell the satellite processes they need to abort.
    for (idx = 1; idx < num; ++idx)
      {
      //cout << "0 send to 1, message: " << message << endl;
      message = VTK_ABORT_RENDER;
      this->MPIController->Send(&message, 1, idx, VTK_STATUS_TAG);
      }
    
    // abort our own render.
    //this->RenderWindow->SetAbortRender(1);
    this->RenderAborted = 1;
    }
}


//-------------------------------------------------------------------------
// "abort" is 1 if rendering was previously aborted.
void vtkPVTreeComposite::RootFinalAbortCheck()
{
  int idx;
  int num;

  // If the render has already been aborted, then we need do nothing else.
  if (this->RenderAborted || !this->Controller)
    {
    return;
    }
  
  // Wait for all the satelite processes to finish.
  num = this->Controller->GetNumberOfProcesses();
  for (idx = 1; idx < num; ++idx)
    {
    // An abort could have occured while waiting for a satellite.
    if ( ! this->RenderAborted)
      {
      this->RootWaitForSatelliteToFinish(idx);
      }
    }

  // Sends the final message to all satellites.
  this->RootSendFinalCompositeDecision();
}



//-------------------------------------------------------------------------
// This only gets called when there has not been an abort. 
// If an abort occured during the call, then this method returns 1.
// It returns 0 if no abort occured.
void vtkPVTreeComposite::RootWaitForSatelliteToFinish(int satelliteId)
{
  int message;

  // Send a message to the process that informs it that we are waiting for
  // it to finish rendering, and expect to be pinged every so often.
  message = VTK_ROOT_WAITING;
  this->MPIController->Send(&message, 1, satelliteId, VTK_STATUS_TAG);
  
  // Wait for the process to finish.
  while (1)
    {
    this->MPIController->Receive(&message, 1, satelliteId, VTK_STATUS_TAG);

    // Even if we abort, We still expect the "FINISHED" message because the
    // satellite might sned the "FINISHED" message before it receives the
    // "ABORT" message.
    if (message == VTK_FINISHED)
      {
      return;
      }
    else if (message == VTK_CHECK_ABORT)
      {
      // The satellite is in the middle of a long render and has pinged us to
      // check for an abort.  This call sends the abort messages internally.
      this->RootAbortCheck();
      }
    else 
      {
      vtkErrorMacro("Sanity check failed: Expecting CheckAbort or Finished "
                    "message.");
      }
    }
}



//-------------------------------------------------------------------------
// This method has simplified to the point that it could be eliminated.
void vtkPVTreeComposite::RootSendFinalCompositeDecision()
{
  int message;
  int idx, num;
  
  if (!this->Controller)
    {
    return;
    }
  
  // If ABORT was already sent, then we do not need to worry about the
  // composite.  It is already cancelled.
  num = this->Controller->GetNumberOfProcesses();
  if ( ! this->RenderAborted)
    {
    for (idx = 1; idx < num; ++idx)
      {
      // In order to get rid of the abort asych receive still pending.
      message = VTK_COMPOSITE;
      this->MPIController->Send(&message, 1, idx, VTK_STATUS_TAG);
      }
    }
}



//------------- Methods for Satellite Processes --------------


//-------------------------------------------------------------------------
void vtkPVTreeComposite::SatelliteAbortCheck()
{
  int message;

  if (this->RenderAborted)
    {
    return;
    }
  
  // If the root is waiting on us, then ping it so that it can check for an
  // abort.
  if (this->RootWaiting)
    {
    //cout << "1: Ping root\n";
    vtkMPICommunicator::Request sendRequest;
    message = VTK_CHECK_ABORT;
    //cout << "1 noBlockSend to 0, message: " << status << endl;
    this->MPIController->NoBlockSend(&message, 1, 0, VTK_STATUS_TAG, 
                                     sendRequest);
    }
  
  // If this is the first call for this render, 
  // then we need to setup the receive message.
  if ( ! this->ReceivePending)
    {
    this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, 
                                        VTK_STATUS_TAG, 
                                        this->ReceiveRequest);
    this->ReceivePending = 1;
    }
  
  if (this->ReceivePending && this->ReceiveRequest.Test())
    { // Received a message from the root.
    this->ReceivePending = 0;
    
    // It could be ABORT, or ROOT_WAITING.  It could not be COMPOSITE
    // because that can only be called after root receives our FINISHED
    // message.
    
    if (this->ReceiveMessage == VTK_ABORT_RENDER)
      {  // Root is telling us to short circuit the render.
      // .... set abort flag of render window.....
      this->RenderWindow->SetAbortRender(1);
      this->RenderAborted = 1;
      // Do NOT rearm to asynchronous receive.
      return;
      }
    else if (this->ReceiveMessage == VTK_ROOT_WAITING)
      { // Root is finished rendering, and is waiting for this process.  
      // It wants to be pinged occasionally so it can check for aborts.
      this->RootWaiting = 1;
      // Rearm the receive to get a possible abort.
      this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, 
                                          VTK_STATUS_TAG, 
                                          this->ReceiveRequest);
      this->ReceivePending = 1;
      }
    else
      {
      vtkErrorMacro("Expecting ABORT or WAITING from root.");
      }
    }
  
  return;
}





//-------------------------------------------------------------------------
// Out process is finished rendering now and will wait for the final status 
// message from root.
void vtkPVTreeComposite::SatelliteFinalAbortCheck()
{
  int message;
  
  // We can not send a FINISHED message until the root is WAITING.
  if ( ! this->RootWaiting && ! this->RenderAborted)
    {
    // Wait for one of these messages: ROOT_WAITING, or ABORT
    if (this->ReceivePending)
      {
      this->ReceiveRequest.Wait();
      this->ReceivePending = 0;
      if (this->ReceiveMessage == VTK_ABORT_RENDER)
        {  // Root is telling us to short circuit the render.
        // We we have received a ROOT_WAITING message, then we have to send
        // a FINISHED message (event if an ABORT has been received meanwhile).
        this->RenderAborted = 1;
        }      
      else if (this->ReceiveMessage == VTK_ROOT_WAITING)
        { 
        this->RootWaiting = 1;
        // Rearm the receive to put in a consistent state.
        this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, 
                                            VTK_STATUS_TAG, 
                                            this->ReceiveRequest);
        this->ReceivePending = 1;
        }
      else 
        {
        vtkErrorMacro("Expecting ROOT_WAITING or ABORT message from root.");
        }
      }
    }
  
  
  // We we have received a ROOT_WAITING message, then we have to send
  // a FINISHED message (event if an ABORT has been received meanwhile).
  if ( this->RootWaiting)
    {
    message = VTK_FINISHED;
    this->MPIController->Send(&message, 1, 0, VTK_STATUS_TAG);
    // Reset the RootWaiting flag for the next render.
    this->RootWaiting = 0;
    }
  
  // If there has already been an ABORT, then receive will no longer be
  // pending.
  
  // Now tie up any loose ends.
  // Wait for one of these messages: COMPOSITE, or ABORT
  // We are gaurenteed to get exactly on of these two.
  // The receive would not still be pending if we received one already.
  if (this->ReceivePending)
    {
    this->ReceiveRequest.Wait();
    this->ReceivePending = 0;
    if (this->ReceiveMessage == VTK_ABORT_RENDER)
      {  // Root is telling us to short circuit the render.
      this->RenderAborted = 1;
      }
    else if (this->ReceiveMessage == VTK_COMPOSITE)
      {
      // We do not need to do anything here.
      }
    }
}



// end VTK_USE_MPI
//#########################################################################
#else
// Not using MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{ 
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForAbortRender()
{
  this->vtkCompositeRenderManager::CheckForAbortRender();
}

//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  return 0;
}

#endif  // VTK_USE_MPI

//----------------------------------------------------------------------------
void vtkPVTreeComposite::SetRenderWindow(vtkRenderWindow *renWin)
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
    rens = this->RenderWindow->GetRenderers();
    rens->InitTraversal();

    // Remove all of the observers.
    if (this->Controller && this->Controller->GetLocalProcessId() == 0)
      {
      // Will make do with renderer 0. (Assumes renderer does not change.)
      ren = vtkRenderer::SafeDownCast(rens->GetItemAsObject(0));
      if (ren)
        {
        //ren->RemoveObserver(this->ResetCameraTag);
        //ren->RemoveObserver(this->ResetCameraClippingRangeTag);
        ren->RemoveObserver(this->StartRenderTag);
        ren->RemoveObserver(this->EndRenderTag);
        }
      }
    if ( this->Controller && this->Controller->GetLocalProcessId() != 0 )
      {
      if (ren)
        {
        ren->RemoveObserver(this->StartRenderTag);
        ren->RemoveObserver(this->EndRenderTag);
        }
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
      // In case a subclass wants to check for aborts.
      vtkCallbackCommand* abc = vtkCallbackCommand::New();
      abc->SetCallback(vtkPVTreeCompositeAbortRenderCheck);
      abc->SetClientData(this);
      this->RenderWindow->AddObserver(vtkCommand::AbortCheckEvent, abc);
      abc->Delete();
      
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();

      if (this->Controller && this->Controller->GetLocalProcessId() == 0)
        {
        vtkCallbackCommand *cbc;

        // Will make do with renderer 0. (Assumes renderer does
        // not change.)
        ren = vtkRenderer::SafeDownCast(rens->GetItemAsObject(0));
        if (ren)
          {
          cbc= vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeStartRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->StartRenderTag = ren->AddObserver(vtkCommand::StartEvent,cbc);
          cbc->Delete();
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeEndRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->EndRenderTag = ren->AddObserver(vtkCommand::EndEvent,cbc);
          cbc->Delete();
          
          //cbc = vtkCallbackCommand::New();
          //cbc->SetCallback(vtkPVTreeCompositeResetCameraClippingRange);
          //cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          //this->ResetCameraClippingRangeTag = 
          //ren->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
          //cbc->Delete();          
          
          //cbc = vtkCallbackCommand::New();
          //cbc->SetCallback(vtkPVTreeCompositeResetCamera);
          //cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          //this->ResetCameraTag = 
          //ren->AddObserver(vtkCommand::ResetCameraEvent,cbc);
          //cbc->Delete();
          }
        }
      else if (this->Controller && this->Controller->GetLocalProcessId() != 0)
        {
        vtkCallbackCommand *cbc;

        // It is simpler to always use single buffer on the server.
        if (this->RenderWindow)
          {
          // Lets keep the render window single buffer
          this->RenderWindow->DoubleBufferOff();
          }        
        
        ren = rens->GetNextItem();
        if (ren)
          {
          cbc= vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeSatelliteStartRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->StartRenderTag = ren->AddObserver(vtkCommand::StartEvent,cbc);
          cbc->Delete();
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeSatelliteEndRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->EndRenderTag = ren->AddObserver(vtkCommand::EndEvent,cbc);
          cbc->Delete();
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::PostRenderProcessing()
{
  int front;
  int myId;
  myId = this->Controller->GetLocalProcessId();  
  
  // Stop the timer that has been timing the render.
  this->Timer->StopTimer();
  this->MaxRenderTime = this->Timer->GetElapsedTime();

  vtkTimerLog *timer = vtkTimerLog::New();
  
  // Get the z buffer.
  timer->StartTimer();
  vtkTimerLog::MarkStartEvent("GetZBuffer");
  this->RenderWindow->GetZbufferData(0,0,
                                     this->ReducedImageSize[0]-1, 
                                     this->ReducedImageSize[1]-1,
                                     this->DepthData);  
  vtkTimerLog::MarkEndEvent("GetZBuffer");

  // If we are process 0 and using double buffering, then we want 
  // to get the back buffer, otherwise we need to get the front.
  if (myId == 0)
    {
    front = 0;
    }
  else
    {
    front = 1;
    }

  // Get the pixel data.
  if (this->UseChar) 
    {
    if (this->ReducedImage->GetNumberOfComponents() == 4)
      {
      vtkTimerLog::MarkStartEvent("Get RGBA Char Buffer");
      this->RenderWindow->GetRGBACharPixelData(
        0,0,this->ReducedImageSize[0]-1,this->ReducedImageSize[1]-1, 
        front, this->ReducedImage);
      vtkTimerLog::MarkEndEvent("Get RGBA Char Buffer");
      }
    else if (this->ReducedImage->GetNumberOfComponents() == 3)
      {
      vtkTimerLog::MarkStartEvent("Get RGB Char Buffer");
      this->RenderWindow->GetPixelData(
        0,0,this->ReducedImageSize[0]-1,this->ReducedImageSize[1]-1, 
        front, this->ReducedImage);
      vtkTimerLog::MarkEndEvent("Get RGB Char Buffer");
      }
    } 
  else 
    {
    vtkTimerLog::MarkStartEvent("Get RGBA Float Buffer");
    this->RenderWindow->GetRGBAPixelData(
      0,0,this->ReducedImageSize[0]-1,this->ReducedImageSize[1]-1, 
      front, this->ReducedFloatImage);
    vtkTimerLog::MarkEndEvent("Get RGBA Float Buffer");
    }
  
  timer->StopTimer();
  this->GetBuffersTime = timer->GetElapsedTime();
  
  timer->StartTimer();
  
  // Let the subclass use its owns composite algorithm to
  // collect the results into "localPData" on process 0.
  vtkTimerLog::MarkStartEvent("Composite Buffers");
  
  this->TmpDepthData->SetNumberOfComponents(
    this->DepthData->GetNumberOfComponents());
  this->TmpDepthData->SetNumberOfTuples(
    this->DepthData->GetNumberOfTuples());

  if (this->UseChar)
    {
    this->TmpPixelData->SetNumberOfComponents(
      this->ReducedImage->GetNumberOfComponents());
    this->TmpPixelData->SetNumberOfTuples(
      this->ReducedImage->GetNumberOfTuples());
    this->Compositer->CompositeBuffer(this->ReducedImage, this->DepthData,
                                      this->TmpPixelData, this->TmpDepthData);
    }
  else
    {
    this->TmpFloatPixelData->SetNumberOfComponents(
      this->ReducedImage->GetNumberOfComponents());
    this->TmpFloatPixelData->SetNumberOfTuples(
      this->ReducedImage->GetNumberOfTuples());
    this->Compositer->CompositeBuffer(this->ReducedFloatImage, this->DepthData,
                                      this->TmpFloatPixelData, this->TmpDepthData);
    }
    
  vtkTimerLog::MarkEndEvent("Composite Buffers");

  timer->StopTimer();
  this->CompositeTime = timer->GetElapsedTime();
  this->RenderWindowImageUpToDate = 0;
  this->ReducedImageUpToDate = 1;
  
  timer->Delete();
  timer = NULL;
}

//-------------------------------------------------------------------------
float vtkPVTreeComposite::GetZ(int x, int y)
{
  int idx;
  
  if (this->Controller == NULL ||
      this->Controller->GetNumberOfProcesses() == 1)
    {
    int *size = this->RenderWindow->GetSize();
    
    // Make sure we have default values.
    this->ImageReductionFactor = 1;
//    this->SetRendererSize(size[0], size[1]);
    this->FullImageSize[0] = this->ReducedImageSize[0] = size[0];
    this->FullImageSize[1] = this->ReducedImageSize[1] = size[1];
    
    this->ReallocDataArrays();
    
    // Get the z buffer.
    this->RenderWindow->GetZbufferData(0,0,size[0]-1, size[1]-1, 
                                       this->DepthData);
    }
  
  if (x < 0 || x >= this->FullImageSize[0] || 
      y < 0 || y >= this->FullImageSize[1])
    {
    return 0.0;
    }
  
  if (this->ImageReductionFactor > 1)
    {
    idx = (int)((x + (y * this->FullImageSize[0] / this->ImageReductionFactor)) 
                / this->ImageReductionFactor);
    }
  else 
    {
    idx = (x + (y * this->FullImageSize[0]));
    }

  return this->DepthData->GetValue(idx);
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::SetRenderWindowInteractor(
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
      this->RenderWindowInteractor->RemoveObserver(this->ExitInteractorTag);
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
      cbc->SetCallback(vtkPVTreeCompositeExitInteractor);
      cbc->SetClientData((void*)this);
      // IRen will delete the cbc when the observer is removed.
      this->ExitInteractorTag = iren->AddObserver(vtkCommand::ExitEvent,cbc);
      cbc->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::ExitInteractor()
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

//----------------------------------------------------------------------------
void vtkPVTreeComposite::SetUseChar(int useChar)
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
  
  this->ReallocDataArrays();
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::SetUseRGB(int useRGB)
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
  
  this->ReallocDataArrays();
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::ReallocDataArrays()
{
  int numComps = 4;
  int numTuples = this->ReducedImageSize[0] * this->ReducedImageSize[1];

  if (numTuples < 0) // happens if we haven't composited anything yet
    {
    return;
    }
  
  if (this->UseRGB)
    {
    numComps = 3;
    }

  if (this->ReducedImage)
    {
    vtkPVTreeComposite::DeleteArray(this->ReducedImage);
    this->ReducedImage = NULL;
    }
  if (this->ReducedFloatImage)
    {
    vtkPVTreeComposite::DeleteArray(this->ReducedFloatImage);
    this->ReducedFloatImage = NULL;
    }
  if (this->DepthData)
    {
    vtkPVTreeComposite::DeleteArray(this->DepthData);
    this->DepthData = NULL;
    }

  this->ReducedImage = vtkUnsignedCharArray::New();
  vtkPVTreeComposite::ResizeUnsignedCharArray(this->ReducedImage,
                                              numComps, numTuples);
  
  this->ReducedFloatImage = vtkFloatArray::New();
  vtkPVTreeComposite::ResizeFloatArray(this->ReducedFloatImage,
                                       numComps, numTuples);
  
  this->DepthData = vtkFloatArray::New();
  vtkPVTreeComposite::ResizeFloatArray(this->DepthData, 1, numTuples);
}

//-------------------------------------------------------------------------
void vtkPVTreeComposite::ResizeFloatArray(vtkFloatArray* fa, int numComp,
                                          vtkIdType size)
{
  fa->SetNumberOfComponents(numComp);

#ifdef MPIPROALLOC
  vtkIdType fa_size = fa->GetSize();
  if ( fa_size < size*numComp )
    {
    float* ptr = fa->GetPointer(0);
    if (ptr)
      {
      MPI_Free_mem(ptr);
      }
    char* tptr;
    MPI_Alloc_mem(size*numComp*sizeof(float), NULL, &tptr);
    ptr = (float*)tptr;
    fa->SetArray(ptr, size*numComp, 1);
    }
  else
    {
    fa->SetNumberOfTuples(size);
    }
#else
  fa->SetNumberOfTuples(size);
#endif
}

//-------------------------------------------------------------------------
void vtkPVTreeComposite::ResizeUnsignedCharArray(vtkUnsignedCharArray* uca, 
                                                 int numComp, vtkIdType size)
{
  uca->SetNumberOfComponents(numComp);
#ifdef MPIPROALLOC
  vtkIdType uca_size = uca->GetSize();

  if ( uca_size < size*numComp )
    {
    unsigned char* ptr = uca->GetPointer(0);
    if (ptr)
      {
      MPI_Free_mem(ptr);
      }
    char* tptr;
    MPI_Alloc_mem(size*numComp*sizeof(unsigned char), NULL, &tptr);
    ptr = (unsigned char*)tptr;
    uca->SetArray(ptr, size*numComp, 1);
    }
  else
    {
    uca->SetNumberOfTuples(size);
    }
#else
  uca->SetNumberOfTuples(size);
#endif
}

//-------------------------------------------------------------------------
void vtkPVTreeComposite::DeleteArray(vtkDataArray* da)
{
#ifdef MPIPROALLOC
  void* ptr = da->GetVoidPointer(0);
  if (ptr)
    {CompositeManager
    MPI_Free_mem(ptr);
    }
#endif
  da->Delete();
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EnableAbort: " << this->GetEnableAbort() << endl;
  os << indent << "CompositeTime: " << this->CompositeTime << endl;
  os << indent << "SetBuffersTime: " << this->SetBuffersTime << endl;
  os << indent << "GetBuffersTime: " << this->GetGetBuffersTime() << endl;
  os << indent << "MaxRenderTime: " << this->MaxRenderTime << endl;
  os << indent << "UseChar: " << this->UseChar << endl;
  os << indent << "UseRGB: " << this->UseRGB << endl;
}
