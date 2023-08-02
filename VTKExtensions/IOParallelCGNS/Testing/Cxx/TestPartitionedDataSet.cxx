// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "mpi.h"
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPCGNSWriter.h"
#include "vtkPVTestUtilities.h"
#include "vtkPartitionedDataSet.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

int TestPartitionedDataSet(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  vtkObject::GlobalWarningDisplayOff();
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 1);

  vtkMultiProcessController::SetGlobalController(mpiController);

  int rank = mpiController->GetCommunicator()->GetLocalProcessId();
  int size = mpiController->GetCommunicator()->GetNumberOfProcesses();

  int result(0);

  vtkNew<vtkPartitionedDataSet> partitioned;
  partitioned->SetNumberOfPartitions(size);
  {
    vtkNew<vtkUnstructuredGrid> ug;
    Create(ug, rank, size);

    partitioned->SetPartition(rank, ug);
    partitioned->GetMetaData(rank)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED");
  }

  vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("partitioned-mpi.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  vtkNew<vtkPCGNSWriter> writer;
  writer->SetInputData(partitioned);
  writer->SetFileName(filename);
  writer->SetController(mpiController);

  result = writer->Write();

  mpiController->Finalize();
  if (result == 1 && rank == 0)
  {
    vtkLogIfF(ERROR, !vtksys::SystemTools::FileExists(filename), "File '%s' not found", filename);
    vtkNew<vtkCGNSReader> reader;
    reader->SetFileName(filename);
    // update information first to get all bases in the information
    reader->UpdateInformation();
    // then enable all bases get both bases (volume, surface) into the output
    reader->EnableAllBases();
    reader->Update();

    unsigned long err = reader->GetErrorCode();
    vtkLogIfF(ERROR, err != 0, "CGNS reading failed.");

    vtkMultiBlockDataSet* output = reader->GetOutput();
    vtkLogIfF(ERROR, nullptr == output, "No CGNS reader output.");
    vtkLogIfF(ERROR, 1 != output->GetNumberOfBlocks(), "Expected 1 base block.");
    {
      vtkMultiBlockDataSet* firstBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
      vtkLogIfF(ERROR, nullptr == firstBlock, "First block is NULL");
      vtkLogIfF(ERROR, 1 != firstBlock->GetNumberOfBlocks(), "Expected 1 zone block.");

      vtkUnstructuredGrid* outputGrid = vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(0));
      vtkLogIfF(ERROR, nullptr == outputGrid, "Read grid is NULL");
      vtkLogIfF(ERROR, std::max(2, size) != outputGrid->GetNumberOfCells(),
        "Expected %d cells, got %lld.", std::max(2, size), outputGrid->GetNumberOfCells());
    }
    result = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
