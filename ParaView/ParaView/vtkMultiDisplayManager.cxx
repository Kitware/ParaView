/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiDisplayManager.cxx
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

#include "vtkMultiDisplayManager.h"
#include "vtkMath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkTimerLog.h"
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
#include "vtkCompressCompositer.h"
#include "vtkPVCompositeBuffer.h"
#include "vtkPVCompositeUtilities.h"
#include "vtkTiledDisplaySchedule.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkMultiDisplayManager, "1.6");
vtkStandardNewMacro(vtkMultiDisplayManager);

vtkCxxSetObjectMacro(vtkMultiDisplayManager, RenderView, vtkObject);


// Structures to communicate render info.
// Marshaling is easier if we use all floats. (24)
class vtkPVMultiDisplayInfo 
{
public:
  vtkPVMultiDisplayInfo();
  float UseCompositing;
  float ReductionFactor;
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

vtkPVMultiDisplayInfo::vtkPVMultiDisplayInfo()
{
  this->UseCompositing = 0.0;
  this->ReductionFactor = 1.0;
  this->CameraPosition[0] = 0.0;               
  this->CameraPosition[1] = 0.0;               
  this->CameraPosition[2] = 0.0;               
  this->CameraFocalPoint[0] = 0.0;    
  this->CameraFocalPoint[1] = 0.0;    
  this->CameraFocalPoint[2] = 0.0;    
  this->CameraViewUp[0] = 0.0;  
  this->CameraViewUp[1] = 0.0;  
  this->CameraViewUp[2] = 0.0;  
  this->CameraClippingRange[0] = 0.0; 
  this->CameraClippingRange[1] = 0.0; 
  this->LightPosition[0] = 0.0;
  this->LightPosition[1] = 0.0;
  this->LightPosition[2] = 0.0;
  this->LightFocalPoint[0] = 0.0;
  this->LightFocalPoint[1] = 0.0;
  this->LightFocalPoint[2] = 0.0;
  this->Background[0] = 0.0;
  this->Background[1] = 0.0;
  this->Background[2] = 0.0;
  this->ParallelScale = 0.0;                         
  this->CameraViewAngle = 0.0;                       
  }
  


//-------------------------------------------------------------------------
vtkMultiDisplayManager::vtkMultiDisplayManager()
{
  this->ClientFlag = 0;

  this->ReductionFactor = 1;
  this->LODReductionFactor = 4;

  this->RenderWindow = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->SocketController = NULL;
  this->NumberOfProcesses = this->Controller->GetNumberOfProcesses();

  if (this->Controller)
    {
    this->Controller->Register(this);
    }

  this->StartTag = this->EndTag = 0;
  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;
  this->RenderView = NULL;

  this->Schedule = NULL;
  this->ZeroEmpty = 1;
  this->UseCompositing = 0;

  this->CompositeUtilities = vtkPVCompositeUtilities::New();

  this->Schedule = vtkTiledDisplaySchedule::New();

}

  
//-------------------------------------------------------------------------
vtkMultiDisplayManager::~vtkMultiDisplayManager()
{
  this->SetRenderWindow(NULL);
  
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }
  if (this->SocketController)
    {
    this->SocketController->UnRegister(this);
    this->SocketController = NULL;
    }

  this->CompositeUtilities->Delete();
  this->CompositeUtilities = NULL;

  this->Schedule->Delete();
  this->Schedule = NULL;
}

//==================== CallbackCommand and RMI functions ====================

//-------------------------------------------------------------------------
// Called by the render window start event. 
void vtkMultiDisplayManagerClientStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkMultiDisplayManager *self = (vtkMultiDisplayManager *)clientData;

  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->ClientStartRender();
}

//-------------------------------------------------------------------------
// Called by the render window start event. 
void vtkMultiDisplayManagerClientEndRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  (void)caller;
  vtkMultiDisplayManager *self = (vtkMultiDisplayManager *)clientData;

  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->ClientEndRender();
}


