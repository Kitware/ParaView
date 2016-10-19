#include "FEDataStructures.h"
#include <iostream>
#include <mpi.h>

#include "FEAdaptor.h"

void SubCommunicatorDriver(int argc, char* argv[], MPI_Comm* handle)
{
  Grid grid;
  unsigned int numPoints[3] = { 70, 60, 44 };
  double spacing[3] = { 1, 1.1, 1.3 };
  grid.Initialize(numPoints, spacing);
  Attributes attributes;
  attributes.Initialize(&grid);

  FEAdaptor::Initialize(argc, argv, handle);
  unsigned int numberOfTimeSteps = 100;
  for (unsigned int timeStep = 0; timeStep < numberOfTimeSteps; timeStep++)
  {
    // use a time step length of 0.1
    double time = timeStep * 0.1;
    attributes.UpdateFields(time);
    FEAdaptor::CoProcess(grid, attributes, time, timeStep, timeStep == numberOfTimeSteps - 1);
  }

  FEAdaptor::Finalize();
}

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  int myrank, numprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  MPI_Group orig_group;
  MPI_Comm_group(MPI_COMM_WORLD, &orig_group);

  std::vector<int> subranks;
  for (int i = 0; i < numprocs / 2; i++)
    subranks.push_back(i);
  if (subranks.empty())
  {
    subranks.push_back(0);
  }
  MPI_Group subgroup;
  MPI_Group_incl(orig_group, subranks.size(), &(subranks[0]), &subgroup);
  MPI_Comm subcommunicator;
  MPI_Comm_create(MPI_COMM_WORLD, subgroup, &subcommunicator);

  if (myrank < static_cast<int>(subranks.size()))
  {
    int newrank;
    MPI_Comm_rank(MPI_COMM_WORLD, &newrank);
    std::cout << "got a rank\n";

    SubCommunicatorDriver(argc, argv, &subcommunicator);
    std::cout << "Process " << myrank << " did some co-processing.\n";
  }
  else
  {
    std::cout << "Process " << myrank << " did not do any co-processing.\n";
  }

  MPI_Finalize();

  return 0;
}
