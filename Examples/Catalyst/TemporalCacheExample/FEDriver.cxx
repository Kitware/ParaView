/**
An example catalyst pipeline that includes temporal processing.
The "simulation" has a number of particles bouncing around in a cube.
Unlike other Catalyst examples in this caseCatalyst maintains a configurably sized cache of the most
recent timesteps worth of produced data. The cache is suitable
for ParaView's time varying filters and for ex post facto / backtracking
analysis. Here the pipeline fully flows only when some
interesting event happens. When it does, the pipelne has access
to more than just the current timestep to perform analysis on.

The command line arguments are:

-DIMS K J I // size of the volume, defaut is 70, 60, 44
-TSTEPS t //number of simulation timesteps, default is 30
-NUMPARTICLES p // number of particles in the box
-CACHESIZE c // number of timesteps to cache
-HOME // When ParaView is compiles with PARAVIEW_USE_MEMKIND, a directory to memory map the cache
in, ideal Optane with -o dax
-ENABLECXXPIPELINE // runs the default C++ Catalyst Pipeline in addition to provided python ones
PythonPipeline... //One or more Catalyst Python Pipelines
*/

#ifdef USE_CATALYST
#include "FEAdaptor.h"
#endif
#include "FEDataStructures.h"

#include <mpi.h>

#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  unsigned int numPoints[3] = { 70, 60, 44 };
  unsigned int numberOfTimeSteps = 20;
  unsigned int delay = 1000;
  int numparticles = 2;
  int ac = 0;
  char** av = new char*[argc];
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-DIMS"))
    {
      for (int j = 0; j < 3; j++)
      {
        numPoints[j] = atoi(argv[i + j + 1]);
      }
      i += 3;
    }
    else if (!strcmp(argv[i], "-TSTEPS"))
    {
      numberOfTimeSteps = atoi(argv[i + 1]);
      i += 1;
    }
    else if (!strcmp(argv[i], "-DELAY"))
    {
      delay = atoi(argv[i + 1]);
      i += 1;
    }
    else if (!strcmp(argv[i], "-NUMPARTICLES"))
    {
      numparticles = atoi(argv[i + 1]);
      i += 1;
    }
    else
    {
      // pass unmatched arguments through for FEAdaptor to use
      av[ac] = argv[i];
      ac++;
    }
  }

  double spacing[3] = { 1, 1.1, 1.3 };

  Grid grid;
  grid.Initialize(numPoints, spacing);

  Attributes attributes(numparticles);
  attributes.Initialize(&grid);

#ifdef USE_CATALYST
  // The first argument is the program name
  FEAdaptor::Initialize(ac - 1, av + 1);
#endif

  double tsim = 0.0;
#ifdef USE_CATALYST
  double tcop = 0.0;
#endif
  auto tlog = vtkSmartPointer<vtkTimerLog>::New();
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    cout << "timeStep " << timeStep << endl;
    double time = timeStep * 0.1;
    tlog->StartTimer();
    attributes.UpdateFields(time);
    tlog->StopTimer();
    tsim += tlog->GetElapsedTime();
#ifdef USE_CATALYST
    tlog->StartTimer();
    FEAdaptor::CoProcess(grid, attributes, time, timeStep, timeStep == numberOfTimeSteps - 1);
    tlog->StopTimer();
    tcop += tlog->GetElapsedTime();
#endif
  }

  cout << "Elapsed Simulation time " << tsim << endl;
#ifdef USE_CATALYST
  cout << "Elapsed CoProcessing time " << tcop << endl;
  FEAdaptor::Finalize();
#endif
  MPI_Finalize();

  delete[] av;
  return 0;
}
