#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "CatalystAdaptor.h"
#endif

// Example of a C++ adaptor for a simulation code
// where the simulation code has a fixed topology
// grid. We treat the grid as an unstructured
// grid even though in the example provided it
// would be best described as a vtkImageData.
// Also, the points are stored in an inconsistent
// manner with respect to the velocity vector.
// This is purposefully done to demonstrate
// the different approaches for getting data
// into Catalyst. Note that through configuration
// that the driver can be run without linking
// to Catalyst.

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  Grid grid;
  unsigned int numPoints[3] = { 70, 60, 44 };
  double spacing[3] = { 1, 1.1, 1.3 };
  grid.Initialize(numPoints, spacing);
  Attributes attributes;
  attributes.Initialize(&grid);

#ifdef USE_CATALYST
  CatalystAdaptor::Initialize(argc, argv);
#endif
  unsigned int numberOfTimeSteps = 10;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    attributes.UpdateFields(time);
#ifdef USE_CATALYST
    CatalystAdaptor::Execute(timeStep, time, grid, attributes);
#endif
  }

#ifdef USE_CATALYST
  CatalystAdaptor::Finalize();
#endif
  MPI_Finalize();
  return EXIT_SUCCESS;
}
