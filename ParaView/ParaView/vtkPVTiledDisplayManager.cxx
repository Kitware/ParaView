/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTiledDisplayManager.cxx
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

#include "vtkPVTiledDisplayManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkTimerLog.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
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

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkPVTiledDisplayManager, "1.10.4.1");
vtkStandardNewMacro(vtkPVTiledDisplayManager);

vtkCxxSetObjectMacro(vtkPVTiledDisplayManager, RenderView, vtkObject);



// Here is a structure to help create and store a compositing schedule.

// Elemnent representing communication step of compositing.
class vtkPVTiledDisplayElement 
{
public:
  int TileId;
  int CompositeLevel;
  int VoidFlag;
  int ReceiveFlag;
  int OtherCompositeId;
  int OtherProcessId;
};

class vtkPVTiledDisplayProcess 
{
public:
  vtkPVTiledDisplayProcess();
  ~vtkPVTiledDisplayProcess();
  int TileId; // not necessary
  int CompositeId;
  int Length;
  vtkPVTiledDisplayElement** Elements;
};

class vtkPVTiledDisplaySchedule 
{
public:
  vtkPVTiledDisplaySchedule();
  ~vtkPVTiledDisplaySchedule();
  void Print(ostream& os, vtkIndent indent);

  void InitializeForTile(int tileId, int tileProcess, int numProcs, int zeroEmtpy);
  // Swaps processes if benefits global totals.
  // Also recomputes global totals.
  int SwapIfApproporiate(int pid1, int pid2,
                         int* totalProcessLengths);
  void ComputeElementOtherProcessIds();

  int NumberOfProcesses; // Same as global number of processe.
  vtkPVTiledDisplayProcess** Processes;
};

//-------------------------------------------------------------------------
vtkPVTiledDisplayProcess::vtkPVTiledDisplayProcess()
{
  this->TileId = -1;
  this->CompositeId = -1;
  this->Length = 0;
  this->Elements = NULL;
}

//-------------------------------------------------------------------------
vtkPVTiledDisplayProcess::~vtkPVTiledDisplayProcess()
{
  int idx;

  for (idx = 0; idx < this->Length; ++idx)
    {
    if (this->Elements && this->Elements[idx])
      {
      delete this->Elements[idx];
      this->Elements[idx] = NULL;
      }
    }
  this->Length = 0;
  if (this->Elements)
    {
    delete [] this->Elements;
    this->Elements = NULL;
    }
}

//-------------------------------------------------------------------------
vtkPVTiledDisplaySchedule::vtkPVTiledDisplaySchedule()
{
  this->NumberOfProcesses = 0;
  this->Processes = NULL;
}

//-------------------------------------------------------------------------
vtkPVTiledDisplaySchedule::~vtkPVTiledDisplaySchedule()
{
  int idx;

  for (idx = 0; idx < this->NumberOfProcesses; ++idx)
    {
    if (this->Processes && this->Processes[idx])
      {
      delete this->Processes[idx];
      this->Processes[idx] = NULL;
      }
    }
  this->NumberOfProcesses = 0;
  if (this->Processes)
    {
    delete [] this->Processes;
    this->Processes = NULL;
    }
}


