/*=========================================================================

  Program:   ParaView
  Module:    TestUnstructuredGrid.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

int TestUnstructuredGrid(int argc, char* argv[])
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

  return UnstructuredGridTest(r->GetOutput(), 0, 0, 10);
}

int UnstructuredGridTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, int N)
{
  vtk_assert(nullptr != read);
  vtk_assert(b0 < read->GetNumberOfBlocks());

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtk_assert(nullptr != block0);
  vtk_assert(b1 < block0->GetNumberOfBlocks());

  vtkUnstructuredGrid* target = vtkUnstructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtk_assert(nullptr != target);
  vtk_assert(N * N * N == target->GetNumberOfPoints());
  int M = N - 1;
  vtk_assert(M * M * M == target->GetNumberOfCells());
  return EXIT_SUCCESS;
}

void Create(vtkUnstructuredGrid* ug, int N)
{
  vtkNew<vtkPoints> pts;

  int i, j, k;
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
    for (j = 0; j < N; ++j)
    {
      xyz[1] = j / 2.0;
      for (k = 0; k < N; ++k)
      {
        xyz[2] = k * 3.0;
        pts->InsertNextPoint(xyz);
        vertexPressure->InsertNextValue(i + j + k);
        vertexVelocity->InsertNextTuple3(i, j, k);
      }
    }
  }

  ug->SetPoints(pts);

  auto calc = [=](int a, int b, int c) {
    int value = a + b * N + c * N * N;
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
    for (j = 0; j < N - 1; ++j)
    {
      for (k = 0; k < N - 1; ++k)
      {
        cellPressure->InsertNextValue(i + j + k);
        cellVelocity->InsertNextTuple3(i, j, k);

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
