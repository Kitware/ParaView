/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingParticlesPriorityQueue.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStreamingPriorityQueue.h"

#include <algorithm>
#include <assert.h>
#include <queue>
#include <vector>

class vtkStreamingParticlesPriorityQueue::vtkInternals
{
public:
  vtkStreamingPriorityQueue PriorityQueue;
  vtkSmartPointer<vtkMultiBlockDataSet> Metadata;
};

vtkStandardNewMacro(vtkStreamingParticlesPriorityQueue);
vtkCxxSetObjectMacro(vtkStreamingParticlesPriorityQueue, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkStreamingParticlesPriorityQueue::vtkStreamingParticlesPriorityQueue()
{
  this->Internals = new vtkInternals();
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkStreamingParticlesPriorityQueue::~vtkStreamingParticlesPriorityQueue()
{
  delete this->Internals;
  this->Internals = 0;
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Initialize(vtkMultiBlockDataSet* metadata)
{
  delete this->Internals;
  this->Internals = new vtkInternals();
  this->Internals->Metadata = metadata;

  // This assumes for following structure:
  // Root
  //   Level 0
  //     DS 0 (Block Idx 0)
  //     DS 1 (Block Idx 1)
  //   Level 1
  //     DS 0 (Block Idx 2)
  //     DS 1 (Block Idx 3)
  //       .
  //       .
  //       .
  // Where "Block Idx" is the key that needs to be sent up the pipeline to the
  // reader to request a particular block and Level k is lower refinement than
  // Level (k+1).

  unsigned int block_index = 0;
  unsigned int num_levels = metadata->GetNumberOfBlocks();
  for (unsigned int level=0; level < num_levels; level++)
    {
    vtkMultiBlockDataSet* mb =
      vtkMultiBlockDataSet::SafeDownCast(metadata->GetBlock(level));
    assert(mb != NULL);

    unsigned int num_blocks = mb->GetNumberOfBlocks();
    for (unsigned int cc=0; cc < num_blocks; cc++, block_index++)
      {
      if (!mb->HasMetaData(cc) ||
          !mb->GetMetaData(cc)->Has(
            vtkStreamingDemandDrivenPipeline::BOUNDS()))
        {
        continue;
        }

      vtkStreamingPriorityQueueItem item;
      item.Identifier = block_index;
      item.Refinement = level;

      // default priority is to prefer lower levels. Thus even without
      // view-planes we have reasonable priority.
      item.Priority = block_index;

      double bounds[6];
      mb->GetMetaData(cc)->Get(vtkStreamingDemandDrivenPipeline::BOUNDS(), bounds);
      item.Bounds.SetBounds(bounds);

      this->Internals->PriorityQueue.push(item);
      }
    }
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Reinitialize()
{
  if (this->Internals->Metadata)
    {
    vtkSmartPointer<vtkMultiBlockDataSet> info = this->Internals->Metadata;
    this->Initialize(info);
    }
}

//----------------------------------------------------------------------------
bool vtkStreamingParticlesPriorityQueue::IsEmpty()
{
  return this->Internals->PriorityQueue.empty();
}

//----------------------------------------------------------------------------
unsigned int vtkStreamingParticlesPriorityQueue::Pop()
{
  if (this->IsEmpty())
    {
    vtkErrorMacro("Queue is empty!");
    return 0;
    }

  int num_procs = this->Controller? this->Controller->GetNumberOfProcesses() : 1;
  int myid = this->Controller? this->Controller->GetLocalProcessId() : 0;
  assert(myid < num_procs);

  std::vector<vtkStreamingPriorityQueueItem> items;
  items.resize(num_procs);
  for (int cc=0; cc < num_procs && !this->Internals->PriorityQueue.empty(); cc++)
    {
    items[cc] = this->Internals->PriorityQueue.top();
    this->Internals->PriorityQueue.pop();
    }

  // at the end, when the queue empties out in the middle of a pop, right now,
  // all other processes are simply going to ask for block 0 (set in
  // initialization of vtkPriorityQueueItem). We can change that, if needed.
  return items[myid].Identifier;
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Update(const double view_planes[24])
{
  double clamp_bounds[6];
  vtkMath::UninitializeBounds(clamp_bounds);
  this->Update(view_planes, clamp_bounds);
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Update(const double view_planes[24],
  const double clamp_bounds[6])
{
  if (!this->Internals->Metadata)
    {
    return;
    }
  this->Internals->PriorityQueue.UpdatePriorities(view_planes, clamp_bounds);
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
