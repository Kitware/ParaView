// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

extern int TestUnstructuredGrid(int argc, char* argv[])
{
  vtkNew<vtkUnstructuredGrid> ug;
  Create(ug.GetPointer(), 10);

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  const char* filename = u->GetTempFilePath("unstructured_grid.cgns");

  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(ug);
  int rc = w->Write();
  if (rc != 1)
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkCGNSReader> r;
  r->SetFileName(filename);
  r->EnableAllBases();
  r->Update();

  delete[] filename;

  return UnstructuredGridTest(r->GetOutput(), 0, 0, 10, "Zone 1");
}

int UnstructuredGridTest(
  vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, int N, const char* name)
{
  vtkLogIfF(ERROR, nullptr == read, "Multiblock data set is NULL");
  vtkLogIfF(ERROR, b0 >= read->GetNumberOfBlocks(), "NUmber of blocks does not match");

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtkLogIfF(ERROR, nullptr == block0, "Block0 is NULL");
  vtkLogIfF(ERROR, b1 >= block0->GetNumberOfBlocks(), "Number of blocks does not match");
  const char* blockName = block0->GetMetaData(b1)->Get(vtkCompositeDataSet::NAME());

  vtkLogIfF(ERROR, 0 != strncmp(name, blockName, std::min(strlen(name), strlen(blockName))),
    "Name '%s' does not match expected name '%s'", blockName, name);

  vtkUnstructuredGrid* target = vtkUnstructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtkLogIfF(ERROR, nullptr == target, "output grid is NULL");
  vtkLogIfF(ERROR, N * N * N != target->GetNumberOfPoints(), "Expected %d points, got %lld",
    N * N * N, target->GetNumberOfPoints());
  int M = N - 1;
  vtkLogIfF(ERROR, M * M * M != target->GetNumberOfCells(), "Expected %d cells, got %lld",
    M * M * M, target->GetNumberOfCells());
  return EXIT_SUCCESS;
}

void Create(vtkUnstructuredGrid* ug, vtkIdType N)
{
  vtkNew<vtkPoints> pts;

  vtkIdType i, j, k;
  vtkNew<vtkDoubleArray> cellPressure;
  cellPressure->SetName("Pressure");
  cellPressure->Allocate(N * N * N);

  vtkNew<vtkDoubleArray> vertexPressure;
  vertexPressure->SetName("Pressure");
  vertexPressure->Allocate(N * N * N);

  vtkNew<vtkDoubleArray> cellVelocity;
  cellVelocity->SetName("Velocity");
  cellVelocity->SetNumberOfComponents(3);
  cellVelocity->Allocate(3 * N * N * N);

  vtkNew<vtkDoubleArray> vertexVelocity;
  vertexVelocity->SetName("Velocity");
  vertexVelocity->SetNumberOfComponents(3);
  vertexVelocity->Allocate(3 * N * N * N);

  pts->Allocate(N * N * N);

  double xyz[3];

  for (i = 0; i < N; ++i)
  {
    xyz[0] = 1.0 * i;
    const auto di = static_cast<double>(i);
    for (j = 0; j < N; ++j)
    {
      xyz[1] = j / 2.0;
      const auto dj = static_cast<double>(j);
      for (k = 0; k < N; ++k)
      {
        const auto dk = static_cast<double>(k);
        xyz[2] = k * 3.0;
        pts->InsertNextPoint(xyz);
        vertexPressure->InsertNextValue(di + dj + dk);
        vertexVelocity->InsertNextTuple3(di, dj, dk);
      }
    }
  }

  ug->SetPoints(pts);

  auto calc = [=](vtkIdType a, vtkIdType b, vtkIdType c)
  {
    vtkIdType value = a + b * N + c * N * N;
    if (value < 0 || value >= N * N * N)
    {
      throw std::runtime_error("Value out of range");
    }
    return value;
  };

  vtkNew<vtkIdList> cellIds;
  cellIds->Allocate(8);
  for (i = 0; i < N - 1; ++i)
  {
    const auto di = static_cast<double>(i);
    for (j = 0; j < N - 1; ++j)
    {
      const auto dj = static_cast<double>(j);
      for (k = 0; k < N - 1; ++k)
      {
        const auto dk = static_cast<double>(k);
        cellPressure->InsertNextValue(di + dj + dk);
        cellVelocity->InsertNextTuple3(di, dj, dk);

        cellIds->Reset();

        vtkIdType n0 = calc(i + 0, j + 0, k + 0);
        vtkIdType n1 = calc(i + 1, j + 0, k + 0);
        vtkIdType n2 = calc(i + 1, j + 1, k + 0);
        vtkIdType n3 = calc(i + 0, j + 1, k + 0);
        vtkIdType n4 = calc(i + 0, j + 0, k + 1);
        vtkIdType n5 = calc(i + 1, j + 0, k + 1);
        vtkIdType n6 = calc(i + 1, j + 1, k + 1);
        vtkIdType n7 = calc(i + 0, j + 1, k + 1);

        cellIds->InsertNextId(n0);
        cellIds->InsertNextId(n1);
        cellIds->InsertNextId(n2);
        cellIds->InsertNextId(n3);
        cellIds->InsertNextId(n4);
        cellIds->InsertNextId(n5);
        cellIds->InsertNextId(n6);
        cellIds->InsertNextId(n7);

        ug->InsertNextCell(VTK_HEXAHEDRON, cellIds);
      }
    }
  }

  ug->GetCellData()->AddArray(cellPressure);
  ug->GetCellData()->AddArray(cellVelocity);
  ug->GetPointData()->AddArray(vertexPressure);
  ug->GetPointData()->AddArray(vertexVelocity);
}
