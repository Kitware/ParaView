/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingPriorityQueue

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStreamingPriorityQueue
 * @brief   provides a datastructure for building
 * priority queues.
 *
 * vtkStreamingPriorityQueue provides a data-structure for building priority
 * queue for steraming based on block bounds. This used by
 * vtkAMRStreamingPriorityQueue.
*/

#ifndef vtkStreamingPriorityQueue_h
#define vtkStreamingPriorityQueue_h
#ifndef VTK_WRAPPING_CXX

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkWrappingHints.h"

#include <algorithm>
#include <queue>

//*****************************************************************************
namespace
{
// this code is stolen from vtkFrustumCoverageCuller.
double vtkComputeScreenCoverage(const double planes[24], const double bounds[6], double& distance,
  double& centeredness, double& itemCoverage)
{
  distance = 0.0;
  centeredness = 0.0;
  itemCoverage = 0.0;

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
  double radius = 0.5 * sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
                          (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
                          (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
  for (int i = 0; i < 6; i++)
  {
    // Compute how far the center of the sphere is from this plane
    double d = planes[i * 4 + 0] * center[0] + planes[i * 4 + 1] * center[1] +
      planes[i * 4 + 2] * center[2] + planes[i * 4 + 3];
    // If d < -radius the prop is not within the view frustum
    if (d < -radius)
    {
      return 0.0;
    }

    // The first four planes are the ones bounding the edges of the
    // view plane (the last two are the near and far planes) The
    // distance from the edge of the sphere to these planes is stored
    // to compute coverage.
    if (i < 4)
    {
      screen_bounds[i] = d - radius;
    }
    // The fifth plane is the near plane - use the distance to
    // the center (d) as the value to sort by
    if (i == 4)
    {
      distance = d;
    }
  }

  // If the prop wasn't culled during the plane tests...
  // Compute the width and height of this slice through the
  // view frustum that contains the center of the sphere
  double full_w = screen_bounds[0] + screen_bounds[1] + 2.0 * radius;
  double full_h = screen_bounds[2] + screen_bounds[3] + 2.0 * radius;

  // Multiply the distances from each side to get a measure of how centered
  // the sphere is on the screen (divide by half the length in that dimension
  // squared to get a measure of how centered the object is in that dimension).
  double measure_w =
    (screen_bounds[0] + radius) * (screen_bounds[1] + radius) / (full_w * full_w / 4.0);
  double measure_h =
    (screen_bounds[2] + radius) * (screen_bounds[3] + radius) / (full_h * full_h / 4.0);

  // If the object is off the edge of the screen, treat it as if it is just
  // inside the edge (which we know it is since it wasn't culled)
  if (measure_w < 0.01)
  {
    measure_w = 0.01;
  }
  if (measure_h < 0.01)
  {
    measure_h = 0.01;
  }
  centeredness = measure_w * measure_h;

  double w = 2 * radius;
  double h = 2 * radius;
  if (screen_bounds[0] < 0.0)
  {
    w += screen_bounds[0];
  }
  if (screen_bounds[1] < 0.0)
  {
    w += screen_bounds[1];
  }
  if (screen_bounds[2] < 0.0)
  {
    h += screen_bounds[2];
  }
  if (screen_bounds[3] < 0.0)
  {
    h += screen_bounds[3];
  }
  itemCoverage = h * w / (4 * radius * radius);

  // Subtract from the full width to get the width of the square
  // enclosing the circle slice from the sphere in the plane
  // through the center of the sphere. If the screen bounds for
  // the left and right planes (0,1) are greater than zero, then
  // the edge of the sphere was a positive distance away from the
  // plane, so there is a gap between the edge of the plane and
  // the edge of the box.
  double part_w = full_w;
  if (screen_bounds[0] > 0.0)
  {
    part_w -= screen_bounds[0];
  }
  if (screen_bounds[1] > 0.0)
  {
    part_w -= screen_bounds[1];
  }
  // Do the same thing for the height with the top and bottom
  // planes (2,3).
  double part_h = full_h;
  if (screen_bounds[2] > 0.0)
  {
    part_h -= screen_bounds[2];
  }
  if (screen_bounds[3] > 0.0)
  {
    part_h -= screen_bounds[3];
  }

  // Compute the fraction of coverage
  if ((full_w * full_h) != 0.0)
  {
    return (part_w * part_h) / (full_w * full_h);
  }

  return 0;
}
}

class VTK_WRAPEXCLUDE vtkStreamingPriorityQueueItem
{
public:
  unsigned int Identifier; // this is used to identify this block when making a
                           // request.
  double Refinement;       // Where lower the Refinement cheaper is the
                           // processing for this block. 0 is considered as
                           // undefined.
  double ScreenCoverage;   // computed coverage for the block.
  double Centeredness; // how centered the object is on the screen (1 is centered, 0.0001 is near
                       // the edge)
  double Priority;     // Computed priority for this block.
  double Distance;
  double AmountOfDetail;
  double ItemCoverage;   // amount of the item that is onscreen (fraction, if whole item is onscreen
                         // it is 1)
  vtkBoundingBox Bounds; // Bounds for the block.

  vtkStreamingPriorityQueueItem()
    : Identifier(0)
    , Refinement(0)
    , ScreenCoverage(0)
    , Priority(0)
    , Distance(0)
    , AmountOfDetail(-1)
  {
  }
};

class VTK_WRAPEXCLUDE vtkStreamingPriorityQueueItemComparator
{
public:
  bool operator()(
    const vtkStreamingPriorityQueueItem& me, const vtkStreamingPriorityQueueItem& other) const
  {
    return me.Priority < other.Priority;
  }
};

template <typename Comparator = vtkStreamingPriorityQueueItemComparator>
class VTK_WRAPEXCLUDE vtkStreamingPriorityQueue
  : public std::priority_queue<vtkStreamingPriorityQueueItem,
      std::vector<vtkStreamingPriorityQueueItem>, Comparator>
{
public:
  //@{
  /**
   * Updates the priorities of items in the queue.
   */
  void UpdatePriorities(const double view_planes[24], const double clamp_bounds[6])
  {
    bool clamp_bounds_initialized =
      (vtkMath::AreBoundsInitialized(const_cast<double*>(clamp_bounds)) != 0);
    vtkBoundingBox clampBox(const_cast<double*>(clamp_bounds));
    //@}

    vtkStreamingPriorityQueue current_queue;
    std::swap(current_queue, *this);

    for (; !current_queue.empty(); current_queue.pop())
    {
      vtkStreamingPriorityQueueItem item = current_queue.top();
      if (!item.Bounds.IsValid())
      {
        continue;
      }

      double block_bounds[6];
      item.Bounds.GetBounds(block_bounds);

      if (clamp_bounds_initialized)
      {
        if (!clampBox.ContainsPoint(block_bounds[0], block_bounds[2], block_bounds[4]) &&
          !clampBox.ContainsPoint(block_bounds[1], block_bounds[3], block_bounds[5]))
        {
          // if the block_bounds is totally outside the clamp_bounds, skip it.
          continue;
        }
      }

      double refinement2 = item.Refinement * item.Refinement;
      double distance, centeredness, itemCoverage;
      double coverage =
        vtkComputeScreenCoverage(view_planes, block_bounds, distance, centeredness, itemCoverage);
      item.ScreenCoverage = coverage;
      item.Distance = distance;
      item.Centeredness = centeredness;
      item.ItemCoverage = itemCoverage;

      if (coverage > 0)
      {
        //        item.Priority =  coverage / (item.Refinement/* * distance*/) ;// / distance;
        //        //coverage * coverage / ( 1 + refinement2 + distance);
        item.Priority = coverage * coverage * centeredness / (1 + refinement2 + distance);
      }
      else
      {
        item.Priority = 0;
      }
      this->push(item);
    }
  }
};
#endif
#endif

// VTK-HeaderTest-Exclude: vtkStreamingPriorityQueue.h
