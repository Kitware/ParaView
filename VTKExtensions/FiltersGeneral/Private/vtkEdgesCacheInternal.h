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

#include "vtkNew.h" // for vtkNew

#include <map>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkDataObject;
class vtkDataSet;
class vtkIdList;
class vtkPointSet;

class vtkEdgesCacheInternal
{
public:
  vtkEdgesCacheInternal();
  vtkEdgesCacheInternal(const std::string& pointFlagArrayName, double pointFlag);
  ~vtkEdgesCacheInternal();

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
  bool UpdateLeafAttributes(vtkDataSet* input, vtkPointSet* output);

  /**
   * Fill edge cache.
   * Each point of output polydata are on an edge of input dataset.
   * Stores edges information to help interpolate data.
   */
  void CacheLeafEdges(vtkDataSet* input, vtkPointSet* output);

  std::map<vtkDataSet*, std::vector<vtkEdgeInternal>> OriginalEdges;
  struct IdMap
  {
    vtkNew<vtkIdList> Source;
    vtkNew<vtkIdList> Destination;
  };
  std::map<vtkDataSet*, IdMap> OriginalPoints;

  // Used to determine if a point is a copy from an input point.
  std::string PointFlagArray;
  double InputPointFlag;
};

VTK_ABI_NAMESPACE_END

#endif
