#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "FEAdaptor.h"
#endif

// Example of a C++ adaptor for a simulation code that
// has two channels/inputs of information for different
// sets of simulation pieces. The first channel is
// for volumetric grid information and the second channel
// is for particles information. Note that through configuration
// that the driver can be run without linking
// to Catalyst.

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  unsigned int numPoints[3] = { 70, 60, 44 };
  double spacing[3] = { 1, 1.1, 1.3 };
  Grid grid(numPoints, spacing);
  Attributes attributes;
  attributes.Initialize(&grid);

  size_t numParticlesPerProcess = 20;
  Particles particles(grid, numParticlesPerProcess);

#ifdef USE_CATALYST
  // The first argument is the program name
  FEAdaptor feaAdaptor(argc - 1, argv + 1);
#endif
  unsigned int numberOfTimeSteps = 10;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    attributes.UpdateFields(time);
    particles.Advect();
#ifdef USE_CATALYST
    feaAdaptor.CoProcess(
      grid, attributes, particles, time, timeStep, timeStep == numberOfTimeSteps - 1);
#endif
  }

#ifdef USE_CATALYST
  feaAdaptor.Finalize();
#endif
  MPI_Finalize();

  return 0;
}
