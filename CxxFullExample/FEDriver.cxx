#include <algorithm>
#include "FEAdaptor.h"
#include "FEDataStructures.h"
#include <iostream>
#include <mpi.h>
#include <vector>



int main(int argc, char* argv[0])
{
  MPI_Init(&argc, &argv);
  Grid grid;
  unsigned int numPoints[3] = {5, 6, 7};
  double spacing[3] = {1, 1.1, 1.3};
  grid.Initialize(numPoints, spacing);
  Attributes attributes;
  attributes.Initialize(&grid);

  Catalyst::Initialize(argc, argv);

  for(unsigned int timeStep=0;timeStep<100;timeStep++)
    {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    attributes.UpdateFields(time);
    Catalyst:CoProcess(grid, attributes, time, timeStep);
    }

  Catalyst::Finalize();
  MPI_Finalize();

  return 0;
}

