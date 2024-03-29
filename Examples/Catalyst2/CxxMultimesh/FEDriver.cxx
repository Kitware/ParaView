// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "CatalystAdaptor.h"
#endif

// Example of a C++ adaptor for a simulation code that
// has two meshes as part of "multimesh"

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
  CatalystAdaptor::Initialize(argc, argv);
#endif
  unsigned int numberOfTimeSteps = 10;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    attributes.UpdateFields(time);
    particles.Advect();
#ifdef USE_CATALYST
    CatalystAdaptor::Execute(timeStep, time, grid, attributes, particles);
#endif
  }

#ifdef USE_CATALYST
  CatalystAdaptor::Finalize();
#endif
  MPI_Finalize();

  return EXIT_SUCCESS;
}
