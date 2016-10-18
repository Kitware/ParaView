#include "FEDataStructures.h"
#include <mpi.h>

#ifdef USE_CATALYST
#include "FEAdaptor.h"
#include <CPythonAdaptorAPI.h>
#include <stdio.h>
#include <string.h>
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
// into Catalyst. In this example we use
// some of the API in CPythonAdaptorAPI.h
// to assist in setting the problem up.
// CFullExample does essentially the same
// thing but without using the existing
// helper API. Note that through configuration
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
  int fileNameLength = 0;
  if (argc < 2)
  {
    printf("Must pass in a Catalyst script\n");
    MPI_Finalize();
    return 1;
  }
  fileNameLength = strlen(argv[1]);
  coprocessorinitializewithpython(argv[1], &fileNameLength);
  int i;
  for (i = 2; i < argc; i++)
  {
    // Add in any other Python script pipelines that are passed in.
    fileNameLength = strlen(argv[i]);
    coprocessoraddpythonscript(argv[i], &fileNameLength);
  }
#endif
  unsigned int numberOfTimeSteps = 100;
  unsigned int timeStep;
  for (timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    UpdateFields(&attributes, time);
#ifdef USE_CATALYST
    int lastTimeStep = 0;
    if (timeStep == numberOfTimeSteps - 1)
    {
      lastTimeStep = 1;
    }
    CatalystCoProcess(grid.NumberOfPoints, grid.Points, grid.NumberOfCells, grid.Cells,
      attributes.Velocity, attributes.Pressure, time, timeStep, lastTimeStep);
#endif
  }

#ifdef USE_CATALYST
  CatalystFinalize();
  coprocessorfinalize();
#endif
  MPI_Finalize();

  return 0;
}
