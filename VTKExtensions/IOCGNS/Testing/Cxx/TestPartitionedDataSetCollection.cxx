/*=========================================================================

  Program:   ParaView
  Module:    TestPartitionedDataSetCollection.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

int TestPartitionedDataSetCollection(int argc, char* argv[])
{

  int result(0);

  vtkNew<vtkPartitionedDataSetCollection> partitionedCollection;
  partitionedCollection->SetNumberOfPartitionedDataSets(2);
  partitionedCollection->SetNumberOfPartitions(0u, 4);
  partitionedCollection->SetNumberOfPartitions(1u, 4);
  constexpr vtkIdType N = 5, M = 7;
  {
    vtkNew<vtkUnstructuredGrid> ug1, ug2;
    Create(ug1, N);

    vtkNew<vtkPolyData> pd;
    Create(ug2, M);

    partitionedCollection->SetPartition(0u, 1, ug1);
    partitionedCollection->SetPartition(1u, 2, ug2);
    partitionedCollection->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED1");
    partitionedCollection->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED2");
  }

  const vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("partitioned-collection.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  const vtkNew<vtkCGNSWriter> writer;
  writer->SetInputData(partitionedCollection);
  writer->SetFileName(filename);

  result = writer->Write();

  if (result == 1)
  {
    vtkLogIfF(ERROR, !vtksys::SystemTools::FileExists(filename), "File %s not found", filename);
    const vtkNew<vtkCGNSReader> reader;
    reader->SetFileName(filename);
    // update information first to get all bases in the information
    reader->UpdateInformation();
    // then enable all bases get both bases (volume, surface) into the output
    reader->EnableAllBases();
    reader->Update();

    const unsigned long err = reader->GetErrorCode();
    vtkLogIfF(ERROR, err != 0, "CGNS reading failed.");

    vtkMultiBlockDataSet* output = reader->GetOutput();
    vtkLogIfF(ERROR, nullptr == output, "No CGNS reader output.");
    vtkLogIfF(ERROR, 1 != output->GetNumberOfBlocks(),
      "Expected 1 base in the CGNS output, got %d.", output->GetNumberOfBlocks());
    vtkMultiBlockDataSet* firstBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
    vtkLogIfF(ERROR, nullptr == firstBlock, "NULL in output.");
    vtkLogIfF(ERROR, 2 != firstBlock->GetNumberOfBlocks(), "Expected 2 sub-block in CGNS output.");

    vtkLogIfF(ERROR, !firstBlock->HasMetaData(0u), "Expected metadata on the first block");
    vtkLogIfF(ERROR, !firstBlock->GetMetaData(0u)->Has(vtkCompositeDataSet::NAME()),
      "Expected NAME() on first block");
    std::string expectedName = "UNSTRUCTURED1";
    vtkLogIfF(ERROR, expectedName != firstBlock->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME()),
      "Expected first block name to be %s, but found %s", expectedName.c_str(),
      firstBlock->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME()));

    vtkLogIfF(ERROR, !firstBlock->HasMetaData(1u), "Expected metadata on the first block");
    vtkLogIfF(ERROR, !firstBlock->GetMetaData(1u)->Has(vtkCompositeDataSet::NAME()),
      "Expected NAME() on first block");
    expectedName = "UNSTRUCTURED2";
    vtkLogIfF(ERROR, expectedName != firstBlock->GetMetaData(1u)->Get(vtkCompositeDataSet::NAME()),
      "Expected first block name to be %s, but found %s", expectedName.c_str(),
      firstBlock->GetMetaData(1u)->Get(vtkCompositeDataSet::NAME()));

    vtkUnstructuredGrid* firstOutputGrid =
      vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(0));
    vtkLogIfF(ERROR, nullptr == firstOutputGrid, "NULL in output.");

    constexpr vtkIdType Nm1 = N - 1;
    constexpr vtkIdType Mm1 = M - 1;
    vtkIdType expected(Nm1 * Nm1 * Nm1);
    vtkLogIfF(ERROR, expected != firstOutputGrid->GetNumberOfCells(),
      "Expected %lld, but got %lld cells in first block.", expected,
      firstOutputGrid->GetNumberOfCells());

    vtkUnstructuredGrid* secondOutputGrid =
      vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(1));
    vtkLogIfF(ERROR, nullptr == secondOutputGrid, "NULL in output.");

    expected = Mm1 * Mm1 * Mm1;
    vtkLogIfF(ERROR, expected != secondOutputGrid->GetNumberOfCells(),
      "Expected %lld, but got %lld cells in first block.", expected,
      secondOutputGrid->GetNumberOfCells());

    result = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return result == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
