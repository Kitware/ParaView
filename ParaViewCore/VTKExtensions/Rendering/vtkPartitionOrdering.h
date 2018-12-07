/*=========================================================================

  Program:   ParaView
  Module:    vtkPartitionOrdering.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPartitionOrdering
 * @brief   Build an ordering partitions for compositing.
 *
 *
 *      Build, in parallel, a list of bounding boxes of one or more
 *      vtkDataSets distributed across processors.  We assume each
 *      process has read in one portion of a large distributed data set.
 *      When done, each process has access to the bounding boxes,
 *      can obtain information about which process contains
 *      data for each spatial region, and can depth sort the spatial
 *      regions.
 *
 * @sa
 *      vtkPKdTree
*/

#ifndef vtkPartitionOrdering_h
#define vtkPartitionOrdering_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include <vector>                              // For dynamic data storage

class vtkDataSet;
class vtkIntArray;
class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPartitionOrdering : public vtkObject
{
public:
  vtkTypeMacro(vtkPartitionOrdering, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPartitionOrdering* New();

  //@{
  /**
   * Set/Get the communicator object
   */
  void SetController(vtkMultiProcessController* c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Get the number of processes/regions.
   */
  int GetNumberOfRegions();

  /**
   * Return a list of all processes in order from front to back given a
   * vector direction of projection.  Use this to do visibility sorts
   * in parallel projection mode. `orderedList' will be resized to the number
   * of processes. The return value is the number of processes.
   * \pre orderedList_exists: orderedList!=0
   */
  virtual int ViewOrderAllProcessesInDirection(
    const double directionOfProjection[3], vtkIntArray* orderedList);

  /**
   * Return a list of all processes in order from front to back given a
   * camera position.  Use this to do visibility sorts in perspective
   * projection mode. `orderedList' will be resized to the number
   * of processes. The return value is the number of processes.
   * \pre orderedList_exists: orderedList!=0
   */
  virtual int ViewOrderAllProcessesFromPosition(
    const double cameraPosition[3], vtkIntArray* orderedList);

  //@{
  /**
   * Construct the global bounding boxes based.
   */
  bool Construct(vtkDataSet* grid);
  bool Construct(const double localBounds[6]);
  //@}

protected:
  vtkPartitionOrdering();
  ~vtkPartitionOrdering() override;

private:
  vtkMultiProcessController* Controller;

  std::vector<double> ProcessBounds;
  double GlobalBounds[6];

  vtkPartitionOrdering(const vtkPartitionOrdering&) = delete;
  void operator=(const vtkPartitionOrdering&) = delete;
};

#endif
