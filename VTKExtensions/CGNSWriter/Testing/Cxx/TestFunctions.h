
#ifndef TestFunctions_h
#define TestFunctions_h
class vtkUnstructuredGrid;
class vtkStructuredGrid;
class vtkPolyData;
class vtkMultiBlockDataSet;

void Create(vtkUnstructuredGrid*, int);
void Create(vtkStructuredGrid*, int, int, int);
void Create(vtkPolyData*);
void CreatePolyhedral(vtkUnstructuredGrid*);

int PolydataTest(vtkMultiBlockDataSet*, unsigned int, unsigned int);
int UnstructuredGridTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int);
int StructuredGridTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int, int, int);
int MappedUnstructuredGridTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int);
int PolyhedralTest(vtkMultiBlockDataSet*, unsigned int, unsigned int);

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;               \
    return EXIT_FAILURE;                                                                           \
  }

#define vtk_assert_equal(expected, actual)                                                         \
  if (expected != actual)                                                                          \
  {                                                                                                \
    cerr << "ERROR: Expected: " << expected << " got: " << actual;                                 \
    cerr << "On line " << __LINE__ << endl;                                                        \
    return EXIT_FAILURE;                                                                           \
  }

#endif
