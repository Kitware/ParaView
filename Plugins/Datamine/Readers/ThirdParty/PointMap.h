// SPDX-FileCopyrightText: Copyright 2000 Jeremy Maccelari
// SPDX-License-Identifier: BSD-3-Clause
// Wrapper class for a stl map

#ifndef vtkDataMinePointMap_h
#define vtkDataMinePointMap_h

#include "vtkType.h"
#include <vector>
class PointMap
{
public:
  PointMap(vtkIdType numPoints);
  ~PointMap();
  void SetID(vtkIdType oldID, vtkIdType newId);
  vtkIdType GetID(vtkIdType oldID);

protected:
  std::vector<vtkIdType> Map;
};
#endif
