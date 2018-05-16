#include "FEDataStructures.h"
#include <mpi.h>
#include <vector>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkNew.h>

// Sample C++ simulation code that provides different levels of ghost cells
// for either a vtkUnstructuredGrid of vtkImageData. This example is
// intended to show how to deal with ghost cells when using Catalyst.
// This example is not intended to show how to do a simulation code
// integration when using ghost cells but to be used with testing Catalyst
// pipeline behavior when the simulation code already provides ghost cells.
// Use the -h flag to see the command line options.

void PrintUsage()
{
  int myRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  if (myRank == 0)
  {
    std::cout << "Command line arguments for CxxGhostCellsExample:\n";
    std::cout << "  -s <file name> : the name of a Catalyst Python script to run\n";
    std::cout << "  -l <numlevels> : the number of ghost levels to produce\n";
    std::cout << "                   (optional with a default of 1 ghost level)\n";
    std::cout << "  -i             : produce image data instead of unstructured grids\n";
    std::cout << "  -h             : print this help message\n";
  }
}

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  // process the input flags
  // Add scripts with -s <filename>.
  std::vector<std::string> scripts;
  // use a default of 1 ghost level. use -l <val> to set the number of levels.
  int numGhostLevels = 1;
  // default to using an unstructured grid. use -i to use a cartesian grid.
  bool generateUnstructuredGrid = true;
  for (int i = 1; i < argc; i++)
  {
    if (strcmp("-s", argv[i]) == 0 && i < argc - 1)
    {
      scripts.push_back(argv[i + 1]);
      i++;
    }
    else if (strcmp("-l", argv[i]) == 0 && i < argc - 1)
    {
      int tmp = atoi(argv[i + 1]);
      if (tmp > 0)
      {
        numGhostLevels = tmp;
      }
      else
      {
        cerr << "Bad ghost level of " << tmp << endl;
        PrintUsage();
        MPI_Finalize();
        return 1;
      }
      i++;
    }
    else if (strcmp("-i", argv[i]) == 0)
    {
      generateUnstructuredGrid = false;
    }
    else if (strcmp("-h", argv[i]) == 0)
    {
      PrintUsage();
      MPI_Finalize();
      return 0;
    }
    else
    {
      cerr << "Unknown option " << argv[i] << endl;
      PrintUsage();
      MPI_Finalize();
      return 1;
    }
  }
  if (scripts.empty())
  {
    cerr << "No Catalysts script to process\n";
    PrintUsage();
    MPI_Finalize();
    return 1;
  }

  unsigned int numPoints[3] = { 70, 60, 44 };
  Grid grid(numPoints, !generateUnstructuredGrid, numGhostLevels);

  vtkCPProcessor* processor = vtkCPProcessor::New();
  processor->Initialize();
  for (auto& script : scripts)
  {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(script.c_str());
    processor->AddPipeline(pipeline);
  }
  vtkIdType numberOfTimeSteps = 20;
  for (vtkIdType timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    vtkNew<vtkCPDataDescription> dataDescription;
    dataDescription->SetTimeData(time, timeStep);
    dataDescription->AddInput("input");
    dataDescription->SetForceOutput(timeStep == numberOfTimeSteps - 1);
    if (processor->RequestDataDescription(dataDescription))
    {
      vtkCPInputDataDescription* inputDataDescription =
        dataDescription->GetInputDescriptionByName("input");
      grid.UpdateField(time, inputDataDescription);
      inputDataDescription->SetGrid(grid.GetVTKGrid());
      if (!generateUnstructuredGrid)
      {
        int wholeExtent[6];
        for (int i = 0; i < 3; i++)
        {
          wholeExtent[2 * i] = 0;
          wholeExtent[2 * i + 1] = numPoints[i];
        }
        inputDataDescription->SetWholeExtent(wholeExtent);
      }
      processor->CoProcess(dataDescription);
    }
  }

  processor->Delete();
  processor = nullptr;
  MPI_Finalize();

  return 0;
}