typedef void (*vtkRMIFunctionType)(void *localArg, 
                                   void *remoteArg, int remoteArgLength, 
                                   int remoteProcessId);

//-------------------------------------------------------------------------
void vtkMultiDisplayManagerRootStartRender(void *localArg, 
                                           void *, int, int)
{
  vtkMultiDisplayManager *self = (vtkMultiDisplayManager *)localArg;
  vtkMultiProcessController *controller = self->GetSocketController();
  vtkPVMultiDisplayInfo info;  

  controller->Receive((float*)(&info), 24, 1, 
                     vtkMultiDisplayManager::INFO_TAG);
  self->RootStartRender(info);
}

//-------------------------------------------------------------------------
void vtkMultiDisplayManagerSatelliteStartRender(void *localArg, 
                                                void *, int, int)
{
  vtkMultiDisplayManager *self = (vtkMultiDisplayManager *)localArg;
  vtkMultiProcessController *controller = self->GetController();
  vtkPVMultiDisplayInfo info;  

  controller->Receive((float*)(&info), 24, 0, 
                     vtkMultiDisplayManager::INFO_TAG);
  self->SatelliteStartRender(info);
}



//==================== end of callback and RMI functions ====================


//-------------------------------------------------------------------------
// o is origin of window, x is last point on x axis, 
// y is last point on y axis, and p is position of viewer.
// Set camera assumeing that all points/window is in world coordinates.
void vtkMultiDisplayManager::ComputeCamera(float *o, float *x, float *y,
                                           float *p, vtkCamera* cam)
{
  int idx;
  float vn[3];
  float ox[3];
  float oy[3];
  float cp[3];
  float center[3];
  float offset[3];
  float xOffset, yOffset;
  float dist;
  float height;
  float width;
  float viewAngle;
  float tmp;

  // Compute the view plane normal.
  for ( idx = 0; idx < 3; ++idx)
    {
    ox[idx] = x[idx] - o[idx];
    oy[idx] = y[idx] - o[idx];
    center[idx] = o[idx] + 0.5*(ox[idx] + oy[idx]);
    cp[idx] = p[idx] - center[idx];
    }
  vtkMath::Cross(ox, oy, vn);
  vtkMath::Normalize(vn);
  // Compute distance to plane.
  dist = vtkMath::Dot(vn,cp);
  // Compute width and height of the window.
  width = sqrt(ox[0]*ox[0] + ox[1]*ox[1] + ox[2]*ox[2]);
  height = sqrt(oy[0]*oy[0] + oy[1]*oy[1] + oy[2]*oy[2]);

  // Point the camera orthogonal toward the plane.
  cam->SetPosition(p[0], p[1], p[2]);
  cam->SetFocalPoint(p[0]-vn[0], p[1]-vn[1], p[2]-vn[2]);
  cam->SetViewUp(oy[0], oy[1], oy[2]);
  
  // Compute view angle.
  viewAngle = asin(height/(2.0*dist)) * 360.0 / 3.1415926;
  cam->SetViewAngle(viewAngle);

  // Compute the shear/offset vector (focal point to window center).
  offset[0] = center[0] - (p[0]-dist*vn[0]);
  offset[1] = center[1] - (p[1]-dist*vn[1]);
  offset[2] = center[2] - (p[2]-dist*vn[2]);

  // Compute the normalized x and y components of shear offset.
  tmp = sqrt(ox[0]*ox[0] + ox[1]*ox[1] + ox[2]*ox[2]);
  xOffset = vtkMath::Dot(offset, ox) / (tmp * tmp); 
  tmp = sqrt(oy[0]*oy[0] + oy[1]*oy[1] + oy[2]*oy[2]);
  yOffset = vtkMath::Dot(offset, oy) / (tmp * tmp); 

  // Off angle positioning of window.
  cam->SetWindowCenter(2*xOffset, 2*yOffset);
}



