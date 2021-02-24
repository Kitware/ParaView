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

#include "vtkCompositeDataPipeline.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStreamingPriorityQueue.h"

#include <algorithm>
#include <assert.h>
#include <map>
#include <queue>
#include <set>
#include <vector>

namespace
{
class vtkParticlesComparator
{
public:
  bool operator()(
    const vtkStreamingPriorityQueueItem& me, const vtkStreamingPriorityQueueItem& other) const
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

  vtkInternals() { this->ResetPreviousViewPlanes(); }
  void ResetPreviousViewPlanes() { memset(this->PreviousViewPlanes, 0, sizeof(double) * 24); }
  bool PlanesChanged(const double view_planes[24])
  {
#ifdef _MSC_VER
#pragma warning(push)
// Disable C4996 because it warns us that the pointer range here cannot be
// guaranteed by the runtime to not overflow (we're not using Microsoft's
// checked iterators). Since we know that 24 is the size of everything here,
// suppress the warning.
#pragma warning(disable : 4996)
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
  this->Controller = nullptr;
  this->UseBlockDetailInformation = false;
  this->AnyProcessCanLoadAnyBlock = true;
  this->DetailLevelToLoad = 8.5e-5;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkStreamingParticlesPriorityQueue::~vtkStreamingParticlesPriorityQueue()
{
  delete this->Internals;
  this->Internals = nullptr;
  this->SetController(nullptr);
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

static inline bool hasOneLikeXButGreater(
  unsigned int x, unsigned int stride, std::map<unsigned, unsigned>& map)
{
  std::map<unsigned, unsigned>::iterator itr = map.find(x % stride);
  if (itr == map.end())
  {
    return false;
  }
  else
  {
    return itr->second >= x;
  }
}

static inline bool hasOneLikeXButLess(
  unsigned int x, unsigned int stride, std::map<unsigned, unsigned>& map)
{
  std::map<unsigned, unsigned>::iterator itr = map.find(x % stride);
  if (itr == map.end())
  {
    return false;
  }
  else
  {
    return itr->second < x;
  }
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::UpdatePriorities(const double view_planes[24])
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
  unsigned int num_block_per_level = 0;
  bool all_levels_have_same_block_count = true;
  for (unsigned int level = 0; level < num_levels; level++)
  {
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(metadata->GetBlock(level));
    assert(mb != nullptr);

    unsigned int num_blocks = mb->GetNumberOfBlocks();
    if (num_blocks > num_block_per_level)
    {
      num_block_per_level = num_blocks;
      all_levels_have_same_block_count = level == 0;
    }
    for (unsigned int cc = 0; cc < num_blocks; cc++, block_index++)
    {
      if (!mb->HasMetaData(cc) ||
        !mb->GetMetaData(cc)->Has(vtkStreamingDemandDrivenPipeline::BOUNDS()))
      {
        continue;
      }

      vtkStreamingPriorityQueueItem item;
      item.Identifier = block_index;
      item.Refinement = level;

      double bounds[6];
      vtkInformation* blockInfo = mb->GetMetaData(cc);
      blockInfo->Get(vtkStreamingDemandDrivenPipeline::BOUNDS(), bounds);
      item.Bounds.SetBounds(bounds);
      if (blockInfo->Has(vtkCompositeDataPipeline::BLOCK_AMOUNT_OF_DETAIL()))
      {
        item.AmountOfDetail = blockInfo->Get(vtkCompositeDataPipeline::BLOCK_AMOUNT_OF_DETAIL());
      }
      if (this->AnyProcessCanLoadAnyBlock ||
        (blockInfo->Has(vtkCompositeDataSet::CURRENT_PROCESS_CAN_LOAD_BLOCK()) &&
            blockInfo->Get(vtkCompositeDataSet::CURRENT_PROCESS_CAN_LOAD_BLOCK())))
      {
        queue.push(item);
      }
    }
  }
  if (!all_levels_have_same_block_count)
  {
    num_block_per_level *= num_levels;
  }
  double clamp_bounds[6];
  vtkMath::UninitializeBounds(clamp_bounds);
  queue.UpdatePriorities(view_planes, clamp_bounds);

  std::map<unsigned, unsigned> blocksRequested;
  for (std::set<unsigned>::iterator itr = this->Internals->BlocksRequested.begin();
       itr != this->Internals->BlocksRequested.end(); ++itr)
  {
    blocksRequested[*itr % num_block_per_level] = *itr;
  }
  this->Internals->BlocksRequested.clear();

  this->Internals->BlocksToPurge.clear();

  std::deque<unsigned int> toRequest;
  std::map<unsigned, unsigned> keepInRequest;
  while (!queue.empty())
  {
    vtkStreamingPriorityQueueItem item = queue.top();

    queue.pop();
    //    if (item.Distance + item.Refinement <= 0 || item.Refinement < 1)
    double diagonal = item.Bounds.GetDiagonalLength();
    // avoid division by 0
    if (diagonal < 1e-10)
    {
      diagonal = 1e-10;
    }
    double factor = item.Refinement == 0 ? 0 : this->DetailLevelToLoad / (double)item.Refinement;
    bool detailMethodNeedsBlock =
      (item.Refinement <= 0 || (item.Distance / diagonal < factor && item.ScreenCoverage > 0));
    //        (item.Refinement <= 0 ||
    //         (item.ItemCoverage > 0 && item.ScreenCoverage / (item.AmountOfDetail *
    //         item.ItemCoverage ) > this->DetailLevelToLoad));
    bool genericMethodNeedsBlock = (item.Refinement <= 1 || item.ScreenCoverage >= 0.75);

    if ((this->UseBlockDetailInformation && item.AmountOfDetail > 0) ? detailMethodNeedsBlock
                                                                     : genericMethodNeedsBlock)
    {
      if (hasOneLikeXButGreater(item.Identifier, num_block_per_level, keepInRequest) ||
        hasOneLikeXButGreater(item.Identifier, num_block_per_level, blocksRequested))
      {
        // we already requested a greater resolution of the block, do nothing
      }
      else if (hasOneLikeXButLess(item.Identifier, num_block_per_level, blocksRequested))
      {
        // we have loaded a lower resolution of the block:
        // 1: request the new (higher) one
        toRequest.push_back(item.Identifier);
        keepInRequest[item.Identifier % num_block_per_level] = item.Identifier;
        // 2: delete the one we currently have loaded
        this->Internals->BlocksToPurge.insert(
          blocksRequested[item.Identifier % num_block_per_level]);
        blocksRequested.erase(item.Identifier % num_block_per_level);
      }
      else if (hasOneLikeXButLess(item.Identifier, num_block_per_level, keepInRequest))
      {
        // we are requesting a lower resolution of the block, so change the request
        // to the highest resolution we need
        keepInRequest[item.Identifier % num_block_per_level] = item.Identifier;
        toRequest.push_back(item.Identifier);
      }
      else
      {
        // we don't have this block loaded or requested at all, request the current
        // resolution
        keepInRequest[item.Identifier % num_block_per_level] = item.Identifier;
        toRequest.push_back(item.Identifier);
      }
    }
    else
    {
      // if we don't need the block at this resolution, but we have it requested at this
      // level or higher then delete the resolution we have and request the resolution
      // below the current one we don't need.  We have to check the request list too since
      // we are adding to the request list without knowing for sure that we need the resolution
      // we are requesting.  Ideally if we are seeing a higher res block in this list, we have
      // already seen the lower resolution, but that is not guaranteed by the priority of the
      // blocks.
      if (hasOneLikeXButGreater(item.Identifier, num_block_per_level, blocksRequested) ||
        hasOneLikeXButGreater(item.Identifier, num_block_per_level, keepInRequest))
      {
        // request the next one lower than this one (may not need that one, but we should show
        // something for each)
        // use max to ensure we show the lowest one not an invalid block index
        keepInRequest[item.Identifier % num_block_per_level] =
          std::max((int)item.Identifier - (int)num_block_per_level,
            (int)(item.Identifier % num_block_per_level));
        toRequest.push_back(std::max((int)item.Identifier - (int)num_block_per_level,
          (int)(item.Identifier % num_block_per_level)));
        // this may be called when the block in question is merely to be requested and not loaded
        // yet
        // if the block is loaded, delete it
        if (hasOneLikeXButGreater(item.Identifier, num_block_per_level, blocksRequested))
        {
          this->Internals->BlocksToPurge.insert(
            blocksRequested[item.Identifier % num_block_per_level]);
          blocksRequested.erase(item.Identifier % num_block_per_level);
        }
      }
    }
  }

  for (std::deque<unsigned int>::iterator itr = toRequest.begin(); itr != toRequest.end(); ++itr)
  {
    std::map<unsigned, unsigned>::iterator keep = keepInRequest.find(*itr % num_block_per_level);
    std::set<unsigned>::iterator purge = this->Internals->BlocksToPurge.find(*itr);
    if (keep != keepInRequest.end() && keep->second == *itr &&
      (all_levels_have_same_block_count || purge == this->Internals->BlocksToPurge.end()))
    {
      this->Internals->BlocksToRequest.push(*itr);
    }
  }
  for (std::map<unsigned, unsigned>::iterator itr = blocksRequested.begin();
       itr != blocksRequested.end(); ++itr)
  {
    this->Internals->BlocksRequested.insert(itr->second);
  }

  cout << "Update information  : " << endl
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

  if (this->AnyProcessCanLoadAnyBlock)
  {
    int myid = this->Controller->GetLocalProcessId();
    int num_ranks = this->Controller->GetNumberOfProcesses();
    std::vector<unsigned int> items;
    items.resize(num_ranks);
    for (int i = 0; i < num_ranks; ++i)
    {
      items[i] = this->Internals->BlocksToRequest.front();
      this->Internals->BlocksToRequest.pop();
      this->Internals->BlocksRequested.insert(items[i]);
    }
    return items[myid];
  }
  else
  {
    unsigned int item;

    item = this->Internals->BlocksToRequest.front();
    this->Internals->BlocksToRequest.pop();
    this->Internals->BlocksRequested.insert(item);

    // As the queue is assumed to be of the local process's blocks now, return
    // the first block in the queue (will be different on each process
    return item;
  }
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Update(const double view_planes[24])
{
  double clamp_bounds[6];
  vtkMath::UninitializeBounds(clamp_bounds);
  this->Update(view_planes, clamp_bounds);
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesPriorityQueue::Update(
  const double view_planes[24], const double vtkNotUsed(clamp_bounds)[6])
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
