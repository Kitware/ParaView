/*=========================================================================

  Program:   ParaView
  Module:    vtkTiledDisplaySchedule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTiledDisplaySchedule.h"
#include "vtkObjectFactory.h"


vtkCxxRevisionMacro(vtkTiledDisplaySchedule, "1.9.2.1");
vtkStandardNewMacro(vtkTiledDisplaySchedule);


// Here are internal object for storing the schedule.
// Elemnent representing communication step of compositing.
class vtkTiledDisplayElement 
{
public:
  // Which tile is this step for.  This is used to set/get
  // the correct tile buffer.
  int TileId;
  // This is not used.  It was just for debuggin purposes.
  int CompositeLevel;
  // Does ths element send or receive a buffer.
  int ReceiveFlag;
  // Used temporarily for creation of single tile schedule.
  // Id relative to the standard tree.
  int OtherCompositeId;
  // After processes have been shuffled, the compositeId is
  // converted into this absolute process id.  This conversion uses
  // the compositeId stored in the process.
  int OtherProcessId;
  // This step can be dependant on a previous step.
  // This reference is here to make sure we keep the
  // dependent ording intack which shuffling schedules together.
  // These values are only used for creating/shuffling the schedules.
  // Only the sending elements have a dependencies.
  vtkTiledDisplayElement* Dependency;
  int Shuffled;
};

class vtkTiledDisplayProcess 
{
public:
  vtkTiledDisplayProcess();
  ~vtkTiledDisplayProcess();
  int TileId; // not necessary
  int CompositeId;

  // int NumberOfElements ....!!!!!!!
  int Length;
  vtkTiledDisplayElement** Elements;
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
  this->NumberOfTiles = 0;
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
  this->NumberOfTiles = 0;
  if (this->Processes)
    {
    delete [] this->Processes;
    this->Processes = NULL;
    }
}

//-------------------------------------------------------------------------
int vtkTiledDisplaySchedule::GetProcessTileId(int procIdx)
{
  if (procIdx < 0 || procIdx >= this->NumberOfProcesses)
    {
    vtkErrorMacro("Bad process id.");
    return -1;
    }

  return this->Processes[procIdx]->TileId;
}


//-------------------------------------------------------------------------
int vtkTiledDisplaySchedule::GetNumberOfProcessElements(int procIdx)
{
  if (procIdx < 0 || procIdx >= this->NumberOfProcesses)
    {
    vtkErrorMacro("Bad process id.");
    return 0;
    }

  return this->Processes[procIdx]->Length;
}

//-------------------------------------------------------------------------
int vtkTiledDisplaySchedule::GetElementOtherProcessId(int procIdx, 
                                                      int elementIdx)
{
  if (procIdx < 0 || procIdx >= this->NumberOfProcesses)
    {
    vtkErrorMacro("Bad process id.");
    return -1;
    }

  vtkTiledDisplayProcess* p = this->Processes[procIdx];
  if (elementIdx < 0 || elementIdx >= p->Length)
    {
    vtkErrorMacro("Bad element id.");
    return -1;
    }
  if (p->Elements[elementIdx] == NULL)
    {
    vtkErrorMacro("MissingElement");
    return -1;
    }

  return p->Elements[elementIdx]->OtherProcessId;
}

//-------------------------------------------------------------------------
int vtkTiledDisplaySchedule::GetElementReceiveFlag(int procIdx, 
                                                   int elementIdx)
{
  if (procIdx < 0 || procIdx >= this->NumberOfProcesses)
    {
    vtkErrorMacro("Bad process id.");
    return -1;
    }

  vtkTiledDisplayProcess* p = this->Processes[procIdx];
  if (elementIdx < 0 || elementIdx >= p->Length)
    {
    vtkErrorMacro("Bad element id.");
    return -1;
    }
  if (p->Elements[elementIdx] == NULL)
    {
    vtkErrorMacro("MissingElement");
    return -1;
    }

  return p->Elements[elementIdx]->ReceiveFlag;
}
 
//-------------------------------------------------------------------------
int vtkTiledDisplaySchedule::GetElementTileId(int procIdx, 
                                              int elementIdx)
{
  if (procIdx < 0 || procIdx >= this->NumberOfProcesses)
    {
    vtkErrorMacro("Bad process id.");
    return -1;
    }

  vtkTiledDisplayProcess* p = this->Processes[procIdx];
  if (elementIdx < 0 || elementIdx >= p->Length)
    {
    vtkErrorMacro("Bad element id.");
    return -1;
    }
  if (p->Elements[elementIdx] == NULL)
    {
    vtkErrorMacro("MissingElement");
    return -1;
    }

  return p->Elements[elementIdx]->TileId;
}

//-------------------------------------------------------------------------
// Only worry about power of 2 processes for now.
// Set up the scedule for a single tile.  Final results
// endup on process "tileProcess".
void vtkTiledDisplaySchedule::InitializeForTile(int tileId, 
                                                int tileProcess,
                                                int numProcs) 
{
  int pIdx, pIdxSend;
  vtkTiledDisplayProcess* p;
  int eIdx;
  vtkTiledDisplayElement* e;
  int maxLevels;
  int level;
  int power2;

  // I could just set this to be NumberOfProcesses, but that would be wasteful.
  maxLevels = (int)(ceil(log((double)(numProcs))/log(2.0)));
  power2 = 1 << maxLevels;

  this->NumberOfProcesses = numProcs;
  this->NumberOfTiles = 1;
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

  // Loop over tree levels.
  // only works for power of two for now.
  level = 0;
  while (power2 > 1)
    {
    // Loop over and create send and receive elements
    power2 = power2 >> 1;
    for (pIdx = 0; pIdx < power2; ++ pIdx)
      {
      pIdxSend = pIdx+power2;
      if (pIdxSend < numProcs)
        {
        // Receiving process    
        p = this->Processes[pIdx];
        e = new vtkTiledDisplayElement;
        p->Elements[p->Length] = e;
        e->ReceiveFlag = 1;
        e->TileId = tileId;
        e->OtherCompositeId = pIdxSend;
        e->OtherProcessId = -1;
        e->CompositeLevel = level;
        e->Dependency = NULL;
        e->Shuffled = 0;
        ++(p->Length);
        if (p->Length > maxLevels)
          { // Sanity check
          vtkGenericWarningMacro("Too many levels.");
          }
        // Sending process
        p = this->Processes[pIdxSend];
        e = new vtkTiledDisplayElement;
        p->Elements[p->Length] = e;
        e->ReceiveFlag = 0;
        e->TileId = tileId;
        e->OtherCompositeId = pIdx;
        e->OtherProcessId = -1;
        e->CompositeLevel = level;
        e->Dependency = NULL;
        if (p->Length > 0)
          { // Do not shuffle this until previous element is shuffled.
          e->Dependency = p->Elements[p->Length-1];
          }
        e->Shuffled = 0;
        ++(p->Length);
        if (p->Length > maxLevels)
          { // Sanity check
          vtkGenericWarningMacro("Too many levels.");
          }
        }
      }
    ++level;
    }

  // Swap processes so that the end result ends up on the tileProcess.
  if (tileProcess != 0)
    {
    p = this->Processes[0];
    this->Processes[0] = this->Processes[tileProcess];
    this->Processes[tileProcess] = p;
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

  // Initialize the map just in case something goes wrong.
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

  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTiles: " << this->NumberOfTiles << "\n";
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
void vtkTiledDisplaySchedule::InitializeTiles(int numTiles, int numProcs)
{
  int  tIdx, pIdx;
  int sum, max;
  int* totalProcessLengths=0;
  vtkTiledDisplaySchedule* ts;
  vtkTiledDisplayProcess* p;
  int i, j;

  if (numProcs == 1 && numTiles > 1)
    { // We must be running client server, and we must be the client.
    return;
    }
      
  this->NumberOfProcesses = numProcs;
  this->NumberOfTiles = numTiles;

  // Create a schedule for each tile.
  vtkTiledDisplaySchedule** tileSchedules;
  tileSchedules = new vtkTiledDisplaySchedule* [numTiles];
  for (tIdx = 0; tIdx < numTiles; ++tIdx)
    {
    ts = vtkTiledDisplaySchedule::New();
    tileSchedules[tIdx] = ts;
    // This assumes that the first n processes are displaying tiles.
    ts->InitializeForTile(tIdx, tIdx, numProcs);
    }

  // Create an array which store the total number of levels
  // for each processes.
  totalProcessLengths = new int[this->NumberOfProcesses];
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    sum = 0;
    for (tIdx = 0; tIdx < numTiles; ++tIdx)
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
    for (tIdx = 0; tIdx < numTiles; ++tIdx)
      {
      ts = tileSchedules[tIdx];
      for (i = 0; i < numProcs; ++i)
        {
        for (j = i+1; j < numProcs; ++j)
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
  for (tIdx = 0; tIdx < numTiles; ++tIdx)
    {
    ts = tileSchedules[tIdx];
    ts->ComputeElementOtherProcessIds();
    }

  // Now shuffle tile schedules into one jumbo schedule.
  max = 1;
  for (pIdx = 0; pIdx < numProcs; ++pIdx)
    {
    if (totalProcessLengths[pIdx] > max)
      {
      max = totalProcessLengths[pIdx];
      }
    }  

  // Create the  processes 
  // (Elements added later by shuffle).
  this->NumberOfProcesses = numProcs;
  this->Processes = 
           new vtkTiledDisplayProcess*[this->NumberOfProcesses];
  for (pIdx = 0; pIdx < this->NumberOfProcesses; ++pIdx)
    {
    p = new vtkTiledDisplayProcess;
    this->Processes[pIdx] = p;
    p->TileId = -1;
    if (pIdx < numTiles)
      {
      p->TileId = pIdx;
      }
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
  int shuffledLevel = 0;
  while (flag)
    {
    flag = this->ShuffleLevel(shuffledLevel, numTiles, tileSchedules);
    ++shuffledLevel;
    }

  // Delete the tile schedules.
  for (tIdx = 0; tIdx < numTiles; ++tIdx)
    {
    ts = tileSchedules[tIdx];
    ts->Delete();
    tileSchedules[tIdx] = NULL;
    }
  delete [] tileSchedules;
  delete [] totalProcessLengths;
}

//-------------------------------------------------------------------------
// I am just going to try brute force adding one level at a time.
// Only when a send and receive can be in the same level (or less)
// does it get copied over.  The tile first loop is ideal because
// it composites as much of one tile as possible before moving (rendering)
// onto the next tile.  This is good for buffer management, and
// should be more efficient for communication.
int vtkTiledDisplaySchedule::ShuffleLevel(int level, int numTiles, 
                                    vtkTiledDisplaySchedule** tileSchedules)
{
  int flag = 0;
  int tIdx, pIdx, eIdx;
  int pIdxOther, eIdxOther;
  vtkTiledDisplaySchedule* ts;
  vtkTiledDisplayProcess* p;
  vtkTiledDisplayProcess* pOther;
  vtkTiledDisplayElement* e;
  vtkTiledDisplayElement* eOther;
  vtkTiledDisplayProcess* p2;
  vtkTiledDisplayProcess* p2Other;

  for (tIdx = 0; tIdx < numTiles; ++tIdx)
    {
    ts = tileSchedules[tIdx];
    for (pIdx = 0; pIdx < ts->NumberOfProcesses; ++pIdx)
      {
      p = ts->Processes[pIdx];
      for (eIdx = 0; eIdx < p->Length; ++eIdx)
        {
        e = p->Elements[eIdx];
        if (e)
          {
          // Consider sends and receive as pair.
          pIdxOther = e->OtherProcessId;
          pOther = ts->Processes[pIdxOther];
          eIdxOther = this->FindOtherElementIdx(pOther, e, pIdx);
          eOther = pOther->Elements[eIdxOther];
          if (eOther == NULL)
            { // Sanity check.
            vtkErrorMacro("Schedule bug: Could not locate other element.");
            } 
          p2 = this->Processes[pIdx];
          p2Other = this->Processes[pIdxOther];
          if (p2->Length <= level && p2Other->Length <= level)
            { // We still have space in the combined/shuffled level.
            // Check to make sure we have already taken care of dependencies.
            if (e->Dependency == NULL || e->Dependency->Shuffled)
              { // e dependencies OK.
              if (eOther->Dependency == NULL || eOther->Dependency->Shuffled)
                { // eOther dependencies OK.
                // Shuffle them into the final schedule.
                p->Elements[eIdx] = NULL;
                p2->Elements[p2->Length] = e;
                e->Shuffled = 1;
                ++(p2->Length);
                pOther->Elements[eIdxOther] = NULL;
                p2Other->Elements[p2Other->Length] = eOther;
                eOther->Shuffled = 1;
                ++(p2Other->Length);
                }
              }
            }
          if ( ! e->Shuffled || ! eOther->Shuffled)
            { // One check would be enough ...
            // We skipped this element pair.  Call this method again.
            flag = 1;
            }
          }
        }
      }
    }
  return flag;
}


//-------------------------------------------------------------------------
int vtkTiledDisplaySchedule::FindOtherElementIdx(vtkTiledDisplayProcess* p, 
                                                 vtkTiledDisplayElement* e,
                                                 int pId)
{
  int idx;
  vtkTiledDisplayElement* e2;

  for (idx = 0; idx < p->Length; ++idx)
    {
    e2 = p->Elements[idx];
    if (e2 && e2->TileId == e->TileId)
      {
      if ((e2->ReceiveFlag == 1 && e->ReceiveFlag == 0) ||
          (e2->ReceiveFlag == 0 && e->ReceiveFlag == 1))
        {
        if (e2->OtherProcessId == pId)
          {
          return idx;
          }
        }
      }
    }

  vtkErrorMacro("Could not find other element.");
  return -1;
}