//-------------------------------------------------------------------------
// Only called on "client".
void vtkMultiDisplayManager::ClientStartRender()
{
  vtkPVMultiDisplayInfo info;
  int numProcs;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  float updateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  vtkDebugMacro("StartRender");
  // Make sure they all swp buffers at the same time.
  this->RenderWindow->SwapBuffersOff();

  // All this just gets information to send to the satellites.  
  if (updateRate > 2.0)
    {
    this->ReductionFactor = 2;
    }
  else
    {
    this->ReductionFactor = 1;
    }
  rens = this->RenderWindow->GetRenderers();
  numProcs = this->Controller->GetNumberOfProcesses();
  info.UseCompositing = this->UseCompositing;
  if (this->RenderWindow->GetDesiredUpdateRate() > 2.0 &&
      this->UseCompositing)
    {
    info.ReductionFactor = this->LODReductionFactor;
    }
  else
    {  
    info.ReductionFactor = 1;
    }
  // Make sure the satellite renderers have the same camera I do.
  // Note: This will lockup unless every process has the same number
  // of renderers.
  rens->InitTraversal();
  // Assume only one renderer.
  ren = rens->GetNextItem();
  cam = ren->GetActiveCamera();
  lc = ren->GetLights();
  lc->InitTraversal();
  light = lc->GetNextItem();
  cam->GetPosition(info.CameraPosition);
  cam->GetFocalPoint(info.CameraFocalPoint);
  cam->GetViewUp(info.CameraViewUp);
  cam->GetClippingRange(info.CameraClippingRange);
  info.CameraViewAngle = cam->GetViewAngle();
  if (cam->GetParallelProjection())
    {
    info.ParallelScale = cam->GetParallelScale();
    }
  else
    {
    info.ParallelScale = 0.0;
    }
  if (light)
    {
    light->GetPosition(info.LightPosition);
    light->GetFocalPoint(info.LightFocalPoint);
    }
  ren->GetBackground(info.Background);


  // Trigger the satellite processes to start their render routine.  
  if (this->SocketController)
    { // client... Send to root
    this->SocketController->TriggerRMI(1, NULL, 0, 
                     vtkMultiDisplayManager::ROOT_RENDER_RMI_TAG);
    this->SocketController->Send((float*)(&info), 24, 1, 
                     vtkMultiDisplayManager::INFO_TAG);
    }
  else
    {
    // Client is also root.  Call directly.
    this->RootStartRender(info);   
    }
}

//-------------------------------------------------------------------------
// Only called on "root".
void vtkMultiDisplayManager::RootStartRender(vtkPVMultiDisplayInfo info)
{
  int id, numProcs;

  if (this->Controller)
    {
    numProcs = this->Controller->GetNumberOfProcesses();
    }
  else
    {
    numProcs = 1;
    }

  // Every process (except "client") gets to participate.  
  for (id = 1; id < numProcs; ++id)
    {
    this->Controller->TriggerRMI(id, NULL, 0, 
                     vtkMultiDisplayManager::SATELLITE_RENDER_RMI_TAG);
    this->Controller->Send((float*)(&info), 24, id,
                     vtkMultiDisplayManager::INFO_TAG);
    }
  if ( this->SocketController)
    { // Root is not client, it participates also.
    this->SatelliteStartRender(info);
    }
}



