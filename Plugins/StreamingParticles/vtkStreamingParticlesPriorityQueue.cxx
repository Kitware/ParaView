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
#include <set>

namespace
{
  class vtkParticlesComparator
    {
  public:
    bool operator()(const vtkStreamingPriorityQueueItem& me, const
      vtkStreamingPriorityQueueItem& other) const
      {
      if (me.ScreenCoverage != other.ScreenCoverage)
        {
        return me.ScreenCoverage < other.ScreenCoverage;
        }
      if (me.Refinement != other.Refinement)
        {
        return me.Refinement > other.Refinement;
        }
      return me.Distance > other.Distance;
      }
    };
}
class vtkStreamingParticlesPriorityQueue::vtkInternals
{
public:
  vtkSmartPointer<vtkMultiBlockDataSet> Metadata;
  std::queue<unsigned int> BlocksToRequest;
  std::set<unsigned int> BlocksRequested;
  std::set<unsigned int> BlocksToPurge;

  double PreviousViewPlanes[24];

  vtkInternals()
    {
    this->ResetPreviousViewPlanes();
    }
  void ResetPreviousViewPlanes()
    {
    memset(this->PreviousViewPlanes, 0, sizeof(double)*24);
    }
  bool PlanesChanged(const double view_planes[24])
    {
#ifdef _MSC_VER
#pragma warning(push)
// Disable C4996 because it warns us that the pointer range here cannot be
// guaranteed by the runtime to not overflow (we're not using Microsoft's
// checked iterators). Since we know that 24 is the size of everything here,
// suppress the warning.
#pragma warning(disable:4996)
#endif
    return !std::equal(this->PreviousViewPlanes, this->PreviousViewPlanes + 24, view_planes);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }
  void SetViewPlanes(const double view_planes[24])
    {
    std::copy(view_planes, view_planes + 24, this->PreviousViewPlanes);
    }
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
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Reinitialize()
{
  if (this->Internals->Metadata)
    {
    std::set<unsigned int> blocksRequested;
    blocksRequested.swap(this->Internals->BlocksRequested);

    vtkSmartPointer<vtkMultiBlockDataSet> info = this->Internals->Metadata;
    this->Initialize(info);

    // restore blocks requested since data didn;t change.
    this->Internals->BlocksRequested.swap(blocksRequested);
    }
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::UpdatePriorities(
  const double view_planes[24])
{
  assert(this->Internals && this->Internals->Metadata);
  assert(this->Internals->BlocksToRequest.empty());

  vtkMultiBlockDataSet* metadata = this->Internals->Metadata;

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

  vtkStreamingPriorityQueue<vtkParticlesComparator> queue;

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

      double bounds[6];
      mb->GetMetaData(cc)->Get(vtkStreamingDemandDrivenPipeline::BOUNDS(), bounds);
      item.Bounds.SetBounds(bounds);

      queue.push(item);
      }
    }
  double clamp_bounds[6];
  vtkMath::UninitializeBounds(clamp_bounds);
  queue.UpdatePriorities(view_planes, clamp_bounds);

  std::set<unsigned int> blocksRequested;
  blocksRequested.swap(this->Internals->BlocksRequested);

  while (!queue.empty())
    {
    vtkStreamingPriorityQueueItem item = queue.top();
    queue.pop();
    if (item.Refinement <= 1 || item.ScreenCoverage >= 0.75)
      {
      if (blocksRequested.find(item.Identifier) != blocksRequested.end())
        {
        // this block was previously requested, pretend we already requested it
        // in the current iteration as well.
        this->Internals->BlocksRequested.insert(item.Identifier);
        blocksRequested.erase(item.Identifier);
        }
      else
        {
        // we need to request this block.
        this->Internals->BlocksToRequest.push(item.Identifier);
        }
      }
    }

  // - anything in blocksRequested is what is "extra" and should
  // be purged.
  // - anything in this->Internals->BlocksToRequest is to be requested.

  this->Internals->BlocksToPurge.clear();
  this->Internals->BlocksToPurge.swap(blocksRequested);
  cout << "Update information  : "  << endl
       << "  To request        : " << this->Internals->BlocksToRequest.size() << endl
       << "  Already requested : " << this->Internals->BlocksRequested.size() << endl
       << "  To purge          : " << this->Internals->BlocksToPurge.size() << endl;
}

//----------------------------------------------------------------------------
bool vtkStreamingParticlesPriorityQueue::IsEmpty()
{
  return this->Internals->BlocksToRequest.empty();
}

//----------------------------------------------------------------------------
unsigned int vtkStreamingParticlesPriorityQueue::Pop()
{
  if (this->IsEmpty())
    {
    return VTK_UNSIGNED_INT_MAX;
    }

  int num_procs = this->Controller? this->Controller->GetNumberOfProcesses() : 1;
  int myid = this->Controller? this->Controller->GetLocalProcessId() : 0;
  assert(myid < num_procs);

  std::vector<unsigned int> items;
  items.resize(num_procs, VTK_UNSIGNED_INT_MAX);

  for (int cc=0; cc < num_procs && !this->Internals->BlocksToRequest.empty(); cc++)
    {
    items[cc] = this->Internals->BlocksToRequest.front();
    this->Internals->BlocksToRequest.pop();
    this->Internals->BlocksRequested.insert(items[cc]);
    }

  // at the end, when the queue empties out in the middle of a pop, right now,
  // all other processes are simply going to ask for block 0 (set in
  // initialization of vtkPriorityQueueItem). We can change that, if needed.
  return items[myid];
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
  const double vtkNotUsed(clamp_bounds)[6])
{
  this->Internals->BlocksToPurge.clear();

  if (!this->Internals->Metadata)
    {
    return;
    }

  // Check if the view has changed. If so, we update the priorities.
  if (!this->Internals->PlanesChanged(view_planes))
    {
    return;
    }

  this->Reinitialize();
  this->UpdatePriorities(view_planes);
  this->Internals->SetViewPlanes(view_planes);
}

//----------------------------------------------------------------------------
const std::set<unsigned int>& vtkStreamingParticlesPriorityQueue::GetBlocksToPurge() const
{
  return this->Internals->BlocksToPurge;
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
