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
#include "vtkFloatArray.h"
#include "vtkCompressCompositer.h"
#include "vtkCompositeManager.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#endif

#ifdef VTK_USE_MPI
 #include <mpi.h>
#endif

vtkCxxRevisionMacro(vtkTiledDisplayManager, "1.6");
vtkStandardNewMacro(vtkTiledDisplayManager);

vtkCxxSetObjectMacro(vtkTiledDisplayManager, RenderView, vtkObject);



// Here is a structure to help create and store a compositing schedule.

// Elemnent representing communication step of compositing.
class vtkTiledDisplayElement 
{
public:
  int TileId;
  int CompositeLevel;
  int VoidFlag;
  int ReceiveFlag;
  int OtherCompositeId;
  int OtherProcessId;
};

class vtkTiledDisplayProcess 
{
public:
  vtkTiledDisplayProcess();
  ~vtkTiledDisplayProcess();
  int TileId; // not necessary
  int CompositeId;
  int Length;
  vtkTiledDisplayElement** Elements;
};

class vtkTiledDisplaySchedule 
{
public:
  vtkTiledDisplaySchedule();
  ~vtkTiledDisplaySchedule();
  void PrintSelf(ostream& os, vtkIndent indent);

  void InitializeForTile(int tileId, int tileProcess, int numProcs, int zeroEmtpy);
  // Swaps processes if benefits global totals.
  // Also recomputes global totals.
  int SwapIfApproporiate(int pid1, int pid2,
                         int* totalProcessLengths);
  void ComputeElementOtherProcessIds();

  int NumberOfProcesses; // Same as global number of processe.
  vtkTiledDisplayProcess** Processes;
};

//-------------------------------------------------------------------------
vtkTiledDisplayProcess::vtkTiledDisplayProcess()
{
  this->TileId = -1;
  this->CompositeId = -1;
  this->Length = 0;
  this->Elements = NULL;
}

//-------------------------------------------------------------------------
vtkTiledDisplayProcess::~vtkTiledDisplayProcess()
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
vtkTiledDisplaySchedule::vtkTiledDisplaySchedule()
{
  this->NumberOfProcesses = 0;
  this->Processes = NULL;
}