//-------------------------------------------------------------------------
void vtkMultiDisplayManager::SatelliteStartRender(vtkPVMultiDisplayInfo info)
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam = 0;
  vtkLightCollection *lc;
  vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;

  // Delay swapping buffers untill all processes are finished.
  if (this->Controller)
    {
    renWin->SwapBuffersOff();  
    }

  // Synchronize
  //renWin->SetDesiredUpdateRate(info.DesiredUpdateRate);
  this->ReductionFactor = static_cast<int>(info.ReductionFactor);
  this->UseCompositing = static_cast<int>(info.UseCompositing);
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  // NOTE:  We are now receiving first!!!!!  
  // This will probably cause a bug based on the folloing comment
  // about getting the active camera.
  // "We put this before receive because we want the pipeline to be
  // updated the first time if the camera does not exist and we want
  // it to happen before we block in receive"
  ren = rens->GetNextItem();
  if (ren == NULL)
    {
    vtkErrorMacro("Renderer mismatch.");
    }
  else
    {
    lc = ren->GetLights();
    lc->InitTraversal();
    light = lc->GetNextItem();
    // Setup tile independent stuff
    cam = ren->GetActiveCamera();
    cam->SetViewAngle(asin(sin(info.CameraViewAngle*3.1415926/360.0)/(double)(this->TileDimensions[0])) * 360.0 / 3.1415926);
    cam->SetPosition(info.CameraPosition);
    cam->SetFocalPoint(info.CameraFocalPoint);
    cam->SetViewUp(info.CameraViewUp);
    cam->SetClippingRange(info.CameraClippingRange);
    if (info.ParallelScale != 0.0)
      {
      cam->ParallelProjectionOn();
      cam->SetParallelScale(info.ParallelScale/(double)(this->TileDimensions[0]));
      }
    else
      {
      cam->ParallelProjectionOff();   
      }
    if (light)
      {
      light->SetPosition(info.LightPosition);
      light->SetFocalPoint(info.LightFocalPoint);
      }
    ren->SetBackground(info.Background);
    ren->SetViewport(0, 0, 1.0/(float)this->ReductionFactor, 
                     1.0/(float)this->ReductionFactor);
    }

  // Renders and composites
  this->Composite();

  // Force swap buffers here.
  if (this->SocketController)
    {
    //this->SocketController->Barrier();
    // Socket barrier is not implemented.
    // Just send a message to synchronize.
    int dummyMessage = 10;
    this->SocketController->Send(&dummyMessage,1, 1, 12323);
    }
  if (this->Controller)
    {
    this->Controller->Barrier();
    }
  renWin->SwapBuffersOn();  
  renWin->Frame();
}




