// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMaterialInterfaceProcessRing
 *
 * Data structure used to distribute work amongst available
 * processes. The buffer can be initialized from a process
 * priority queue such that only those processes with loading
 * less than a specified tolerance are included.
 */

#ifndef vtkMaterialInterfaceProcessRing_h
#define vtkMaterialInterfaceProcessRing_h

#include "vtkMaterialInterfaceProcessLoading.h"
#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports
#include <vector>                                             // needed for Initialize()

class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfaceProcessRing
{
public:
  vtkMaterialInterfaceProcessRing();
  ~vtkMaterialInterfaceProcessRing();
  /**
   * Return the object to an empty state.
   */
  void Clear();
  /**
   * Size buffer and point to first element.
   */
  void Initialize(int nProcs);
  /**
   * Build from a process loading from a sorted
   * vector of process loading items.
   */
  void Initialize(std::vector<vtkMaterialInterfaceProcessLoading>& Q, vtkIdType upperLoadingBound);
  /**
   * Get the next process id from the ring.
   */
  vtkIdType GetNextId();
  /**
   * Print the state of the ring.
   */
  void Print();

private:
  vtkIdType NextElement;
  vtkIdType BufferSize;
  class BufferContainer;
  BufferContainer* Buffer;
};
#endif