//-------------------------------------------------------------------------
// Only worry about power of 2 processes for now.
// Set up the scedule for a single tile.  Final results
// endup on process "tileProcess".
void vtkPVTiledDisplaySchedule::InitializeForTile(int tileId, 
                                                int tileProcess, 
                                                int numProcs,
                                                int zeroEmpty)
{
  int pIdx, pIdxSend;
  vtkPVTiledDisplayProcess* p;
  int eIdx;
  vtkPVTiledDisplayElement* e;
  int maxLevels;
  int level;


  // I could just set this to be NumberOfProcesses, but that would be wasteful.
  maxLevels = (int)(ceil(log((double)(numProcs))/log(2.0)));

  this->NumberOfProcesses = numProcs;
  this->Processes = new vtkPVTiledDisplayProcess*[numProcs];

  // Just allocate all of the schedule process objects.
  for (pIdx = 0; pIdx < numProcs; ++ pIdx)
    {
    p = new vtkPVTiledDisplayProcess;
    this->Processes[pIdx] = p;
    // Initialize process
    p->Elements = new vtkPVTiledDisplayElement*[maxLevels];
    for (eIdx = 0; eIdx < maxLevels; ++eIdx)
      {
      p->Elements[eIdx] = NULL;
      }
    p->Length = 0; // Actual number not max.
    p->TileId = tileId;
    p->CompositeId = pIdx; // Initially, composite id and process id are same.
    }

  if (zeroEmpty)
    {
    numProcs = numProcs - 1;
    }

  // Loop over tree levels.
  // only works for power of two for now.
  level = 0;
  while (numProcs > 1)
    {
    // Loop over and create send and receive elements
    numProcs = numProcs >> 1;
    for (pIdx = 0; pIdx < numProcs; ++ pIdx)
      {
      pIdxSend = pIdx+numProcs;
      // Receiving process    
      p = this->Processes[pIdx+zeroEmpty];
      e = new vtkPVTiledDisplayElement;
      p->Elements[p->Length] = e;
      e->ReceiveFlag = 1;
      e->VoidFlag = 0;
      e->TileId = tileId;
      e->OtherCompositeId = pIdxSend+zeroEmpty;
      e->OtherProcessId = -1;
      e->CompositeLevel = level;
      ++(p->Length);
      if (p->Length > maxLevels)
        { // Sanity check
        vtkGenericWarningMacro("Too many levels.");
        }
      // Sending process
      p = this->Processes[pIdxSend+zeroEmpty];
      e = new vtkPVTiledDisplayElement;
      p->Elements[p->Length] = e;
      e->ReceiveFlag = 0;
      e->VoidFlag = 0;
      e->TileId = tileId;
      e->OtherCompositeId = pIdx+zeroEmpty;
      e->OtherProcessId = -1;
      e->CompositeLevel = level;
      ++(p->Length);
      if (p->Length > maxLevels)
        { // Sanity check
        vtkGenericWarningMacro("Too many levels.");
        }
      }
    ++level;
    }

  // Swap processes so that the end result ends up on the tileProcess.
  if (tileProcess != 0)
    {
    p = this->Processes[zeroEmpty];
    this->Processes[zeroEmpty] = this->Processes[tileProcess+zeroEmpty];
    this->Processes[tileProcess+zeroEmpty] = p;
    }
}

//-------------------------------------------------------------------------
int vtkPVTiledDisplaySchedule::SwapIfApproporiate(int pid1, int pid2,
                                                int* totalProcessLengths)
{
  vtkPVTiledDisplayProcess* p1;
  vtkPVTiledDisplayProcess* p2;
  int max;
  int t1, t2;

  // Process 0 is the user interface (zeroEmpty).
  if (pid1 == 0 || pid2 == 0)
    {
    return 0;
    }

  p1 = this->Processes[pid1];
  p2 = this->Processes[pid2];

  // Cannot move processes with composite id 0 because final image needs
  // to end up on the assigned process.
  // Tile composite Id is actually 1 because of zeroEmpty condition (hardcoded).
  if (p1->CompositeId == 1 || p2->CompositeId == 1)
    {
    return 0;
    }

  t1 = totalProcessLengths[pid1];
  t2 = totalProcessLengths[pid2];
  max = t1;
  if (max < t2)
    {
    max = t2;
    }

  // Compute the total lengths if swap occurs.
  t1 = t1 - p1->Length + p2->Length;
  t2 = t2 - p2->Length + p1->Length;

  // Is the swap beneficial?
  if (t1 >= max || t2 >= max)
    { // No
    return 0;
    }

  // Perform the swap.
  this->Processes[pid1] = p2;
  this->Processes[pid2] = p1;
  totalProcessLengths[pid1] = t1;
  totalProcessLengths[pid2] = t2;

  return 1;
}

//-------------------------------------------------------------------------
void vtkPVTiledDisplaySchedule::ComputeElementOtherProcessIds()
{
  int pIdx, cIdx, eIdx;
  int* map = new int[this->NumberOfProcesses];
  vtkPVTiledDisplayProcess* p;
  vtkPVTiledDisplayElement* e;

  // Initialize the map just in case something went wrong.
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    map[pIdx] = -1;
    }
  // Now create the reverse map.
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    cIdx = this->Processes[pIdx]->CompositeId;
    map[cIdx] = pIdx;
    }

  // Loop through all elements doing the conversion.
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    p = this->Processes[pIdx];
    for (eIdx = 0; eIdx < p->Length; ++eIdx)
      {
      e = p->Elements[eIdx];
      e->OtherProcessId = map[e->OtherCompositeId];
      }
    }

  delete [] map;
}