//----------------------------------------------------------------------------
// Use the schedule to do the compositing.
// Only Called on the satellites.
void vtkMultiDisplayManager::Composite()
{
  static int firstRender = 1;
  int myId = this->Controller->GetLocalProcessId() - this->ZeroEmpty;
  int numberOfCompositeSteps = this->Schedule->GetNumberOfProcessElements(myId);
  int i, x, y, idx;
  int tileId = this->Schedule->GetProcessTileId(myId);
  vtkFloatArray*        zData;
  vtkUnsignedCharArray* pData;
  vtkPVCompositeBuffer* buf;
  vtkPVCompositeBuffer* buf2;
  vtkPVCompositeBuffer* buf3;
  int length;
  int size[2];
  int *rws;
  vtkPVCompositeBuffer** tileBuffers;
  vtkCamera* cam;
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  int  numberOfTiles = this->TileDimensions[0] * this->TileDimensions[1];

  rens = renWin->GetRenderers();
  rens->InitTraversal();
  ren = rens->GetNextItem();
  if (ren)
    {
    cam = ren->GetActiveCamera();
    }

  // If this flag is set by the root, then skip compositing.
  if ( ! this->UseCompositing || numberOfCompositeSteps == 0)
    { // Just set up this one tile and render.
    // Figure out the tile indexes.
    // ZeroEmpty causes the -1?
    i = this->Controller->GetLocalProcessId() - this->ZeroEmpty;
    y = i/this->TileDimensions[0];
    x = i - y*this->TileDimensions[0];
    cam->SetWindowCenter(1.0-(double)(this->TileDimensions[0]) + 2.0*(double)x,
                         1.0-(double)(this->TileDimensions[1]) + 2.0*(double)y);
    // Ignore reduction (only necessary for one tile).
    ren->SetViewport(0, 0, 1.0, 1.0);
    renWin->Render();
    return;
    }

  int front = 0;

  // size is not valid until after the first render.
  if (firstRender)
    {
    renWin->Render();
    firstRender = 0;
    }
  
  rws = this->RenderWindow->GetSize();
  size[0] = (int)((float)rws[0] / (float)(this->ReductionFactor));
  size[1] = (int)((float)rws[1] / (float)(this->ReductionFactor));  

  // We allocated with special mpiPro new so we do not need to copy.
#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(0);
#endif

  // Allocate an array of buffers for the tiles (not all will be used.)
  tileBuffers = new vtkPVCompositeBuffer* [numberOfTiles];
  for (idx = 0; idx < numberOfTiles; ++idx)
    {
    tileBuffers[idx] = NULL;
    }  

  // Sanity check
  // Since we handle the first "numberOfTiles" steps separately.
  if (numberOfCompositeSteps < numberOfTiles)
    {
    vtkErrorMacro("Too few composites for algorithm.");
    }

  // Intermix the first n (numTiles) compositing steps with rendering.
  // Each of these stages is dedicated to one (corresponding) tile.
  // Since half of these processes immediately send the buffer,
  // It would be a waste to store the buffer and not reuse it.
  // All this rendering will be done in the back buffer without any swaps.
  for (idx = 0; idx < numberOfTiles; ++idx)
    {
    // Figure out the tile indexes.
    y = idx/this->TileDimensions[0];
    x = idx - y*this->TileDimensions[0];
    // Setup the camera for this tile.
    cam->SetWindowCenter(1.0-(double)(this->TileDimensions[0]) + 2.0*(double)x,
                         1.0-(double)(this->TileDimensions[1]) + 2.0*(double)y);
    renWin->Render();

    // Get the color buffer (RGB).
    pData = this->CompositeUtilities->NewUnsignedCharArray(size[0]*size[1], 3);
    this->RenderWindow->GetPixelData(
             0,0,size[0]-1, size[1]-1, 
             front,pData);
    // Get the z buffer.
    zData = this->CompositeUtilities->NewFloatArray(size[0]*size[1], 1);
    this->RenderWindow->GetZbufferData(0,0, size[0]-1, size[1]-1,
                                       zData);  
    // Compress the buffer.
    length = vtkPVCompositeUtilities::GetCompressedLength(zData);
    buf = this->CompositeUtilities->NewCompositeBuffer(length);
    vtkPVCompositeUtilities::Compress(zData, pData, buf);

    // Overhead of deleting these and getting them is low.
    // Doing so may decrease total buffer count.
    pData->Delete();
    pData = NULL;
    zData->Delete();
    zData = NULL;
  
    // Sanity check that the schedule put the first N tiles as steps.
    // We make this assumption by storing the tiles in tileBuffers
    if (this->Schedule->GetElementTileId(myId,idx) != idx)
      {
      vtkErrorMacro("Wrong tile rendered!");
      }

    if ( ! this->Schedule->GetElementReceiveFlag(myId, idx) )
      {
      // Send and recycle the buffer.
      vtkPVCompositeUtilities::SendBuffer(this->Controller, buf,
                                  this->Schedule->GetElementOtherProcessId(myId, idx)+this->ZeroEmpty, 
                                  98);
      buf->Delete();
      buf = NULL;          
      }
    else
      {
      // Receive a buffer.
      buf2 = this->CompositeUtilities->ReceiveNewBuffer(this->Controller, 
               this->Schedule->GetElementOtherProcessId(myId, idx)+this->ZeroEmpty, 
               98);
      // This value is currently a conservative estimate.
      length = vtkPVCompositeUtilities::GetCompositedLength(buf, buf2);
      buf3 = this->CompositeUtilities->NewCompositeBuffer(length);
      vtkPVCompositeUtilities::CompositeImagePair(buf, buf2, buf3);
      tileBuffers[idx] = buf3;
      buf3 = NULL;
      buf->Delete();
      buf = NULL;
      buf2->Delete();
      buf2 = NULL;
      }
    }
  
  // Do the rest of the compositing steps.
  for (i = numberOfTiles; i < numberOfCompositeSteps; i++) 
    {
    if ( ! this->Schedule->GetElementReceiveFlag(myId, i))
      {
      // Send and recycle the buffer.
      buf = tileBuffers[this->Schedule->GetElementTileId(myId, i)];
      tileBuffers[this->Schedule->GetElementTileId(myId, i)] = NULL;
      vtkPVCompositeUtilities::SendBuffer(this->Controller, buf, 
        this->Schedule->GetElementOtherProcessId(myId, i)+this->ZeroEmpty, 
        99);
      buf->Delete();          
      buf = NULL;
      }
    else
      {
      buf = tileBuffers[this->Schedule->GetElementTileId(myId, i)];
      tileBuffers[this->Schedule->GetElementTileId(myId, i)] = NULL;
      // Receive a buffer.
      buf2 = this->CompositeUtilities->ReceiveNewBuffer(this->Controller, 
               this->Schedule->GetElementOtherProcessId(myId, idx)+this->ZeroEmpty, 
               99);
      // Length is a conservative estimate.
      length = vtkPVCompositeUtilities::GetCompositedLength(buf, buf2);
      buf3 = this->CompositeUtilities->NewCompositeBuffer(length);
      vtkPVCompositeUtilities::CompositeImagePair(buf, buf2, buf3);
      tileBuffers[this->Schedule->GetElementTileId(myId, i)] = buf3;
      buf3 = NULL;
      buf->Delete();
      buf2->Delete();
      }
    }

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  if (tileId >= 0)
    { // Local process has a tile to display.
    buf = tileBuffers[this->Schedule->GetProcessTileId(myId)];
    tileBuffers[this->Schedule->GetProcessTileId(myId)] = NULL;

    // Recreate a buffer to hold the color data.
    pData = this->CompositeUtilities->NewUnsignedCharArray(size[0]*size[1], 3);

    // Now we want to decompress into the original buffers.
    // Ignore z because it is not used by composite manager.
    vtkPVCompositeUtilities::Uncompress(buf, pData);
    buf->Delete();
    buf = NULL;

    if (this->ReductionFactor > 1)
      {
      vtkUnsignedCharArray* pData2;
      pData2 = pData;
      pData = this->CompositeUtilities->NewUnsignedCharArray(rws[0]*rws[1], 3);

      vtkTimerLog::MarkStartEvent("Magnify Buffer");
      vtkPVCompositeUtilities::MagnifyBuffer(pData2, pData, size, 
                                             this->ReductionFactor);
      vtkTimerLog::MarkEndEvent("Magnify Buffer");
      pData2->Delete();
      pData2 = NULL;

      // I do not know if this is necessary !!!!!!!
      vtkRenderer* renderer =
          ((vtkRenderer*)
          this->RenderWindow->GetRenderers()->GetItemAsObject(0));
      renderer->SetViewport(0, 0, 1.0, 1.0);
      renderer->GetActiveCamera()->UpdateViewport(renderer);
      }

    this->RenderWindow->SetPixelData(0, 0, 
                                     rws[0]-1, 
                                     rws[1]-1, 
                                     pData, 0);
    pData->Delete();
    pData = NULL;
    }
  
  // They should all already be gone, but ...
  for (idx = 0; idx < numberOfTiles; ++idx)
    {
    if (tileBuffers[idx])
      {
      vtkErrorMacro("Expecting all buffers to be deleted all ready.");
      tileBuffers[idx]->Delete();
      tileBuffers[idx] = NULL;
      }
    }
  delete [] tileBuffers;
}




