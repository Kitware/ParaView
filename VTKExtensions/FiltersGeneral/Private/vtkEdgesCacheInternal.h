// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEdgesCacheInternal
 * @brief   Cache edges informations for interpolation purpose
 *
 */

#ifndef vtkEdgesCacheInternal_h
#define vtkEdgesCacheInternal_h

#include "vtkABINamespace.h"
#include "vtkEdgeInternal.h"

#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataObject;
class vtkDataSet;
class vtkPolyData;

class vtkEdgesCacheInternal
{
public:
  vtkEdgesCacheInternal() = default;
  ~vtkEdgesCacheInternal() = default;

  /**
   * Invalidate current content.
   */
  void InvalidateCache();

  /**
   * Update PointData attributes data only.
   *
   * Each output point belongs to an input edge: find it and interpolate.
   * On first call, it caches edge information, for faster further interpolation.
   *
   * Expect vtkDataSet as input and vtkPolyData as output, or vtkCompositeDataSet of those.
   */
  bool UpdateAttributes(vtkDataObject* input, vtkDataObject* output);

private:
  /**
   * Update output PointData attributes according to input.
   */
  bool UpdateLeafAttributes(vtkDataSet* input, vtkPolyData* output);

  /**
   * Fill edge cache.
   * Each point of output polydata are on an edge of input dataset.
   * Stores edges information to help interpolate data.
   */
  void CacheLeafEdges(vtkDataSet* input, vtkPolyData* output);

  std::map<vtkDataSet*, std::vector<vtkEdgeInternal>> OriginalEdges;
};

VTK_ABI_NAMESPACE_END

#endif
