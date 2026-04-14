// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEdgeInternal
 * @brief   edge informations for interpolation purpose
 *
 */

#ifndef vtkEdgeInternal_h
#define vtkEdgeInternal_h

#include "vtkABINamespace.h"
#include "vtkType.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;

struct vtkEdgeInternal
{
  vtkIdType Ids[2];
  double Weight;
  vtkEdgeInternal() = default;
  vtkEdgeInternal(double weight, vtkCell* edge);
};

VTK_ABI_NAMESPACE_END

#endif
