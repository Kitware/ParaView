// Test whether Catalyst can be initialized with just a subset of
// the MPI processes.

#include <vtkCPProcessor.h>
#include <vtkMPI.h>
#include <vtkSmartPointer.h>

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

  int output;
  MPI_Allreduce(&didCoProcessing, &output, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  int retVal = 0;
  if (output != 1)
  {
    vtkGenericWarningMacro("Sum should be 1 but is " << output);
    retVal = 1;
  }

  MPI_Finalize();

  return retVal;
}
