// it is used to keep the id reference for points from a datamine file
// to the point ids used in vtkPointData

#include "PointMap.h"
#include <stdio.h>

// --------------------------------------
PointMap::PointMap(vtkIdType numPoints)
{
  // we add 1 as datamine tends to be one referenced
  this->Map.resize(numPoints + 1);
}

// --------------------------------------
PointMap::~PointMap() = default;

// --------------------------------------
vtkIdType PointMap::GetID(vtkIdType oldID)
{
  if (oldID < 0)
  {
    return -1;
  }

  if (static_cast<size_t>(oldID) >= this->Map.size())
  {
    return -1;
  }

  return this->Map[oldID];
}

// --------------------------------------
void PointMap::SetID(vtkIdType oldID, vtkIdType newId)
{
  if (oldID < 0)
  {
    return;
  }

  // ideally this should never happen
  if (static_cast<size_t>(oldID) >= this->Map.size())
  {
    this->Map.resize(static_cast<size_t>(this->Map.size() * 1.2));
  }

  this->Map[oldID] = newId;
}
