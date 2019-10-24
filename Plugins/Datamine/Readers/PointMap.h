// Wrapper class for a stl map

#ifndef vtkDataMinePointMap_h
#define vtkDataMinePointMap_h

#include <map>
class PointMap
{
public:
  PointMap();
  ~PointMap();
  void SetID(int oldID, int newId);
  int GetID(int oldID);

protected:
  std::map<int, int> map;
};
#endif