//-------------------------------------------------------------------------
void vtkPVTiledDisplaySchedule::Print(ostream& os, vtkIndent indent)
{
  int pIdx, eIdx;
  vtkPVTiledDisplayProcess* p;
  vtkPVTiledDisplayElement* e;
  vtkIndent i2 = indent.GetNextIndent();

  os << indent << "Schedule: (" << this << ")\n";

  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    p = this->Processes[pIdx];
    os << i2 << "Process:";

    for (eIdx = 0; eIdx < p->Length; ++eIdx)
      {
      e = p->Elements[eIdx];
      if (e)
        {
        os << " " << e->TileId;
        if (e->ReceiveFlag)
          { 
          os << "R";
          }
        else
          {
          os << "S";
          }
        os  << e->OtherProcessId << ",";
        }
      else
        {
        os << " null,";
        }
      }
    os << endl;
    }
}


//============== end of special objects =================


//-------------------------------------------------------------------------
void vtkPVTiledDisplayManager::InitializeSchedule()
{
  int  tIdx, pIdx;
  int sum, max;
  int  numberOfTiles = this->TileDimensions[0] * this->TileDimensions[1];
  int* totalProcessLengths;
  vtkPVTiledDisplaySchedule* ts;
  vtkPVTiledDisplayProcess* p;
  int i, j;

  // Create a schedule for each tile.
  vtkPVTiledDisplaySchedule** tileSchedules;
  tileSchedules = new vtkPVTiledDisplaySchedule* [numberOfTiles];
  for (tIdx = 0; tIdx < numberOfTiles; ++tIdx)
    {
    ts = new vtkPVTiledDisplaySchedule;
    tileSchedules[tIdx] = ts;
    // This assumes that the first n processes are displaying tiles.
    ts->InitializeForTile(tIdx, tIdx, this->NumberOfProcesses, this->ZeroEmpty);
    }

  // Create an array which store the total number of levels
  // for each processes.
  totalProcessLengths = new int[this->NumberOfProcesses];
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    sum = 0;
    for (tIdx = 0; tIdx < numberOfTiles; ++tIdx)
      {
      sum += tileSchedules[tIdx]->Processes[pIdx]->Length;
      }
    totalProcessLengths[pIdx] = sum;
    }

  // Now swap to get maximum length down to a minimum.
  int changeFlag = 1;
  while (changeFlag)
    {
    changeFlag = 0;
    for (tIdx = 0; tIdx < numberOfTiles; ++tIdx)
      {
      ts = tileSchedules[tIdx];
      for (i = 0; i < ts->NumberOfProcesses; ++i)
        {
        for (j = i+1; j < ts->NumberOfProcesses; ++j)
          {
          if (ts->SwapIfApproporiate(i, j, totalProcessLengths))
            {
            changeFlag = 1;
            }
          }
        }
      }
    }

  // Fill in the other process Ids of all the elements.
  // Convert the otherCompositeId to otherProcessId.
  for (tIdx = 0; tIdx < numberOfTiles; ++tIdx)
    {
    ts = tileSchedules[tIdx];
    ts->ComputeElementOtherProcessIds();
    }

  // Now shuffle tile schedules into one jumbo schedule.
  max = 1;
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    if (totalProcessLengths[pIdx] > max)
      {
      max = totalProcessLengths[pIdx];
      }
    }  
  if (this->Schedule)
    { // Should not happen (init called only once) ...
    delete this->Schedule;
    this->Schedule = NULL;
    }
  // Create the schedule and processes 
  // (Elements added later by shuffle).
  this->Schedule = new vtkPVTiledDisplaySchedule;
  this->Schedule->NumberOfProcesses = this->NumberOfProcesses;
  this->Schedule->Processes = 
           new vtkPVTiledDisplayProcess*[this->NumberOfProcesses];
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    p = new vtkPVTiledDisplayProcess;
    this->Schedule->Processes[pIdx] = p;
    // Hard coded zeroEmpty
    p->TileId = -1;
    if (pIdx > 0 && pIdx <= numberOfTiles)
      {
      p->TileId = pIdx - 1;
      }
    p->CompositeId = -1;  // Not used here.
    // Length is the actual number of elements in the process
    // not the length of the array.  No array bounds checking.
    p->Length = 0;
    p->Elements = new vtkPVTiledDisplayElement*[max];
    for (i = 0; i < max; ++i)
      {
      p->Elements[i] = NULL;
      }
    }

  // Ok, so now how do we add/shuffle elements?
  int flag = 1;
  i = 0;
  while (flag)
    {
    flag = this->ShuffleLevel(i, numberOfTiles, tileSchedules);
    ++i;
    }

  // Delete the tile schedules.
  for (tIdx = 0; tIdx < numberOfTiles; ++tIdx)
    {
    ts = tileSchedules[tIdx];
    delete ts;
    tileSchedules[tIdx] = NULL;
    }
  delete [] tileSchedules;
}

