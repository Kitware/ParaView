#include <algorithm>
#include <iostream>
#include <vector>
#include "FEAdaptor.h"

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
  FEAdaptor::Initialize(argc, argv);
  unsigned int numberOfTimeSteps = 100;
  for(unsigned int timeStep=0;timeStep<numberOfTimeSteps;timeStep++)
    {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    FEAdaptor::CoProcess(time, timeStep, timeStep == numberOfTimeSteps-1);
    }
  FEAdaptor::Finalize();

  return 0;
}

