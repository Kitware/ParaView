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
#include "vtkAMRStreamingPriorityQueue.h"

#include "vtkAMRInformation.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <assert.h>
#include <queue>
#include <vector>

//*****************************************************************************
namespace
{
  // this code is stolen from vtkFrustumCoverageCuller.
  double vtkComputeScreenCoverage(const double planes[24],
    const double bounds[6], double &distance)
    {
    distance = 0.0;

    // a duff dataset like a polydata with no cells will have bad bounds
    if (!vtkMath::AreBoundsInitialized(const_cast<double*>(bounds)))
      {
      return 0.0;
      }
    double screen_bounds[4];

    double center[3];
    center[0] = (bounds[0] + bounds[1]) / 2.0;
    center[1] = (bounds[2] + bounds[3]) / 2.0;
    center[2] = (bounds[4] + bounds[5]) / 2.0;
    double radius = 0.5 * sqrt(
      ( bounds[1] - bounds[0] ) * ( bounds[1] - bounds[0] ) +
      ( bounds[3] - bounds[2] ) * ( bounds[3] - bounds[2] ) +
      ( bounds[5] - bounds[4] ) * ( bounds[5] - bounds[4] ) );
    for (int i = 0; i < 6; i++ )
      {
      // Compute how far the center of the sphere is from this plane
      double d = planes[i*4 + 0] * center[0] +
        planes[i*4 + 1] * center[1] +
        planes[i*4 + 2] * center[2] +
        planes[i*4 + 3];
      // If d < -radius the prop is not within the view frustum
      if ( d < -radius )
        {
        return 0.0;
        }

      // The first four planes are the ones bounding the edges of the
      // view plane (the last two are the near and far planes) The
      // distance from the edge of the sphere to these planes is stored
      // to compute coverage.
      if ( i < 4 )
        {
        screen_bounds[i] = d - radius;
        }
      // The fifth plane is the near plane - use the distance to
      // the center (d) as the value to sort by
      if ( i == 4 )
        {
        distance = d;
        }
      }

    // If the prop wasn't culled during the plane tests...
    // Compute the width and height of this slice through the
    // view frustum that contains the center of the sphere
    double full_w = screen_bounds[0] + screen_bounds[1] + 2.0 * radius;
    double full_h = screen_bounds[2] + screen_bounds[3] + 2.0 * radius;
    // Subtract from the full width to get the width of the square
    // enclosing the circle slice from the sphere in the plane
    // through the center of the sphere. If the screen bounds for
    // the left and right planes (0,1) are greater than zero, then
    // the edge of the sphere was a positive distance away from the
    // plane, so there is a gap between the edge of the plane and
    // the edge of the box.
    double part_w = full_w;
    if ( screen_bounds[0] > 0.0 )
      {
      part_w -= screen_bounds[0];
      }
    if ( screen_bounds[1] > 0.0 )
      {
      part_w -= screen_bounds[1];
      }
    // Do the same thing for the height with the top and bottom
    // planes (2,3).
    double part_h = full_h;
    if ( screen_bounds[2] > 0.0 )
      {
      part_h -= screen_bounds[2];
      }
    if ( screen_bounds[3] > 0.0 )
      {
      part_h -= screen_bounds[3];
      }

    // Compute the fraction of coverage
    if ((full_w * full_h)!=0.0)
      {
      return (part_w * part_h) / (full_w * full_h);
      }

    return 0;
    }
}

class vtkAMRStreamingPriorityQueue::vtkInternals
{
public:
  class vtkPriorityQueueItem
    {
  public:
    unsigned int BlockId; 
    double Priority;

    vtkPriorityQueueItem() : BlockId(0), Priority(0)
    {
    }

    bool operator < (const vtkPriorityQueueItem& other) const
      {
      return this->Priority < other.Priority;
      }
    };

  typedef std::priority_queue<vtkPriorityQueueItem> PriorityQueueType;
  PriorityQueueType PriorityQueue;

  vtkSmartPointer<vtkAMRInformation> AMRMetadata;
};

vtkStandardNewMacro(vtkAMRStreamingPriorityQueue);
vtkCxxSetObjectMacro(vtkAMRStreamingPriorityQueue, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkAMRStreamingPriorityQueue::vtkAMRStreamingPriorityQueue()
{
  this->Internals = new vtkInternals();
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkAMRStreamingPriorityQueue::~vtkAMRStreamingPriorityQueue()
{
  delete this->Internals;
  this->Internals = 0;
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::Initialize(vtkAMRInformation* amr)
{
  delete this->Internals;
  this->Internals = new vtkInternals();
  this->Internals->AMRMetadata = amr;

  for (unsigned int cc=0; cc < amr->GetTotalNumberOfBlocks(); cc++)
    {
    vtkInternals::vtkPriorityQueueItem item;
    item.BlockId = cc;
    item.Priority = (amr->GetTotalNumberOfBlocks() - cc);
      // default priority is to prefer lower levels. Thus even without
      // view-planes we have reasonable priority.
    this->Internals->PriorityQueue.push(item);
    }
}

//----------------------------------------------------------------------------
bool vtkAMRStreamingPriorityQueue::IsEmpty()
{
  return this->Internals->PriorityQueue.empty();
}

//----------------------------------------------------------------------------
unsigned int vtkAMRStreamingPriorityQueue::Pop()
{
  if (this->IsEmpty())
    {
    vtkErrorMacro("Queue is empty!");
    return 0;
    }

  int num_procs = this->Controller? this->Controller->GetNumberOfProcesses() : 1;
  int myid = this->Controller? this->Controller->GetLocalProcessId() : 0;
  assert(myid < num_procs);

  std::vector<vtkInternals::vtkPriorityQueueItem> items;
  items.resize(num_procs);
  for (int cc=0; cc < num_procs && !this->Internals->PriorityQueue.empty(); cc++)
    {
    items[cc] = this->Internals->PriorityQueue.top();
    this->Internals->PriorityQueue.pop();
    }

  // at the end, when the queue empties out in the middle of a pop, right now,
  // all other processes are simply going to ask for block 0 (set in
  // initialization of vtkPriorityQueueItem). We can change that, if needed.
  return items[myid].BlockId;
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::Update(const double view_planes[24])
{
  if (!this->Internals->AMRMetadata)
    {
    return;
    }

  vtkInternals::PriorityQueueType current_queue;
  std::swap(current_queue, this->Internals->PriorityQueue);

  for (;!current_queue.empty(); current_queue.pop())
    {
    vtkInternals::vtkPriorityQueueItem item = current_queue.top();
    unsigned int level=0, index=0;
    this->Internals->AMRMetadata->ComputeIndexPair(
      item.BlockId, level, index);

    double block_bounds[6];
    this->Internals->AMRMetadata->GetBounds(level, index, block_bounds);

    double center[3];
    center[0] = (block_bounds[0] + block_bounds[1]) / 2.0;
    center[1] = (block_bounds[2] + block_bounds[3]) / 2.0;
    center[2] = (block_bounds[4] + block_bounds[5]) / 2.0;

    double distance;
    double coverage = vtkComputeScreenCoverage(
      view_planes, block_bounds, distance);
    item.Priority =  coverage * coverage /
      ( 1 + level * level + distance);

    this->Internals->PriorityQueue.push(item);
    }
}

//----------------------------------------------------------------------------
void vtkAMRStreamingPriorityQueue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
