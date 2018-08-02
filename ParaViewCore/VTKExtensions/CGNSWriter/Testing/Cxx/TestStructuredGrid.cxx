/*=========================================================================

  Program:   ParaView
  Module:    TestStructuredGrid.cxx

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
#include "vtkDoubleArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"

int TestStructuredGrid(int argc, char* argv[])
{
  vtkNew<vtkStructuredGrid> sg;
  Create(sg, 10);

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
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
  return StructuredGridTest(r->GetOutput(), 0, 0, 10);
}

int StructuredGridTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, int N)
{
  vtk_assert(nullptr != read);
  vtk_assert(b0 < read->GetNumberOfBlocks());

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtk_assert(nullptr != block0);
  vtk_assert(b1 < block0->GetNumberOfBlocks());

  vtkStructuredGrid* target = vtkStructuredGrid::SafeDownCast(block0->GetBlock(b1));
  cout << block0->GetBlock(b1)->GetDataObjectType() << endl;
  vtk_assert(nullptr != target);

  vtk_assert(N * N * N == target->GetNumberOfPoints());
  int M = N - 1;
  vtk_assert(M * M * M == target->GetNumberOfCells());

  return EXIT_SUCCESS;
}

void Create(vtkStructuredGrid* sg, int N)
{
  int i, j, k;
  vtkNew<vtkPoints> pts;

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

  for (i = 0; i < N - 1; ++i)
  {
    for (j = 0; j < N - 1; ++j)
    {
      for (k = 0; k < N - 1; ++k)
      {
        cellPressure->InsertNextValue(i + j + k);
        cellVelocity->InsertNextTuple3(i, j, k);
      }
    }
  }

  sg->SetDimensions(N, N, N);
  sg->SetPoints(pts);
  sg->GetCellData()->AddArray(cellPressure);
  sg->GetCellData()->AddArray(cellVelocity);
  sg->GetPointData()->AddArray(vertexPressure);
  sg->GetPointData()->AddArray(vertexVelocity);
}