//-------------------------------------------------------------------------
// I am just going to try brute force adding one level at a time.
// Returns 0 when all leves are finished.
int vtkPVTiledDisplayManager::ShuffleLevel(int level, int numTiles, 
                                    vtkPVTiledDisplaySchedule** tileSchedules)
{
  int flag = 0;
  int tIdx, pIdx, eIdx;
  vtkPVTiledDisplaySchedule* ts;
  vtkPVTiledDisplayProcess* p;
  vtkPVTiledDisplayElement* e;
  vtkPVTiledDisplayProcess* p2;

  for (tIdx = 0; tIdx < numTiles; ++tIdx)
    {
    ts = tileSchedules[tIdx];
    for (pIdx = 0; pIdx < ts->NumberOfProcesses; ++pIdx)
      {
      p = ts->Processes[pIdx];
      for (eIdx = 0; eIdx < p->Length; ++eIdx)
        {
        e = p->Elements[eIdx];
        if (e && e->CompositeLevel <= level)
          {
          flag = 1;
          p->Elements[eIdx] = NULL;
          p2 = this->Schedule->Processes[pIdx];
          p2->Elements[p2->Length] = e;
          ++(p2->Length);
          }
        }
      }
    }

  return flag;
}


//==========================================================================
// End of schedule object stuff.
//==========================================================================


// Structures to communicate render info.
struct vtkPVTiledDisplayRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
  int ImageReductionFactor;
  int UseCompositing;
};

struct vtkPVTiledDisplayRendererInfo 
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
vtkPVTiledDisplayManager::vtkPVTiledDisplayManager()
{
  this->ImageReductionFactor = 1;
  this->LODReductionFactor = 4;

  this->RenderWindow = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();
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

}

  
//-------------------------------------------------------------------------
vtkPVTiledDisplayManager::~vtkPVTiledDisplayManager()
{
  this->SetRenderWindow(NULL);
  
  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }

  if (this->Schedule)
    {
    delete this->Schedule;
    }
  this->CompositeUtilities->Delete();
  this->CompositeUtilities = NULL;
}

//-------------------------------------------------------------------------
// We may want to pass the render window as an argument for a sanity check.
void vtkPVTiledDisplayManagerStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkPVTiledDisplayManager *self = (vtkPVTiledDisplayManager *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->StartRender();
}

