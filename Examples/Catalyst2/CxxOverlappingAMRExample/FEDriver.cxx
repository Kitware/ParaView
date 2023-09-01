// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifdef USE_CATALYST
#include "CatalystAdaptor.h"
#endif
#include "FEDataStructures.h"
#include <iostream>
#include <mpi.h>

// Example of a C++ adaptor for a simulation code
// where the simulation code has an overlapping AMR
// grid. The grid in this case is a vtkOverlappingAMR
// data set with the MPI process id specified
// as cell data. Note that in order to see the AMR
// cells (i.e. Surface With Edges representation of
// the data) that the .vtk reader needs to increase
// the `Default Number Of Levels` parameter to
// greater than the default of 1.

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  int numRanks(1), myRank(0);
  MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

  unsigned int numberOfAMRLevels = 5;
  AMR amr(numberOfAMRLevels, myRank, numRanks);

  // The first argument is the program name
#ifdef USE_CATALYST
  CatalystAdaptor::Initialize(argc, argv);
#endif
  // keep the number of time steps small since nothing about the grid or fields is changing
  unsigned int numberOfTimeSteps = 5;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    // the number of AMR levels that each process will generate
    // the grid is one cell "wide" in the X-direction for each MPI process,
    // one cell deep in the Y-direction and numberOfAMRLevels high in the
    // Z-direction, relative to the 0th level cell. Each 0th level cell in the
    // Z-direction is subsequently refined.
#ifdef USE_CATALYST
    CatalystAdaptor::Execute(timeStep, time, amr);
#endif
  }
  CatalystAdaptor::Finalize();

#ifdef USE_CATALYST
  MPI_Finalize();
#endif
  return 0;
}
