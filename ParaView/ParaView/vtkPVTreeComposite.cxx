/*=========================================================================
  
  Program:   ParaView
  Module:    vtkPVTreeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
vtkCxxRevisionMacro(vtkPVTreeComposite, "1.39.4.3");


//=========================================================================
// Stuff to avoid compositing if there is no data on statlite processes.


//----------------------------------------------------------------------------
void vtkPVTreeCompositeCheckForDataRMI(void *arg, void *, int, int)
{
  vtkPVTreeComposite* self = (vtkPVTreeComposite*) arg;
  
  self->CheckForDataRMI();
}


struct vtkPVTreeCompositeRenderWindowInfo 
{
  int Size[2];
  int ImageReductionFactor;
  int NumberOfRenderers;
  float DesiredUpdateRate;
};

struct vtkPVTreeCompositeRendererInfo 
{
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float LightPosition[3];
  float LightFocalPoint[3];
  float Background[3];
  float ParallelScale;
};

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

  this->vtkCompositeManager::InitializeRMIs();

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
  numProcs = this->NumberOfProcesses;
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
void vtkPVTreeComposite::InternalStartRender()
{
  struct vtkPVTreeCompositeRenderWindowInfo winInfo;
  struct vtkPVTreeCompositeRendererInfo renInfo;
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
  
  // Trigger the satellite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  numProcs = this->NumberOfProcesses;
  size = this->RenderWindow->GetSize();
  if (this->ImageReductionFactor > 0)
    {
    winInfo.Size[0] = size[0];
    winInfo.Size[1] = size[1];
    winInfo.ImageReductionFactor = this->ImageReductionFactor;
    vtkRenderer* renderer =
      ((vtkRenderer*)this->RenderWindow->GetRenderers()->GetItemAsObject(0));
    renderer->SetViewport(0, 0, 1.0/this->ImageReductionFactor, 
                          1.0/this->ImageReductionFactor);
    }
  else
    {
    winInfo.Size[0] = size[0];
    winInfo.Size[1] = size[1];
    winInfo.ImageReductionFactor = 1;
    }
//  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfo.NumberOfRenderers = 1;
  winInfo.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  if ( winInfo.Size[0] == 0 || winInfo.Size[1] == 0 )
    {
    this->FirstRender = 1;
    renWin->SwapBuffersOff();
    return;
    }

  this->SetRendererSize(winInfo.Size[0]/this->ImageReductionFactor, 
                        winInfo.Size[1]/this->ImageReductionFactor);
  
  for (id = 1; id < numProcs; ++id)
    {
    if (this->Manual == 0)
      {
      controller->TriggerRMI(id, NULL, 0, 
                             vtkCompositeManager::RENDER_RMI_TAG);
      }
    // Synchronize the size of the windows.
    controller->Send((char*)(&winInfo), 
                     sizeof(vtkPVTreeCompositeRenderWindowInfo), id, 
                     vtkCompositeManager::WIN_INFO_TAG);
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
    cam->GetPosition(renInfo.CameraPosition);
    cam->GetFocalPoint(renInfo.CameraFocalPoint);
    cam->GetViewUp(renInfo.CameraViewUp);
    cam->GetClippingRange(renInfo.CameraClippingRange);
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
                       sizeof(struct vtkPVTreeCompositeRendererInfo), id, 
                       vtkCompositeManager::REN_INFO_TAG);
      }
//    }
  
  // Turn swap buffers off before the render so the end render method
  // has a chance to add to the back buffer.
  renWin->SwapBuffersOff();

  vtkTimerLog::MarkStartEvent("Render Geometry");
}

// Done disabling compositing when no data is on satelite processes.
//============================================================================



//----------------------------------------------------------------------------
void vtkPVTreeComposite::ComputeVisiblePropBounds(vtkRenderer *ren, 
                                                  float bounds[6])
{
  float tmp[6];
  float *pbds;
  int id, num;
  int numProps;
  vtkProp    *prop;
  vtkPropCollection *props;
  int foundOne = 0;
  
  num = this->NumberOfProcesses;  
  for (id = 1; id < num; ++id)
    {
    this->Controller->TriggerRMI(id,COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
    }

  bounds[0] = bounds[2] = bounds[4] = VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;
  
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
           pbds[0] > -VTK_LARGE_FLOAT && pbds[1] < VTK_LARGE_FLOAT &&
           pbds[2] > -VTK_LARGE_FLOAT && pbds[3] < VTK_LARGE_FLOAT &&
           pbds[4] > -VTK_LARGE_FLOAT && pbds[5] < VTK_LARGE_FLOAT )
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
    this->Controller->Receive(tmp, 6, id, vtkCompositeManager::BOUNDS_TAG);
    if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
    if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
    if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
    if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
    if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
    if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
    }
}



//#########################################################################
// If we are not using MPI, just stub out this class so the super class
// will do every thing.
#ifdef VTK_USE_MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{
  this->MPIController = vtkMPIController::SafeDownCast(this->Controller);
  
  this->EnableAbort = 1;

  this->RootWaiting = 0;
  this->ReceivePending = 0;
  this->ReceiveMessage = 0;
  
  if (this->MPIController == NULL)
    {
    vtkErrorMacro("This objects requires an MPI controller.");
    this->LocalProcessId = 0;
    }
  else
    {
    this->LocalProcessId = this->Controller->GetLocalProcessId();
    }
  this->RenderAborted = 0;
  
  this->Initialized = 0;
  
  this->UseChar = 1;
  this->UseCompositing = 0;
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
  this->MPIController = NULL;
    
  // sanity check
  if (this->ReceivePending)
    {
    vtkErrorMacro("A receive is still pending.");
    }  
    
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


// Root ------>>>>>>>> Satellite
// Exaclty one of these two methods will be sent (by the root) to each
// satelite process.  They terminate the non blocking receive.  The second
// message is also used as a barrier to ensure all processes start
// compositing at the same time (Satellites wait for the request).
// Although this barrier is less efficient, I did not want the mess of
// cancelling compositing.
#define  VTK_ABORT_RENDER              0
#define  VTK_COMPOSITE                 1


// Root ------>>>>>>>> Satellite
// When the root process has finished rendering, it waits for each of the
// satellite processes (one by one) to finish rendering.  This root sends
// this message to inform a satellite process that is waiting for it.  In a
// normal render (not aborted) each satellite process will get exactly one
// of these messages.  If rendering has been aborted, the the root does not
// bother sending this message to the remaining satellites.
#define  VTK_ROOT_WAITING              2

// Root <<<<<<<<------ Satellite
// This message may be sent from any satellite processes to the root
// processes.  It is used to ping the root process when it has finished
// rendering and is waiting in a blocking receive for a "Finshed" message.
// Only the processes that is currently being waited on can send these
// messages.  Any number of them can be sent (including 0).
#define  VTK_CHECK_ABORT               3

// Root <<<<<<<<------ Satellite
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
  if (this->RenderAborted)
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
    int num = this->NumberOfProcesses;

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
// "abort" is true if rendering was previously aborted.
void vtkPVTreeComposite::RootFinalAbortCheck()
{
  int idx;
  int num;

  // If the render has already been aborted, then we need do nothing else.
  if (this->RenderAborted)
    {
    return;
    }
  
  // Wait for all the satelite processes to finish.
  num = this->NumberOfProcesses;
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
  
  // If ABORT was already sent, then we do not need to worry about the
  // composite.  It is already cancelled.
  num = this->NumberOfProcesses;
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
  this->vtkCompositeManager::CheckForAbortRender();
}

//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  return this->vtkCompositeManager::CheckForAbortComposite();
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
        ren->RemoveObserver(this->ResetCameraTag);
        ren->RemoveObserver(this->ResetCameraClippingRangeTag);
        ren->RemoveObserver(this->StartTag);
        ren->RemoveObserver(this->EndTag);
        }
      }
    if ( this->Controller && this->Controller->GetLocalProcessId() != 0 )
      {
      ren = rens->GetNextItem();
      if (ren)
        {
        ren->RemoveObserver(this->StartTag);
        ren->RemoveObserver(this->EndTag);
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
          this->StartTag = ren->AddObserver(vtkCommand::StartEvent,cbc);
          cbc->Delete();
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeEndRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->EndTag = ren->AddObserver(vtkCommand::EndEvent,cbc);
          cbc->Delete();
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeResetCameraClippingRange);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->ResetCameraClippingRangeTag = 
          ren->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
          cbc->Delete();          
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeResetCamera);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->ResetCameraTag = 
          ren->AddObserver(vtkCommand::ResetCameraEvent,cbc);
          cbc->Delete();
          }
        }
      else if (this->Controller && this->Controller->GetLocalProcessId() != 0)
        {
        vtkCallbackCommand *cbc;
        
        ren = rens->GetNextItem();
        if (ren)
          {
          cbc= vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeSatelliteStartRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->StartTag = ren->AddObserver(vtkCommand::StartEvent,cbc);
          cbc->Delete();
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(vtkPVTreeCompositeSatelliteEndRender);
          cbc->SetClientData((void*)this);
          // ren will delete the cbc when the observer is removed.
          this->EndTag = ren->AddObserver(vtkCommand::EndEvent,cbc);
          cbc->Delete();
          }
        }
      else
        {
#ifdef _WIN32
        // I had a problem with some graphics cards getting front and
        // back buffers mixed up, so I made the remote render windows
        // single buffered. One nice feature of this is being able to
        // see the render in these helper windows.
        vtkWin32OpenGLRenderWindow *renWin;
  
        renWin = vtkWin32OpenGLRenderWindow::SafeDownCast(this->RenderWindow);
        if (renWin)
          {
          // Lets keep the render window single buffer
          renWin->DoubleBufferOff();
          // I do not want to replace the original.
          renWin = renWin;
          }
#endif
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::Composite()
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
                                     this->RendererSize[0]-1, 
                                     this->RendererSize[1]-1,
                                     this->LocalZData);  
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
    if (this->LocalPData->GetNumberOfComponents() == 4)
      {
      vtkTimerLog::MarkStartEvent("Get RGBA Char Buffer");
      this->RenderWindow->GetRGBACharPixelData(
        0,0,this->RendererSize[0]-1,this->RendererSize[1]-1, 
        front,static_cast<vtkUnsignedCharArray*>(this->LocalPData));
      vtkTimerLog::MarkEndEvent("Get RGBA Char Buffer");
      }
    else if (this->LocalPData->GetNumberOfComponents() == 3)
      {
      vtkTimerLog::MarkStartEvent("Get RGB Char Buffer");
      this->RenderWindow->GetPixelData(
        0,0,this->RendererSize[0]-1,this->RendererSize[1]-1, 
        front,static_cast<vtkUnsignedCharArray*>(this->LocalPData));
      vtkTimerLog::MarkEndEvent("Get RGB Char Buffer");
      }
    } 
  else 
    {
    vtkTimerLog::MarkStartEvent("Get RGBA Float Buffer");
    this->RenderWindow->GetRGBAPixelData(
      0,0,this->RendererSize[0]-1,this->RendererSize[1]-1, 
      front,static_cast<vtkFloatArray*>(this->LocalPData));
    vtkTimerLog::MarkEndEvent("Get RGBA Float Buffer");
    }
  
  timer->StopTimer();
  this->GetBuffersTime = timer->GetElapsedTime();
  
  timer->StartTimer();
  
  // Let the subclass use its owns composite algorithm to
  // collect the results into "localPData" on process 0.
  vtkTimerLog::MarkStartEvent("Composite Buffers");
  this->Compositer->CompositeBuffer(this->LocalPData, this->LocalZData,
                                    this->PData, this->ZData);
    
  vtkTimerLog::MarkEndEvent("Composite Buffers");

  timer->StopTimer();
  this->CompositeTime = timer->GetElapsedTime();
    
  if (myId == 0) 
    {
    int windowSize[2];
    // Default value (no reduction).
    windowSize[0] = this->RendererSize[0];
    windowSize[1] = this->RendererSize[1];

    vtkDataArray* magPdata = 0;
    
    if (this->ImageReductionFactor > 1 && this->DoMagnifyBuffer)
      {
      // localPdata gets freed (new memory is allocated and returned.
      // windowSize get modified.
      if (this->UseChar)
        {
        magPdata = vtkUnsignedCharArray::New();
        }
      else
        {
        magPdata = vtkFloatArray::New();
        }
      magPdata->SetNumberOfComponents(
        this->LocalPData->GetNumberOfComponents());
      vtkTimerLog::MarkStartEvent("Magnify Buffer");
      this->MagnifyBuffer(this->LocalPData, magPdata, windowSize);
      vtkTimerLog::MarkEndEvent("Magnify Buffer");
      
      vtkRenderer* renderer =
        ((vtkRenderer*)
         this->RenderWindow->GetRenderers()->GetItemAsObject(0));
      renderer->SetViewport(0, 0, 1.0, 1.0);
      renderer->GetActiveCamera()->UpdateViewport(renderer);
      }

    
    timer->StartTimer();
    if (this->UseChar) 
      {
      vtkUnsignedCharArray *buf;
      if (magPdata)
        {
        buf = static_cast<vtkUnsignedCharArray*>(magPdata);
        }
      else
        {
        buf = static_cast<vtkUnsignedCharArray*>(this->LocalPData);
        }
      if (this->LocalPData->GetNumberOfComponents() == 4)
        {
        vtkTimerLog::MarkStartEvent("Set RGBA Char Buffer");
        this->RenderWindow->SetRGBACharPixelData(0, 0, windowSize[0]-1, 
                                  windowSize[1]-1, buf, 0);
        vtkTimerLog::MarkEndEvent("Set RGBA Char Buffer");
        }
      else if (this->LocalPData->GetNumberOfComponents() == 3)
        {
        vtkTimerLog::MarkStartEvent("Set RGB Char Buffer");
        this->RenderWindow->SetPixelData(0, 0, windowSize[0]-1, 
                                         windowSize[1]-1, buf, 0);
        vtkTimerLog::MarkEndEvent("Set RGB Char Buffer");
        }
      } 
    else 
      {
      if (magPdata)
        {
        vtkTimerLog::MarkStartEvent("Set RGBA Float Buffer");
        this->RenderWindow->SetRGBAPixelData(0, 0, windowSize[0]-1, 
                                             windowSize[1]-1,
                                             static_cast<vtkFloatArray*>(magPdata), 0);
        vtkTimerLog::MarkEndEvent("Set RGBA Float Buffer");
        }
      else
        {
        vtkTimerLog::MarkStartEvent("Set RGBA Float Buffer");
        this->RenderWindow->SetRGBAPixelData(
          0, 0, windowSize[0]-1, windowSize[1]-1,
          static_cast<vtkFloatArray*>(this->LocalPData), 0);
        vtkTimerLog::MarkEndEvent("Set RGBA Float Buffer");
        }
      }
    
    timer->StopTimer();
    this->SetBuffersTime = timer->GetElapsedTime();

    if (magPdata)
      {
      magPdata->Delete();
      }    
    }
  
  timer->Delete();
  timer = NULL;
}

//----------------------------------------------------------------------------
void vtkPVTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EnableAbort: " << this->GetEnableAbort() << endl;
}
