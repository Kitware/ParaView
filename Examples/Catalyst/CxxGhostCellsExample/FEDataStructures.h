#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

class vtkCPInputDataDescription;

class Grid
{
public:
  Grid(const unsigned int numberOfPoints[3], const bool outputImageData,
    const int numberOfGhostLevels);

  void UpdateField(double time, vtkCPInputDataDescription* inputDataDescription);
  vtkDataSet* GetVTKGrid();

private:
  void CreateImageData(int extent[6]);
  void CreateUnstructuredGrid(int extent[6]);
  vtkSmartPointer<vtkDataSet> VTKGrid;
  int WholeExtent[6];
};
#endif
