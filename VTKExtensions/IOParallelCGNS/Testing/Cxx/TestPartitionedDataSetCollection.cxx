// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "mpi.h"
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkCommunicator.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPCGNSWriter.h"
#include "vtkPVTestUtilities.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

extern int TestPartitionedDataSetCollection(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  vtkObject::GlobalWarningDisplayOff();
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 1);

  vtkMultiProcessController::SetGlobalController(mpiController);

  int rank = mpiController->GetCommunicator()->GetLocalProcessId();
  int size = mpiController->GetCommunicator()->GetNumberOfProcesses();

  int result(0);

  vtkNew<vtkPartitionedDataSetCollection> partitionedCollection;
  partitionedCollection->SetNumberOfPartitionedDataSets(2);
  partitionedCollection->SetNumberOfPartitions(0u, size);
  partitionedCollection->SetNumberOfPartitions(1u, size);
  {
    vtkNew<vtkUnstructuredGrid> ug;
    Create(ug, rank, size);
    vtkNew<vtkPolyData> pd;
    Create(pd, rank, size);

    partitionedCollection->SetPartition(0u, rank, ug);
    partitionedCollection->SetPartition(1u, rank, pd);
    partitionedCollection->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED");
    partitionedCollection->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "POLYDATA");
  }

  vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("partitioned-collection-mpi.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  vtkNew<vtkPCGNSWriter> writer;
  writer->SetInputData(partitionedCollection);
  writer->SetFileName(filename);
  writer->SetController(mpiController);

  result = writer->Write();

  mpiController->Finalize();
  if (result == 1 && rank == 0)
  {
    vtkLogIfF(ERROR, !vtksys::SystemTools::FileExists(filename), "File %s not found", filename);
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

    vtkLogIfF(ERROR, 2 != output->GetNumberOfBlocks(),
      "Expected 2 bases in the CGNS output, got %d.", output->GetNumberOfBlocks());
    {
      vtkMultiBlockDataSet* firstBase = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
      vtkLogIfF(ERROR, nullptr == firstBase, "NULL in output.");

      vtkLogIfF(ERROR, !firstBase->HasMetaData(0u), "No metadata on first base");
      vtkLogIfF(ERROR, !firstBase->GetMetaData(0u)->Has(vtkCompositeDataSet::NAME()),
        "No name on first base");
      const std::string zoneName = firstBase->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
      vtkLogIfF(ERROR, zoneName != "UNSTRUCTURED", "Expected zone name UNSTRUCTURED, got %s",
        zoneName.c_str());

      vtkLogIfF(ERROR, 1 != firstBase->GetNumberOfBlocks(), "Expected 1 block in CGNS output.");

      vtkPointSet* outputGrid = vtkPointSet::SafeDownCast(firstBase->GetBlock(0));
      vtkLogIfF(ERROR, nullptr == outputGrid, "NULL in output.");

      vtkLogIfF(ERROR, std::max(2, size) != outputGrid->GetNumberOfCells(),
        "Expected %d, but got %lld cells in first block.", std::max(2, size),
        outputGrid->GetNumberOfCells());
    }
    {
      vtkMultiBlockDataSet* secondBase = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(1));
      vtkLogIfF(ERROR, nullptr == secondBase, "NULL in output.");
      vtkLogIfF(ERROR, !secondBase->HasMetaData(0u), "No metadata on second base");
      vtkLogIfF(ERROR, !secondBase->GetMetaData(0u)->Has(vtkCompositeDataSet::NAME()),
        "No name on second base");
      const std::string zoneName = secondBase->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
      vtkLogIfF(
        ERROR, zoneName != "POLYDATA", "Expected zone name POLYDATA, got %s.", zoneName.c_str());

      vtkLogIfF(
        ERROR, 1 != secondBase->GetNumberOfBlocks(), "Expected 1 sub-block in CGNS output.");

      vtkPointSet* outputGrid = vtkPointSet::SafeDownCast(secondBase->GetBlock(0));
      vtkLogIfF(ERROR, nullptr == outputGrid, "NULL in output.");

      vtkLogIfF(ERROR, std::max(2, size) != outputGrid->GetNumberOfCells(),
        "Expected %d, but got %lld cells in second block.", std::max(2, size),
        outputGrid->GetNumberOfCells());
    }
    result = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
