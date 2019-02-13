// Test whether Catalyst can be initialized with just a subset of
// the MPI processes.

#include <vtkCPProcessor.h>
#include <vtkMPI.h>
#include <vtkSmartPointer.h>

#include <cassert>
#include <vector>

void SubCommunicatorDriver(MPI_Comm* handle)
{
  vtkSmartPointer<vtkCPProcessor> processor = vtkSmartPointer<vtkCPProcessor>::New();
  vtkMPICommunicatorOpaqueComm comm(handle);
  processor->Initialize(comm);

  processor->Finalize();
}

int SubController(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  int myrank, numprocs;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  if (numprocs < 2)
  {
    cout << "ERROR: Too few ranks. Needs at least 2!" << std::endl;
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  MPI_Group orig_group;
  MPI_Comm_group(MPI_COMM_WORLD, &orig_group);

  std::vector<int> subranks;
  for (int i = 0; i < numprocs / 2; i++)
  {
    subranks.push_back(i);
  }
  assert(subranks.size() >= 1);

  MPI_Group subgroup;
  MPI_Group_incl(orig_group, static_cast<int>(subranks.size()), &(subranks[0]), &subgroup);
  MPI_Comm subcommunicator;
  MPI_Comm_create(MPI_COMM_WORLD, subgroup, &subcommunicator);

  int didCoProcessing = 0;

  if (myrank < static_cast<int>(subranks.size()))
  {
    didCoProcessing = 1;
    SubCommunicatorDriver(&subcommunicator);
    std::cout << "Process " << myrank << " did some co-processing.\n";
  }
  else
  {
    std::cout << "Process " << myrank << " did not do any co-processing.\n";
  }

  int ranksThatDidCoProcessing;
  MPI_Allreduce(&didCoProcessing, &ranksThatDidCoProcessing, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  int retVal = EXIT_SUCCESS;
  if (ranksThatDidCoProcessing != static_cast<int>(subranks.size()))
  {
    vtkGenericWarningMacro("Sum should be 1 but is " << ranksThatDidCoProcessing);
    retVal = EXIT_FAILURE;
  }

  MPI_Finalize();
  return retVal;
}
