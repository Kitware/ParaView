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
#include "vtkStructuredGrid.h"

int TestStructuredGrid(int argc, char* argv[])
{
  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  {
    vtkNew<vtkStructuredGrid> sg;
    int i(10), j(10), k(10);
    Create(sg, i, j, k);

    const char* filename = u->GetTempFilePath("structured_grid.cgns");
    vtkNew<vtkCGNSWriter> w;
    w->UseHDF5Off();
    w->SetFileName(filename);
    w->SetInputData(sg);
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
    rc = StructuredGridTest(r->GetOutput(), 0, 0, i, j, k);
    if (rc != EXIT_SUCCESS)
      return rc;
  }
  {
    vtkNew<vtkStructuredGrid> sg2D;
    const int i(10), j(10), k(1); // 2D in K-direction
    Create(sg2D, i, j, k);
    const char* filename = u->GetTempFilePath("structured_grid_2d.cgns");
    vtkNew<vtkCGNSWriter> w;
    w->UseHDF5Off();
    w->SetFileName(filename);
    w->SetInputData(sg2D);
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
    return StructuredGridTest(r->GetOutput(), 0, 0, i, j, k);
  }
}

int StructuredGridTest(
  vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, int I, int J, int K)
{
  vtkLogIfF(ERROR, nullptr == read, "Multiblock dataset is NULL");
  vtkLogIfF(ERROR, b0 >= read->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtkLogIfF(ERROR, nullptr == block0, "Block is NULL");
  vtkLogIfF(ERROR, b1 >= block0->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkStructuredGrid* target = vtkStructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtkLogIfF(ERROR, nullptr == target, "OUtput grid is NULL");
  const vtkIdType nPts = I * J * K;
  vtkLogIfF(ERROR, nPts != target->GetNumberOfPoints(), "Expected %lld points, got %lld", nPts,
    target->GetNumberOfPoints());

  const vtkIdType Im1 = std::max(1, I - 1);
  const vtkIdType Jm1 = std::max(1, J - 1);
  const vtkIdType Km1 = std::max(1, K - 1);
  vtkLogIfF(ERROR, Im1 * Jm1 * Km1 != target->GetNumberOfCells(), "Expected %lld cells, got %lld",
    Im1 * Jm1 * Km1, target->GetNumberOfCells());

  return EXIT_SUCCESS;
}

void Create(vtkStructuredGrid* sg, vtkIdType I, vtkIdType J, vtkIdType K)
{
  vtkIdType i, j, k;
  vtkNew<vtkPoints> pts;

  vtkNew<vtkDoubleArray> cellPressure;
  cellPressure->SetName("Pressure");
  cellPressure->Allocate(I * J * K);

  vtkNew<vtkDoubleArray> vertexPressure;
  vertexPressure->SetName("Pressure");
  vertexPressure->Allocate(I * J * K);

  vtkNew<vtkDoubleArray> cellVelocity;
  cellVelocity->SetName("Velocity");
  cellVelocity->SetNumberOfComponents(3);
  cellVelocity->Allocate(3 * I * J * K);

  vtkNew<vtkDoubleArray> vertexVelocity;
  vertexVelocity->SetName("Velocity");
  vertexVelocity->SetNumberOfComponents(3);
  vertexVelocity->Allocate(3 * I * J * K);

  pts->Allocate(I * J * K);

  double xyz[3];

  for (i = 0; i < I; ++i)
  {
    xyz[0] = 1.0 * i;
    const auto di = static_cast<double>(i);
    for (j = 0; j < J; ++j)
    {
      xyz[1] = j / 2.0;
      const auto dj = static_cast<double>(j);
      for (k = 0; k < K; ++k)
      {
        xyz[2] = k * 3.0;
        const auto dk = static_cast<double>(k);

        pts->InsertNextPoint(xyz);
        vertexPressure->InsertNextValue(di + dj + dk);
        vertexVelocity->InsertNextTuple3(di, dj, dk);
      }
    }
  }

  for (i = 0; i < I - 1; ++i)
  {
    const auto di = static_cast<double>(i);
    for (j = 0; j < J - 1; ++j)
    {
      const auto dj = static_cast<double>(j);
      for (k = 0; k < K - 1; ++k)
      {
        const auto dk = static_cast<double>(k);
        cellPressure->InsertNextValue(di + dj + dk);
        cellVelocity->InsertNextTuple3(di, dj, dk);
      }
    }
  }

  sg->SetDimensions(I, J, K);
  sg->SetPoints(pts);
  sg->GetCellData()->AddArray(cellPressure);
  sg->GetCellData()->AddArray(cellVelocity);
  sg->GetPointData()->AddArray(vertexPressure);
  sg->GetPointData()->AddArray(vertexVelocity);
}
