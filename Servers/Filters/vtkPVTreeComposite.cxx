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
#include "vtkProcessModule.h"
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
vtkCxxRevisionMacro(vtkPVTreeComposite, "1.63");


//----------------------------------------------------------------------------
void vtkPVTreeCompositeCheckForDataRMI(void *arg, void *, int, int)
{
  vtkPVTreeComposite* self = (vtkPVTreeComposite*) arg;
  
  self->CheckForDataRMI();
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
  this->Controller->Send(&dataFlag, 1, 
                         0, vtkProcessModule::TreeCompositeDataFlag);
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
    this->Controller->Receive(&tmp, 1, 
                              idx, vtkProcessModule::TreeCompositeDataFlag);
    if (tmp)
      {
      dataFlag = 1;
      }  
    }
  
  return dataFlag;
}



//-------------------------------------------------------------------------
// No need to turn swap buffers off.
void vtkPVTreeComposite::PreRenderProcessing()
{
  if (!this->UseCompositing)
    {
    return;
    }

  this->ReallocDataArrays();

  this->RenderWindow->SwapBuffersOff();
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
  
  this->ReducedFloatImage = vtkFloatArray::New();
  this->FullFloatImage = vtkFloatArray::New();
  this->TmpFloatPixelData = vtkFloatArray::New();
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
      this->MPIController->Send(&message, 1, 
                                idx, vtkProcessModule::TreeCompositeStatus);
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
  this->MPIController->Send(&message, 1, 
                            satelliteId, vtkProcessModule::TreeCompositeStatus);
  
  // Wait for the process to finish.
  while (1)
    {
    this->MPIController->Receive(
      &message, 1, satelliteId, vtkProcessModule::TreeCompositeStatus);

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
      this->MPIController->Send(
        &message, 1, idx, vtkProcessModule::TreeCompositeStatus);
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
    this->MPIController->NoBlockSend(
      &message, 1, 
      0, vtkProcessModule::TreeCompositeStatus, 
      sendRequest);
    }
  
  // If this is the first call for this render, 
  // then we need to setup the receive message.
  if ( ! this->ReceivePending)
    {
    this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, 
                                        vtkProcessModule::TreeCompositeStatus, 
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
                                          vtkProcessModule::TreeCompositeStatus, 
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
        this->MPIController->NoBlockReceive(
          &this->ReceiveMessage, 1, 0, vtkProcessModule::TreeCompositeStatus, 
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
    this->MPIController->Send(&message, 1, 
                              0, vtkProcessModule::TreeCompositeStatus);
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
void vtkPVTreeComposite::PostRenderProcessing()
{
  if (!this->UseCompositing)
    {
    return;
    }
  
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

  // Get the pixel data.
  if (this->UseChar) 
    {
    if (this->ReducedImage->GetNumberOfComponents() == 4)
      {
      vtkTimerLog::MarkStartEvent("Get RGBA Char Buffer");
      this->RenderWindow->GetRGBACharPixelData(
        0,0,this->ReducedImageSize[0]-1,this->ReducedImageSize[1]-1, 
        this->ChooseBuffer(), this->ReducedImage);
      vtkTimerLog::MarkEndEvent("Get RGBA Char Buffer");
      }
    else if (this->ReducedImage->GetNumberOfComponents() == 3)
      {
      vtkTimerLog::MarkStartEvent("Get RGB Char Buffer");
      this->RenderWindow->GetPixelData(
        0,0,this->ReducedImageSize[0]-1,this->ReducedImageSize[1]-1, 
        this->ChooseBuffer(), this->ReducedImage);
      vtkTimerLog::MarkEndEvent("Get RGB Char Buffer");
      }
    } 
  else 
    {
    vtkTimerLog::MarkStartEvent("Get RGBA Float Buffer");
    this->RenderWindow->GetRGBAPixelData(
      0,0,this->ReducedImageSize[0]-1,this->ReducedImageSize[1]-1, 
      this->ChooseBuffer(), this->ReducedFloatImage);
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

  this->WriteFullImage();
  this->RenderWindow->SwapBuffersOn();
  this->RenderWindow->Frame();
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
