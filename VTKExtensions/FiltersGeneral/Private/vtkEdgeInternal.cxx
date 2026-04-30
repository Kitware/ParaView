// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkEdgeInternal.h"

#include "vtkCell.h"

vtkEdgeInternal::vtkEdgeInternal(vtkIdType outId, double weight, vtkCell* edge)
  : OutId(outId)
  , Parametric(weight)
{
  this->Ids[0] = edge->GetPointId(0);
  this->Ids[1] = edge->GetPointId(1);
}
