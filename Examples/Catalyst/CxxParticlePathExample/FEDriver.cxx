#include "FEDataStructures.h"
#include <mpi.h>
#include <string>
#include <vector>

#ifdef USE_CATALYST
#include "FEAdaptor.h"
#endif

#include <cstring>
#include <stdlib.h>

// Example of a C++ adaptor for a simulation code
// where the simulation code has a fixed topology
// grid. We treat the grid as an unstructured
// grid even though in the example provided it
// would be best described as a vtkImageData.
// Also, the points are stored in an inconsistent
// manner with respect to the velocity vector.
// This is purposefully done to demonstrate
// the different approaches for getting data
// into Catalyst. The simulation can be run
// from a restarted time step with the
// -- restart <time step> command line argument.
// All other arguments are considered to be input
// script. Note that through configuration
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

  // we are doing a restarted simulation
  unsigned int startTimeStep = 0;

  std::vector<std::string> scripts;
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--restart") == 0)
    {
      if (i + 1 < argc)
      {
        startTimeStep = static_cast<unsigned int>(atoi(argv[2]));
        i++;
      }
    }
    else
    {
      scripts.push_back(argv[i]);
    }
  }

#ifdef USE_CATALYST
  FEAdaptor::Initialize(scripts);
#endif
  unsigned int numberOfTimeSteps = 50;
  for (unsigned int timeStep = startTimeStep; timeStep <= startTimeStep + numberOfTimeSteps;
       timeStep++)
  {
    // use a time step length of 0.018
    double time = timeStep * 0.018;
    attributes.UpdateFields(time);
#ifdef USE_CATALYST
    FEAdaptor::CoProcess(
      grid, attributes, time, timeStep, timeStep == numberOfTimeSteps + startTimeStep);
#endif
  }

#ifdef USE_CATALYST
  FEAdaptor::Finalize();
#endif
  MPI_Finalize();

  return 0;
}
