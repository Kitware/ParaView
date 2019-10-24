// wrapper for stl map class
// it is used to keep the id reference for points from a datamine file
// to the point ids used in vtkPointData

#include "PointMap.h"

// --------------------------------------
PointMap::PointMap()
{
}

// --------------------------------------
PointMap::~PointMap()
{
}

// --------------------------------------
int PointMap::GetID(int oldID)
{
  auto it = this->map.find(oldID);
  if (it != this->map.end())
  {
    // id is located
    return it->second;
  }
  else
  {
    // return a bad ID
    return -1;
  }
}

// --------------------------------------
void PointMap::SetID(int oldID, int newId)
{
  this->map[oldID] = newId;
}
