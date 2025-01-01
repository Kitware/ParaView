// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/SystemTools.hxx"

int CellAndPointDataTest(vtkPointSet* target, vtkIdType N, const char* phase);

extern int TestCellAndPointData(int argc, char* argv[])
{
  vtkNew<vtkPVTestUtilities> utitilies;
  utitilies->Initialize(argc, argv);
  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  constexpr vtkIdType N = 5;
  Create(unstructuredGrid, N);

  vtkNew<vtkStructuredGrid> structuredGrid;
  Create(structuredGrid, N, N, N);

  // assert that the expected cell and point data is present on the
  // grids that are to be written
  int result = CellAndPointDataTest(unstructuredGrid, N, "CreationUnstructured");
  if (result == EXIT_SUCCESS)
  {
    result = CellAndPointDataTest(structuredGrid, N, "CreationStructured");
  }
  if (result != EXIT_SUCCESS)
  {
    return result;
  }

  // write the files
  const char* filename = utitilies->GetTempFilePath("cell_and_point_data_unstructured.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  vtkNew<vtkCGNSWriter> writer;
  writer->SetFileName(filename);
  writer->SetInputData(unstructuredGrid);

  result = writer->Write();
  if (result != 1)
  {
    return EXIT_FAILURE;
  }

  delete[] filename;
  filename = utitilies->GetTempFilePath("cell_and_point_data_structured.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }
  writer->SetFileName(filename);
  writer->SetInputData(structuredGrid);

  result = writer->Write();
  if (result != 1)
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(filename);
  reader->UpdateInformation();
  reader->EnableAllBases();
  reader->EnableAllCellArrays();
  reader->EnableAllPointArrays();
  reader->Update();

  delete[] filename;
  result = CellAndPointDataTest(reader->GetOutput(), 0, 0, N);
  if (result != EXIT_SUCCESS)
  {
    return result;
  }

  filename = utitilies->GetTempFilePath("cell_and_point_data_unstructured.cgns");
  reader->SetFileName(filename);
  reader->UpdateInformation();
  reader->EnableAllBases();
  reader->EnableAllCellArrays();
  reader->EnableAllPointArrays();
  reader->Update();

  result = CellAndPointDataTest(reader->GetOutput(), 0, 0, N);
  if (result != EXIT_SUCCESS)
  {
    return result;
  }

  delete[] filename;
  return EXIT_SUCCESS;
}
int CellAndPointDataTest(vtkPointSet* target, vtkIdType N, const char* phase)
{
  vtkPointData* pointData = target->GetPointData();
  vtkLogIfF(ERROR, nullptr == pointData, "%s: Point data is NULL", phase);
  vtkLogIfF(ERROR, 2 != pointData->GetNumberOfArrays(), "%s: Expected 2 point data arrays, got %d",
    phase, pointData->GetNumberOfArrays());

  const std::string arrayNames[2] = { "Pressure", "Velocity" };

  vtkCellData* cellData = target->GetCellData();
  vtkLogIfF(ERROR, nullptr == cellData, "%s: Cell data is NULL", phase);
  vtkLogIfF(ERROR, 2 != cellData->GetNumberOfArrays(), "%s: Expected 2 cell data arrays, got %d",
    phase, cellData->GetNumberOfArrays());

  const vtkIdType nPoints = N * N * N;
  N = std::max(1LL, N - 1);
  const vtkIdType nCells = N * N * N;

  for (size_t i = 0; i < 2; ++i)
  {
    const auto cellArr = cellData->GetArray(static_cast<int>(i));
    vtkLogIfF(ERROR, nullptr == cellArr, "%s: No cell data", phase);
    if (cellArr)
    {
      vtkLogIfF(ERROR, cellArr->GetName() != arrayNames[i], "%s: Name does not match", phase);
      vtkLogIfF(ERROR, nCells != cellArr->GetNumberOfTuples(),
        "%s: Number of cell data does not match", phase);
    }

    const auto pointArr = pointData->GetArray(static_cast<int>(i));
    vtkLogIfF(ERROR, nullptr == pointArr, "%s: No point data", phase);
    if (pointArr)
    {
      vtkLogIfF(ERROR, pointArr->GetName() != arrayNames[i], "%s: Name does not match", phase);
      vtkLogIfF(ERROR, nPoints != pointArr->GetNumberOfTuples(),
        "%s: Number of point data does not match", phase);
    }
  }
  return EXIT_SUCCESS;
}

int CellAndPointDataTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, int N)
{
  vtkLogIfF(ERROR, nullptr == read, "Multiblock dataset is NULL");
  vtkLogIfF(ERROR, b0 >= read->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtkLogIfF(ERROR, nullptr == block0, "Block is NULL");
  vtkLogIfF(ERROR, b1 >= block0->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkPointSet* target = vtkPointSet::SafeDownCast(block0->GetBlock(b1));
  vtkLogIfF(ERROR, nullptr == target, "Output grid is NULL");
  if (target->IsA("vktStructuredGrid"))
  {
    return CellAndPointDataTest(target, N, "AssertionStructured");
  }

  return CellAndPointDataTest(target, N, "AssertionUnstructured");
}
