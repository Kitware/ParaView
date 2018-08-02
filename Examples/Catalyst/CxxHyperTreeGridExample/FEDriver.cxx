#include <mpi.h>

#include "FEAdaptor.h"

// Simplified Catalyst example that produces a hypertree
// grid. It has one tree per MPI process. It is not
// based on any input type of dataset though.

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  // The first argument is the program name
  FEAdaptor feaAdaptor(argc - 1, argv + 1);
  unsigned int numberOfTimeSteps = 100;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    feaAdaptor.CoProcess(time, timeStep, timeStep == numberOfTimeSteps - 1);
  }

  feaAdaptor.Finalize();
  MPI_Finalize();

  return 0;
}
