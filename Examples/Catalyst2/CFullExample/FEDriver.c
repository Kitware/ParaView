#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "CatalystAdaptor.h"
#endif

// Example of a C adaptor for a simulation code
// where the simulation code has a fixed topology
// grid. We treat the grid as an unstructured
// grid even though in the example provided it
// would be best described as a vtkImageData.
// Also, the points are stored in an inconsistent
// manner with respect to the velocity vector.
// This is purposefully done to demonstrate
// the different approaches for getting data
// into Catalyst. In this example we don't
// use any of the Fortran/C API provided in
// Catalyst. That is in CFullExample2.
// Note that through configuration
// that the driver can be run without linking
// to Catalyst.

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  Grid grid = (Grid){.NumberOfPoints = 0, .Points = 0, .NumberOfCells = 0, .Cells = 0 };
  unsigned int numPoints[3] = { 70, 60, 44 };
  double spacing[3] = { 1, 1.1, 1.3 };
  InitializeGrid(&grid, numPoints, spacing);
  Attributes attributes;
  InitializeAttributes(&attributes, &grid);

#ifdef USE_CATALYST
  do_catalyst_initialization(argc, argv);
#endif
  unsigned int numberOfTimeSteps = 2;
  unsigned int timeStep;
  for (timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    UpdateFields(&attributes, time);

#ifdef USE_CATALYST
    do_catalyt_execute(timeStep, time, &grid, &attributes);
#endif
  }

#ifdef USE_CATALYST
  do_catalyt_finalization();
#endif

  MPI_Finalize();
  return 0;
}