//-------------------------------------------------------------------------
void vtkMultiDisplayManager::InitializeSchedule()
{
  // In clinet server mode, the client does not have a schedule.
  if ( ! this->ClientFlag)
    {
    int  numberOfTiles = this->TileDimensions[0] * this->TileDimensions[1];
    this->Schedule->InitializeTiles(numberOfTiles, 
                                    this->NumberOfProcesses-this->ZeroEmpty);
    }
}




//-------------------------------------------------------------------------
// Only client needs start and end render callbacks.
void vtkMultiDisplayManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  int clientFlag = 0;

  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->ClientFlag)
    {
    clientFlag = 1;
    }
  if (this->ZeroEmpty && this->Controller && 
      this->Controller->GetLocalProcessId() == 0)
    {
    clientFlag = 1;
    }

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (clientFlag)
      {
      this->RenderWindow->RemoveObserver(this->StartTag);
      this->RenderWindow->RemoveObserver(this->EndTag);
      }
    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    if (clientFlag)
      {
      vtkCallbackCommand *cbc;
      
      cbc= vtkCallbackCommand::New();
      cbc->SetCallback(vtkMultiDisplayManagerClientStartRender);
      cbc->SetClientData((void*)this);
      // renWin will delete the cbc when the observer is removed.
      this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
      cbc->Delete();
        
      cbc = vtkCallbackCommand::New();
      cbc->SetCallback(vtkMultiDisplayManagerClientEndRender);
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


//-------------------------------------------------------------------------
void vtkMultiDisplayManager::SetController(vtkMultiProcessController *mpc)
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
void vtkMultiDisplayManager::SetSocketController(vtkSocketController *mpc)
{
  if (this->SocketController == mpc)
    {
    return;
    }
  if (mpc)
    {
    mpc->Register(this);
    }
  if (this->SocketController)
    {
    this->SocketController->UnRegister(this);
    }
  this->SocketController = mpc;
}



//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkMultiDisplayManager::InitializeRMIs()
{
  // Adding RMIs to processes that do not need them is harmless ...
  if (this->SocketController)
    {
    this->SocketController->AddRMI(vtkMultiDisplayManagerRootStartRender, (void*)this, 
                                   vtkMultiDisplayManager::ROOT_RENDER_RMI_TAG); 
    }
  if (this->Controller)
    {
    this->Controller->AddRMI(vtkMultiDisplayManagerSatelliteStartRender, (void*)this, 
                             vtkMultiDisplayManager::SATELLITE_RENDER_RMI_TAG); 
    }
}





//-------------------------------------------------------------------------
void vtkMultiDisplayManager::ClientEndRender()
{
  vtkRenderWindow* renWin = this->RenderWindow;
  
  // Force swap buffers here.
  if (this->ZeroEmpty)
    {
    if (this->Controller)
      {
      this->Controller->Barrier();
      }
    }
  else
    {
    if (this->SocketController)
      {
      this->SocketController->Barrier();
      // Since socket barrier is not implemented,
      // just receive a message to synchronize.
      int dummyMessage;
      this->SocketController->Receive(&dummyMessage,1, 1, 12323);
      }
    }

  if (renWin)
    {
    renWin->SwapBuffersOn();  
    renWin->Frame();
    }
}


//----------------------------------------------------------------------------
void vtkMultiDisplayManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "ClientFlag: " << this->ClientFlag << endl;

  if ( this->RenderWindow )
    {
    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    }
  else
    {
    os << indent << "RenderWindow: (none)\n";
    }
  os << indent << "UseCompositing: " << this->UseCompositing << "\n";
  os << indent << "LODReductionFactor: " << this->LODReductionFactor << "\n";
  
  os << indent << "Tile Dimensions: " << this->TileDimensions[0] << ", "
     << this->TileDimensions[1] << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;

  os << indent << "Controller: (" << this->Controller << ")\n"; 
  os << indent << "SocketController: (" << this->SocketController << ")\n"; 

  if (this->Schedule)
    {
    this->Schedule->PrintSelf(os, indent);
    }

  os << indent << "CompositeUtilities: \n";
  vtkIndent i2 = indent.GetNextIndent();
  this->CompositeUtilities->PrintSelf(os, i2);
}



