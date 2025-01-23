// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCell.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

extern int TestPartitionedDataSet(int argc, char* argv[])
{
  vtkNew<vtkPartitionedDataSet> partitioned;
  partitioned->SetNumberOfPartitions(4);

  vtkNew<vtkUnstructuredGrid> ug0, ug3;
  constexpr vtkIdType M = 6, N = 4;
  Create(ug0, M);
  Create(ug3, N);

  partitioned->SetPartition(0u, ug0);
  partitioned->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED");
  partitioned->SetPartition(3u, ug3);
  partitioned->GetMetaData(3u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED");

  vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("partitioned.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  vtkNew<vtkCGNSWriter> writer;
  writer->SetInputData(partitioned);
  writer->SetFileName(filename);

  int result = writer->Write();

  if (result == 1)
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
    vtkLogIfF(ERROR, err != 0, "Reading %s failed", filename);

    vtkMultiBlockDataSet* output = reader->GetOutput();
    vtkLogIfF(ERROR, nullptr == output, "Read output is NULL");
    vtkLogIfF(ERROR, 1 != output->GetNumberOfBlocks(), "Expected 1 block, got %d",
      output->GetNumberOfBlocks());

    vtkMultiBlockDataSet* firstBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
    vtkLogIfF(ERROR, nullptr == firstBlock, "First block is NULL");
    const char* name = firstBlock->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
    const size_t len = std::min(static_cast<size_t>(12), strlen(name));
    vtkLogIfF(ERROR, 0 != strncmp(name, "UNSTRUCTURED", len),
      "Block name '%s' is not 'UNSTRUCTURED'", name);

    vtkLogIfF(ERROR, 1 != firstBlock->GetNumberOfBlocks(), "Expected 1 grid in the first block");

    vtkUnstructuredGrid* outputGrid = vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(0));
    vtkLogIfF(ERROR, nullptr == outputGrid, "Grid in first block is NULL");

    // number of cells expected is point-dimension minus 1 to the power 3
    constexpr vtkIdType Nm1 = N - 1;
    constexpr vtkIdType Mm1 = M - 1;

    vtkLogIfF(ERROR, Mm1 * Mm1 * Mm1 + Nm1 * Nm1 * Nm1 != outputGrid->GetNumberOfCells(),
      "Expected %lld cells, got %lld", Mm1 * Mm1 * Mm1 + Nm1 * Nm1 * Nm1,
      outputGrid->GetNumberOfCells());

    result = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
