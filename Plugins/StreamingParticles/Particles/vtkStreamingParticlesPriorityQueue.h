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
// .NAME vtkStreamingParticlesPriorityQueue - implements a coverage based priority
// queue for a vtkMultiBlockDataSet.
// .SECTION Description
// vtkStreamingParticlesPriorityQueue is used by representations supporting streaming
// of multi-block datasets to determine the priorities for blocks in the dataset. This
// class relies on the bounds information provided in the multi-block's
// meta-data. This class support view-based priority computation. Simply
// provide the view planes (returned by vtkCamera::GetFrustumPlanes()) to the
// vtkStreamingParticlesPriorityQueue::Update() call to update the prorities for the
// blocks currently in the queue.
//
// This implementation is based on vtkAMRStreamingPriorityQueue.
// .SECTION See Also
// vtkStreamingParticlesRepresentation, vtkAMRStreamingPriorityQueue

#ifndef vtkStreamingParticlesPriorityQueue_h
#define vtkStreamingParticlesPriorityQueue_h

#include "vtkObject.h"
#include "vtkStreamingParticlesModule.h" // for export macro
#include <set>                           // needed for set

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKSTREAMINGPARTICLES_EXPORT vtkStreamingParticlesPriorityQueue : public vtkObject
{
public:
  static vtkStreamingParticlesPriorityQueue* New();
  vtkTypeMacro(vtkStreamingParticlesPriorityQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // If the controller is specified, the queue can be used in parallel. So long
  // as Initialize(), Update() and Pop() methods are called on all processes
  // (need not be synchronized) and all process get the same composite tree and
  // view_planes (which is generally true with ParaView), the blocks are
  // distributed among the processes.
  // By default, this is set to the
  // vtkMultiProcessController::GetGlobalController();
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Initializes the queue. All information about items in the is lost. We only
  // look at the meta-data for the vtkMultiBlockDataSet i.e. none of the heavy
  // data (or leaf nodes) will be tested or checked.
  void Initialize(vtkMultiBlockDataSet* metadata);

  // Description:
  // Re-initializes the priority queue using the multi-block structure given to the most
  // recent call to Initialize().
  void Reinitialize();

  // Description:
  // Updates the priorities of blocks based on the new view frustum planes.
  // Information about blocks "popped" from the queue is preserved and those
  // blocks are not reinserted in the queue.
  void Update(const double view_planes[24], const double clamp_bounds[6]);
  void Update(const double view_planes[24]);

  // Description:
  // Returns if the queue is empty.
  bool IsEmpty();

  // Description:
  // Pops and returns of composite id for the block at the top of the queue.
  // Test if the queue is empty before calling this method.
  unsigned int Pop();

  // Description:
  // After every Update() call, returns the list of blocks that should be purged
  // given the current view.
  const std::set<unsigned int>& GetBlocksToPurge() const;

  // Description:
  // If this variable is set to true and the blocks have
  // vtkPGenericIOMultiBlockReader::BLOCK_AMOUNT_OF_DETAIL information, use this
  // information to determine the blocks to load, otherwise use a default selection
  // method.  Defaults to false.
  vtkGetMacro(UseBlockDetailInformation, bool);
  vtkBooleanMacro(UseBlockDetailInformation, bool);
  vtkSetMacro(UseBlockDetailInformation, bool);

  // Description:
  // If this variable is set to true then the priority queue will internally manage
  // round-robining the blocks across the processes.  If this is set to false, then
  // the dataset should have the vtkCompositeData::CURRENT_PROCESS_CAN_LOAD_BLOCK
  // key on each block's metadata to indicate which process can load each block and
  // this information will be used to determine the process to use for each block.
  // Defaults to true.
  vtkGetMacro(AnyProcessCanLoadAnyBlock, bool);
  vtkBooleanMacro(AnyProcessCanLoadAnyBlock, bool);
  vtkSetMacro(AnyProcessCanLoadAnyBlock, bool);

  // Description:
  // When UseBlockDetailInformation is on, this variable controls the minimum level of
  // detail a block can have and still be loaded.  More specifically, the value from
  // vtkPGenericIOMultiBlockReader::BLOCK_AMOUNT_OF_DETAIL is used with the distance to
  // the block to compute approximate distance between features in the dataset.  This
  // computed distance is compared with this value.  Default: 8.5e-5 seems to work
  // well with point clouds where the BLOCK_AMOUNT_OF_DETAIL is the number of points.
  vtkGetMacro(DetailLevelToLoad, double);
  vtkSetMacro(DetailLevelToLoad, double);

protected:
  vtkStreamingParticlesPriorityQueue();
  ~vtkStreamingParticlesPriorityQueue();

  // Description:
  // Updates priorities and builds a BlocksToPurge list.
  void UpdatePriorities(const double view_planes[24]);

  vtkMultiProcessController* Controller;

  bool UseBlockDetailInformation;
  bool AnyProcessCanLoadAnyBlock;
  double DetailLevelToLoad;

private:
  vtkStreamingParticlesPriorityQueue(const vtkStreamingParticlesPriorityQueue&) = delete;
  void operator=(const vtkStreamingParticlesPriorityQueue&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
