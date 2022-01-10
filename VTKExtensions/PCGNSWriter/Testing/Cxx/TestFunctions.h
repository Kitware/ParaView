#ifndef PCGNSWRiterTestFunctions_h
#define PCGNSWRiterTestFunctions_h
class vtkUnstructuredGrid;
class vtkPolyData;

void CreatePartial(vtkUnstructuredGrid* ph, int rank);
void Create(vtkPolyData* pd, int rank, int size);
void CreatePolygonal(vtkPolyData* pd, int rank);
void CreatePolyhedral(vtkUnstructuredGrid* ph, int rank);
void Create(vtkUnstructuredGrid* ug, int rank, int size);

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;               \
    return EXIT_FAILURE;                                                                           \
  }

#define vtk_assert2(expected, found)                                                               \
  if (expected != found)                                                                           \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " Found (" << found << ") does not match expected ("         \
         << expected << ")";                                                                       \
    cerr << "Expected expression: " << #expected << ": Found expression: " << #found << "."        \
         << endl;                                                                                  \
    return EXIT_FAILURE;                                                                           \
  }

#endif