//-------------------------------------------------------------------------
void vtkPVTiledDisplayManagerEndRender(vtkObject *caller,
                                  unsigned long vtkNotUsed(event), 
                                  void *clientData, void *)
{
  vtkPVTiledDisplayManager *self = (vtkPVTiledDisplayManager *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->EndRender();
}

//----------------------------------------------------------------------------
void vtkPVTiledDisplayManagerRenderRMI(void *arg, void *, int, int)
{
  vtkPVTiledDisplayManager* self = (vtkPVTiledDisplayManager*) arg;
  
  self->RenderRMI();
}


//-------------------------------------------------------------------------
// Only process 0 needs start and end render callbacks.
void vtkPVTiledDisplayManager::SetRenderWindow(vtkRenderWindow *renWin)
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
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    if (this->Controller)
      {
      if (this->Controller && this->Controller->GetLocalProcessId() == 0)
        {
        vtkCallbackCommand *cbc;
        
        cbc= vtkCallbackCommand::New();
        cbc->SetCallback(vtkPVTiledDisplayManagerStartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(vtkPVTiledDisplayManagerEndRender);
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
void vtkPVTiledDisplayManager::SetController(vtkMultiProcessController *mpc)
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


//----------------------------------------------------------------------------
void vtkPVTiledDisplayManager::RenderRMI()
{
  // Start and end methods take care of synchronization and compositing
  vtkRenderWindow* renWin = this->RenderWindow;

  // Delay swapping buffers untill all processes are finished.
  if (this->Controller)
    {
    renWin->SwapBuffersOff();  
    }

  // Synchronizes
  this->SatelliteStartRender();
  // Renders and composites
  this->Composite();

  // Force swap buffers here.
  if (this->Controller)
    {
    this->Controller->Barrier();
    renWin->SwapBuffersOn();  
    renWin->Frame();
    }
}


//----------------------------------------------------------------------------
// Use the schedule to do the compositing.
void vtkPVTiledDisplayManager::Composite()
{
  static int firstRender = 1;
  int myId = this->Controller->GetLocalProcessId();
  vtkPVTiledDisplayProcess *tdp;
  vtkPVTiledDisplayElement *tde;
  int i, x, y, idx;
  int tileId = this->Schedule->Processes[myId]->TileId;
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
  if ( ! this->UseCompositing)
    { // Just set up this one tile and render.
    // Figure out the tile indexes.
    i = this->Controller->GetLocalProcessId() - 1;
    y = i/this->TileDimensions[0];
    x = i - y*this->TileDimensions[0];
    cam->SetWindowCenter(1.0-(double)(this->TileDimensions[0]) + 2.0*(double)x,
                         1.0-(double)(this->TileDimensions[1]) + 2.0*(double)y);
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
  size[0] = (int)((float)rws[0] / (float)(this->ImageReductionFactor));
  size[1] = (int)((float)rws[1] / (float)(this->ImageReductionFactor));  

  tdp = this->Schedule->Processes[myId];

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

    // One stage of compositing.
    tde = tdp->Elements[idx];
    // make sure the correct tile is being composited.
    if (tde && tde->TileId != idx)
      {
      vtkErrorMacro("Wrong tile rendered!");
      }
    if ( ! tde )
      { // This only happens when numTiles == 1.
      tileBuffers[idx] = buf;
      }
    else if ( ! tde->ReceiveFlag)
      {
      // Send and recycle the buffer.
      vtkPVCompositeUtilities::SendBuffer(this->Controller, buf,
                                          tde->OtherProcessId, 98);
      buf->Delete();
      buf = NULL;          
      }
    else
      {
      // Receive a buffer.
      buf2 = this->CompositeUtilities->ReceiveNewBuffer(this->Controller, 
                                                 tde->OtherProcessId, 98);
      // This value is currently a conservative estimate.
      length = vtkPVCompositeUtilities::GetCompositedLength(buf, buf2);
      buf3 = this->CompositeUtilities->NewCompositeBuffer(length);
      // Buf1 was allocated as full size.
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
  for (i = numberOfTiles; i < tdp->Length; i++) 
    {
    tde = tdp->Elements[i];
    if ( ! tde->ReceiveFlag)
      {
      // Send and recycle the buffer.
      buf = tileBuffers[tde->TileId];
      tileBuffers[tde->TileId] = NULL;
      vtkPVCompositeUtilities::SendBuffer(this->Controller, buf, 
                                          tde->OtherProcessId, 99);
      buf->Delete();          
      buf = NULL;
      }
    else
      {
      buf = tileBuffers[tde->TileId];
      tileBuffers[tde->TileId] = NULL;
      // Receive a buffer.
      buf2 = this->CompositeUtilities->ReceiveNewBuffer(this->Controller, 
                                                 tde->OtherProcessId, 99);
      // Length is a conservative estimate.
      length = vtkPVCompositeUtilities::GetCompositedLength(buf, buf2);
      buf3 = this->CompositeUtilities->NewCompositeBuffer(length);
      // Buf1 was allocated as full size.
      vtkPVCompositeUtilities::CompositeImagePair(buf, buf2, buf3);
      tileBuffers[tde->TileId] = buf3;
      buf3 = NULL;
      buf->Delete();
      buf2->Delete();
      }
    }

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  if (tileId >= 0)
    {
    buf = tileBuffers[tdp->TileId];
    tileBuffers[tdp->TileId] = NULL;

    // Recreate a buffer to hold the color data.
    pData = this->CompositeUtilities->NewUnsignedCharArray(size[0]*size[1], 3);

    // Now we want to decompress into the original buffers.
    // Ignore z because it is not used by composite manager.
    vtkPVCompositeUtilities::Uncompress(buf, pData);
    buf->Delete();
    buf = NULL;

    if (this->ImageReductionFactor > 1)
      {
      vtkUnsignedCharArray* pData2;
      pData2 = pData;
      pData = this->CompositeUtilities->NewUnsignedCharArray(rws[0]*rws[1], 3);

      vtkTimerLog::MarkStartEvent("Magnify Buffer");
      vtkPVCompositeUtilities::MagnifyBuffer(pData2, pData, size, 
                                             this->ImageReductionFactor);
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
void vtkPVTiledDisplayManager::SatelliteStartRender()
{
  vtkPVTiledDisplayRenderWindowInfo winInfo;
  vtkPVTiledDisplayRendererInfo renInfo;
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
  winInfo.ImageReductionFactor = 1;
  winInfo.UseCompositing = 1;

  // Receive the window size.
  controller->Receive((char*)(&winInfo), 
                      sizeof(struct vtkPVTiledDisplayRenderWindowInfo), 0, 
                      vtkPVTiledDisplayManager::WIN_INFO_TAG);
  //renWin->SetDesiredUpdateRate(winInfo.DesiredUpdateRate);
  this->ImageReductionFactor = winInfo.ImageReductionFactor;
  this->UseCompositing = winInfo.UseCompositing;

  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();

  // We put this before receive because we want the pipeline to be
  // updated the first time if the camera does not exist and we want
  // it to happen before we block in receive
  ren = rens->GetNextItem();
  if (ren)
    {
    cam = ren->GetActiveCamera();
    }

  controller->Receive((char*)(&renInfo), 
                      sizeof(struct vtkPVTiledDisplayRendererInfo), 
                      0, vtkPVTiledDisplayManager::REN_INFO_TAG);
  if (ren == NULL)
    {
    vtkErrorMacro("Renderer mismatch.");
    }
  else
    {
    lc = ren->GetLights();
    lc->InitTraversal();
    light = lc->GetNextItem();

    // Setup tile independant stuff
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
    ren->SetViewport(0, 0, 1.0/(float)this->ImageReductionFactor, 
                     1.0/(float)this->ImageReductionFactor);
    }
}

//-------------------------------------------------------------------------
void vtkPVTiledDisplayManager::SatelliteEndRender()
{  
  // Swap buffers.
}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkPVTiledDisplayManager::InitializeRMIs()
{
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  this->Controller->AddRMI(vtkPVTiledDisplayManagerRenderRMI, (void*)this, 
                           vtkPVTiledDisplayManager::RENDER_RMI_TAG); 

}





//-------------------------------------------------------------------------
// Only called in process 0.
void vtkPVTiledDisplayManager::StartRender()
{
  struct vtkPVTiledDisplayRenderWindowInfo winInfo;
  struct vtkPVTiledDisplayRendererInfo renInfo;
  int id, numProcs;
  int *size;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  float updateRate = this->RenderWindow->GetDesiredUpdateRate();
  
  vtkDebugMacro("StartRender");
  
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;

  if (controller == NULL)
    {
    return;
    }

  if (updateRate > 2.0)
    {
    this->ImageReductionFactor = 2;
    }
  else
    {
    this->ImageReductionFactor = 1;
    }
 

  // Make sure they all swp buffers at the same time.
  renWin->SwapBuffersOff();

  // Trigger the satellite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  numProcs = this->Controller->GetNumberOfProcesses();
  size = this->RenderWindow->GetSize();
  // This does nothing because tiles are full screen.
  winInfo.Size[0] = size[0]/this->ImageReductionFactor;
  winInfo.Size[1] = size[1]/this->ImageReductionFactor;
  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfo.UseCompositing = this->UseCompositing;
  // We should send the reduction factor !!!
  if (this->RenderWindow->GetDesiredUpdateRate() > 2.0 &&
      this->UseCompositing)
    {
    winInfo.ImageReductionFactor = this->LODReductionFactor;
    }
  else
    {
    winInfo.ImageReductionFactor = 1;
    }
  
  for (id = 1; id < numProcs; ++id)
    {
    controller->TriggerRMI(id, NULL, 0, 
                           vtkPVTiledDisplayManager::RENDER_RMI_TAG);

    // Synchronize the size of the windows.
    controller->Send((char*)(&winInfo), 
                     sizeof(vtkPVTiledDisplayRenderWindowInfo), id, 
                     vtkPVTiledDisplayManager::WIN_INFO_TAG);
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
                       sizeof(struct vtkPVTiledDisplayRendererInfo), id, 
                       vtkPVTiledDisplayManager::REN_INFO_TAG);
      }
    }
  
  // Turn swap buffers off before the render so the end render method
  // has a chance to add to the back buffer.
  //renWin->SwapBuffersOff();
}

//-------------------------------------------------------------------------
void vtkPVTiledDisplayManager::EndRender()
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
void vtkPVTiledDisplayManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
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

  if (this->Schedule)
    {
    this->Schedule->Print(os, indent);
    }

  os << indent << "CompositeUtilities: \n";
  vtkIndent i2 = indent.GetNextIndent();
  this->CompositeUtilities->PrintSelf(os, i2);
}



