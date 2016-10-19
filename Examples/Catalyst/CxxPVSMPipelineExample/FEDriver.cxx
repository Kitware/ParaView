#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "FEAdaptor.h"
#include <cstdlib>
#include <iostream>
#endif

// Example of a C++ adaptor for a simulation code
// where we use a hard-coded ParaView server-manager
// C++ pipeline. The simulation code has a fixed topology
// grid. We treat the grid as an unstructured
// grid even though in the example provided it
// would be best described as a vtkImageData.
// Also, the points are stored in an inconsistent
// manner with respect to the velocity vector.
// This is purposefully done to demonstrate
// the different approaches for getting data
// into Catalyst. The hard-coded C++ pipeline
// uses a slice filter to cut 4 planes through
// the domain.
// Note that through configuration
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
  bool doCoProcessing = false;
  if (argc == 3)
  {
    doCoProcessing = true;
    // pass in the number of time steps and base file name.
    FEAdaptor::Initialize(atoi(argv[1]), argv[2]);
  }
  else
  {
    std::cerr
      << "To run with Catalyst you must pass in the output frequency and the base file name.\n";
  }
#endif
  unsigned int numberOfTimeSteps = 15;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    attributes.UpdateFields(time);
#ifdef USE_CATALYST
    if (doCoProcessing)
    {
      FEAdaptor::CoProcess(grid, attributes, time, timeStep, timeStep == numberOfTimeSteps - 1);
    }
#endif
  }

#ifdef USE_CATALYST
  if (doCoProcessing)
  {
    FEAdaptor::Finalize();
  }
#endif
  MPI_Finalize();

  return 0;
}