//-------------------------------------------------------------------------
vtkTiledDisplaySchedule::~vtkTiledDisplaySchedule()
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
void vtkTiledDisplaySchedule::InitializeForTile(int tileId, 
                                                int tileProcess, 
                                                int numProcs,
                                                int zeroEmpty)
{
  int pIdx, pIdxSend;
  vtkTiledDisplayProcess* p;
  int eIdx;
  vtkTiledDisplayElement* e;
  int maxLevels;
  int level;


  // I could just set this to be NumberOfProcesses, but that would be wasteful.
  maxLevels = (int)(ceil(log((double)(numProcs))/log(2.0)));

  this->NumberOfProcesses = numProcs;
  this->Processes = new vtkTiledDisplayProcess*[numProcs];

  // Just allocate all of the schedule process objects.
  for (pIdx = 0; pIdx < numProcs; ++ pIdx)
    {
    p = new vtkTiledDisplayProcess;
    this->Processes[pIdx] = p;
    // Initialize process
    p->Elements = new vtkTiledDisplayElement*[maxLevels];
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
      e = new vtkTiledDisplayElement;
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
      e = new vtkTiledDisplayElement;
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
int vtkTiledDisplaySchedule::SwapIfApproporiate(int pid1, int pid2,
                                                int* totalProcessLengths)
{
  vtkTiledDisplayProcess* p1;
  vtkTiledDisplayProcess* p2;
  int max;
  int t1, t2;

  p1 = this->Processes[pid1];
  p2 = this->Processes[pid2];

  // Cannot move processes with composite id 0 because final image needs
  // to end up on the assigned process.
  if (p1->CompositeId == 0 || p2->CompositeId == 0)
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
void vtkTiledDisplaySchedule::ComputeElementOtherProcessIds()
{
  int pIdx, cIdx, eIdx;
  int* map = new int[this->NumberOfProcesses];
  vtkTiledDisplayProcess* p;
  vtkTiledDisplayElement* e;

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
void vtkTiledDisplaySchedule::PrintSelf(ostream& os, vtkIndent indent)
{
  int pIdx, eIdx;
  vtkTiledDisplayProcess* p;
  vtkTiledDisplayElement* e;
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
void vtkTiledDisplayManager::InitializeSchedule()
{
  int  tIdx, pIdx;
  int sum, max;
  int  numberOfTiles = this->TileDimensions[0] * this->TileDimensions[1];
  int* totalProcessLengths;
  vtkTiledDisplaySchedule* ts;
  vtkTiledDisplayProcess* p;
  int i, j;

  // Create a schedule for each tile.
  vtkTiledDisplaySchedule** tileSchedules;
  tileSchedules = new vtkTiledDisplaySchedule* [numberOfTiles];
  for (tIdx = 0; tIdx < numberOfTiles; ++tIdx)
    {
    ts = new vtkTiledDisplaySchedule;
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
  max = 0;
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
  // Create the schedule and processes (Elements added by shuffle).
  this->Schedule = new vtkTiledDisplaySchedule;
  this->Schedule->NumberOfProcesses = this->NumberOfProcesses;
  this->Schedule->Processes = 
           new vtkTiledDisplayProcess*[this->NumberOfProcesses];
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    p = new vtkTiledDisplayProcess;
    this->Schedule->Processes[pIdx] = p;
    p->CompositeId = -1;  // Not used here.
    // Length is the actual number of elements in the process
    // not the length of the array.  No array bounds checking.
    p->Length = 0;
    p->Elements = new vtkTiledDisplayElement*[max];
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
int vtkTiledDisplayManager::ShuffleLevel(int level, int numTiles, 
                                    vtkTiledDisplaySchedule** tileSchedules)
{
  int flag = 0;
  int tIdx, pIdx, eIdx;
  vtkTiledDisplaySchedule* ts;
  vtkTiledDisplayProcess* p;
  vtkTiledDisplayElement* e;
  vtkTiledDisplayProcess* p2;

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


//-------------------------------------------------------------------------
void vtkTiledDisplayManager::InitializeBuffers()
{
  int idx;

  this->ZData = vtkFloatArray::New();
  this->PData = vtkUnsignedCharArray::New();  
  this->PData->SetNumberOfComponents(3);
  this->ZData2 = vtkFloatArray::New();
  this->PData2 = vtkUnsignedCharArray::New();  
  this->PData2->SetNumberOfComponents(3);

  this->NumberOfTiles = this->TileDimensions[0] * this->TileDimensions[1];
  this->TileZData = new vtkFloatArray*[this->NumberOfTiles];
  this->TilePData = new vtkUnsignedCharArray*[this->NumberOfTiles];
  for (idx = 0; idx < this->NumberOfTiles; ++idx)
    {
    this->TileZData[idx] = vtkFloatArray::New();
    this->TilePData[idx] = vtkUnsignedCharArray::New();  
    this->TilePData[idx]->SetNumberOfComponents(3);
    }
}

//-------------------------------------------------------------------------
void vtkTiledDisplayManager::SetPDataSize(int x, int y)
{
  int numPixels;
  int numComps;
  int idx;

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

  numPixels = x * y;

  vtkCompositeManager::ResizeFloatArray(
      static_cast<vtkFloatArray*>(this->ZData), 
      1, numPixels);
  vtkCompositeManager::ResizeFloatArray(
      static_cast<vtkFloatArray*>(this->ZData2), 
      1, numPixels);
  for (idx = 0; idx < this->NumberOfTiles; ++idx)
    {
    vtkCompositeManager::ResizeFloatArray(
        static_cast<vtkFloatArray*>(this->TileZData[idx]), 
        1, numPixels);
    }

  numComps = 3;
  
  vtkCompositeManager::ResizeUnsignedCharArray(
      static_cast<vtkUnsignedCharArray*>(this->PData), 
      numComps, numPixels);
  vtkCompositeManager::ResizeUnsignedCharArray(
      static_cast<vtkUnsignedCharArray*>(this->PData2), 
      numComps, numPixels);
  for (idx = 0; idx < this->NumberOfTiles; ++idx)
    {
    vtkCompositeManager::ResizeUnsignedCharArray(
        static_cast<vtkUnsignedCharArray*>(this->TilePData[idx]), 
        numComps, numPixels);
    }
}


//==========================================================================
// End of schedule object stuff.
//==========================================================================


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

  this->Schedule = NULL;
  this->ZeroEmpty = 1;
  this->CompositeFlag = 1;

  this->PDataSize[0] = 0;
  this->PDataSize[1] = 0;

  this->NumberOfTiles = 0;
  this->PData = NULL;
  this->ZData = NULL;
  this->PData2 = NULL;
  this->ZData2 = NULL;
  this->TilePData = NULL;
  this->TileZData = NULL;
}

  
//-------------------------------------------------------------------------
vtkTiledDisplayManager::~vtkTiledDisplayManager()
{
  int idx;

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

  if (this->ZData)
    {
    this->ZData->Delete();
    this->ZData = NULL;
    }
  if (this->PData)
    {
    this->PData->Delete();
    this->PData = NULL;
    }
  if (this->ZData2)
    {
    this->ZData2->Delete();
    this->ZData2 = NULL;
    }
  if (this->PData2)
    {
    this->PData2->Delete();
    this->PData2 = NULL;
    }

  for (idx = 0; idx < this->NumberOfTiles; ++idx)
    {
    if (this->TilePData && this->TilePData[idx])
      {
      this->TilePData[idx]->Delete();
      this->TilePData[idx] = NULL;
      }
    if (this->TileZData && this->TileZData[idx])
      {
      this->TileZData[idx]->Delete();
      this->TileZData[idx] = NULL;
      }
    }
  if (this->TilePData)
    {
    delete [] this->TilePData;
    this->TilePData = NULL;
    }
  if (this->TileZData)
    {
    delete [] this->TileZData;
    this->TileZData = NULL;
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

  // This renders all frames
  this->SatelliteStartRender();

  if (this->CompositeFlag)
    {
    this->Composite();
    }

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
void vtkTiledDisplayManager::Composite()
{
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->NumberOfProcesses;
  vtkTiledDisplayProcess *tdp;
  vtkTiledDisplayElement *tde;
  int i, id;
  int uncompressedLength = this->ZData->GetNumberOfTuples();
  int bufSize=0;
  int numComps = this->PData->GetNumberOfComponents();

  //this->Timer->StartTimer();

  // We allocated with special mpiPro new so we do not need to copy.
#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(0);
#endif
  tdp = this->Schedule->Processes[myId];
  for (i = 0; i < tdp->Length; i++) 
    {
    tde = tdp->Elements[i];
    if (tde->ReceiveFlag)
      {
      this->Controller->Receive(&bufSize, 1, tde->OtherProcessId, 98);
      this->Controller->Receive(this->ZData->GetPointer(0), bufSize, 
                                tde->OtherProcessId, 99);
      // Couldn't I compute the second bufSize (3*bufSize) !!!!!!!!!!!!!!!
      this->Controller->Receive(&bufSize, 1, tde->OtherProcessId, 98);
      this->Controller->Receive(reinterpret_cast<unsigned char*>
                                      (this->PData->GetVoidPointer(0)), 
                                      bufSize, tde->OtherProcessId, 99);
      // The result is stored in the second local buffer.
      vtkCompressCompositer::CompositeImagePair(this->TileZData[tde->TileId], 
                                 this->TilePData[tde->TileId], 
                                 this->ZData, this->PData,
                                 this->ZData2, this->PData2);

      // Swap the temp buffers with tile buffers.
      vtkFloatArray *zTemp = this->ZData2;
      vtkUnsignedCharArray *pTemp = this->PData2;
      this->ZData2 = this->TileZData[tde->TileId];
      this->PData2 = this->TilePData[tde->TileId];
      this->TileZData[tde->TileId] = zTemp;
      this->TilePData[tde->TileId] = pTemp;
      }
    else 
      {
      bufSize = this->TileZData[tde->TileId]->GetNumberOfTuples();
      this->Controller->Send(&bufSize, 1, id, 98);
      this->Controller->Send(this->TileZData[tde->TileId]->GetPointer(0), bufSize, id, 99);
      bufSize = this->TilePData[tde->TileId]->GetNumberOfTuples() * numComps;
      this->Controller->Send(&bufSize, 1, id, 98);
      this->Controller->Send(reinterpret_cast<unsigned char*>
                             (this->TileZData[tde->TileId]->GetVoidPointer(0)), 
                             bufSize, id, 99);
          
      }
    }

#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  // Just hard code zeroEmpty
  if (myId > 0 && myId <= this->NumberOfTiles)
    {
    // Now we want to decompress into the original buffers.
    // Ignore z because it is not used by composite manager.
    vtkCompressCompositer::Uncompress(this->TileZData[myId], this->TilePData[myId], 
                     this->PData, uncompressedLength);
    int* windowSize = this->RenderWindow->GetSize();
    this->RenderWindow->SetPixelData(0, 0, windowSize[0]-1, 
                                    windowSize[1]-1, this->PData, 0);
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

  this->SetPDataSize(winInfo.Size[0], winInfo.Size[1]);


  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();

  // This loop is really dumb,  tile will have only one renderer.
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

      // This flag should really be transmitted from the root.
      // This is a place holder until I activate the feature.
      if ( ! this->CompositeFlag)
        { // Just set up this one tile and render.
        // Figure out the tile indexes.
        i = this->Controller->GetLocalProcessId() - 1;
        y = i/this->TileDimensions[0];
        x = i - y*this->TileDimensions[0];

        cam->SetWindowCenter(1.0-(double)(this->TileDimensions[0]) + 2.0*(double)x,
                             1.0-(double)(this->TileDimensions[1]) + 2.0*(double)y);
        renWin->Render();
        }
      else
        {
        int front = 1;
        // All this rendering should be done in the back buffer without any swaps.
        for (int idx = 0; idx < this->NumberOfTiles; ++idx)
          {
          // Figure out the tile indexes.
          i = this->Controller->GetLocalProcessId() - 1;
          y = i/this->TileDimensions[0];
          x = i - y*this->TileDimensions[0];

          cam->SetWindowCenter(1.0-(double)(this->TileDimensions[0]) + 2.0*(double)x,
                               1.0-(double)(this->TileDimensions[1]) + 2.0*(double)y);
          renWin->Render();
          // This is specific to RGB char pixels.
          this->RenderWindow->GetPixelData(
                   0,0,this->PDataSize[0]-1, this->PDataSize[1]-1, 
                   front,static_cast<vtkUnsignedCharArray*>(this->PData));
          this->RenderWindow->GetZbufferData(0,0,
                                            this->PDataSize[0]-1, 
                                            this->PDataSize[1]-1,
                                            this->ZData);  
          // Copy the buffers into the tile array and compress at the same time.
          vtkCompressCompositer::Compress(this->ZData, this->PData,
                       this->TileZData[idx], this->TilePData[idx]);
          }
        }
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
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;

  os << indent << "Controller: (" << this->Controller << ")\n"; 

  if (this->Schedule)
    {
    this->Schedule->PrintSelf(os, indent);
    }
}



