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

#ifndef __vtkStreamingParticlesPriorityQueue_h
#define __vtkStreamingParticlesPriorityQueue_h

#include "vtkPVClientServerCoreRenderingModule.h" // for export macros
#include "vtkObject.h"
#include <set> // needed for set

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class vtkStreamingParticlesPriorityQueue : public vtkObject
{
public:
  static vtkStreamingParticlesPriorityQueue* New();
  vtkTypeMacro(vtkStreamingParticlesPriorityQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

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

//BTX
protected:
  vtkStreamingParticlesPriorityQueue();
  ~vtkStreamingParticlesPriorityQueue();

  // Description:
  // Updates priorities and builds a BlocksToPurge list.
  void UpdatePriorities(const double view_planes[24]);

  vtkMultiProcessController* Controller;

private:
  vtkStreamingParticlesPriorityQueue(const vtkStreamingParticlesPriorityQueue&); // Not implemented
  void operator=(const vtkStreamingParticlesPriorityQueue&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
