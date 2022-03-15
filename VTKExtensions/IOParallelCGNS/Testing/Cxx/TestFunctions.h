#ifndef PCGNSWRiterTestFunctions_h
#define PCGNSWRiterTestFunctions_h
class vtkUnstructuredGrid;
class vtkPolyData;

void CreatePartial(vtkUnstructuredGrid* ph, int rank);
void Create(vtkPolyData* pd, int rank, int size);
void CreatePolygonal(vtkPolyData* pd, int rank);
void CreatePolyhedral(vtkUnstructuredGrid* ph, int rank);
void Create(vtkUnstructuredGrid* ug, int rank, int size);

#endif
