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
// .NAME vtkAMRStreamingPriorityQueue - implements a coverage based priority
// queue for vtkOverlappingAMR dataset.
// .SECTION Description
// vtkAMRStreamingPriorityQueue is used by representations supporting streaming
// of AMR datasets to determine the priorities for blocks in the dataset. This
// class relies on the bounds information provided by the AMR meta-data i.e.
// vtkAMRInformation. This class support view-based priority computation. Simply
// provide the view planes (returned by vtkCamera::GetFrustumPlanes()) to the
// vtkAMRStreamingPriorityQueue::Update() call to update the prorities for the
// blocks currently in the queue.
// .SECTION See Also
// vtkAMROutlineRepresentation, vtkAMRStreamingVolumeRepresentation.

#ifndef vtkAMRStreamingPriorityQueue_h
#define vtkAMRStreamingPriorityQueue_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" // for export macros

class vtkAMRInformation;
class vtkMultiProcessController;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkAMRStreamingPriorityQueue : public vtkObject
{
public:
  static vtkAMRStreamingPriorityQueue* New();
  vtkTypeMacro(vtkAMRStreamingPriorityQueue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If the controller is specified, the queue can be used in parallel. So long
  // as Initialize(), Update() and Pop() methods are called on all processes
  // (need not be synchronized) and all process get the same amr tree and
  // view_planes (which is generally true with ParaView), the blocks are
  // distributed among the processes.
  // By default, this is set to the
  // vtkMultiProcessController::GetGlobalController();
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Initializes the queue. All information about items in the is lost.
  void Initialize(vtkAMRInformation* amr);

  // Description:
  // Re-initializes the priority queue using the amr structure given to the most
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

protected:
  vtkAMRStreamingPriorityQueue();
  ~vtkAMRStreamingPriorityQueue();

  vtkMultiProcessController* Controller;

private:
  vtkAMRStreamingPriorityQueue(const vtkAMRStreamingPriorityQueue&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAMRStreamingPriorityQueue&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

};

#endif
