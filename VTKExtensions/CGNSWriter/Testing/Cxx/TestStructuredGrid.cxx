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
    int i(10), j(10), k(1); // 2D in K-direction
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
  vtk_assert(nullptr != read);
  vtk_assert(b0 < read->GetNumberOfBlocks());

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtk_assert(nullptr != block0);
  vtk_assert(b1 < block0->GetNumberOfBlocks());

  vtkStructuredGrid* target = vtkStructuredGrid::SafeDownCast(block0->GetBlock(b1));
  cout << block0->GetBlock(b1)->GetDataObjectType() << endl;
  vtk_assert(nullptr != target);

  vtk_assert_equal(I * J * K, target->GetNumberOfPoints());
  vtk_assert(I * J * K == target->GetNumberOfPoints());

  int Im1 = std::max(1, I - 1);
  int Jm1 = std::max(1, J - 1);
  int Km1 = std::max(1, K - 1);
  vtk_assert_equal(Im1 * Jm1 * Km1, target->GetNumberOfCells());

  return EXIT_SUCCESS;
}

void Create(vtkStructuredGrid* sg, int I, int J, int K)
{
  int i, j, k;
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
    for (j = 0; j < J; ++j)
    {
      xyz[1] = j / 2.0;
      for (k = 0; k < K; ++k)
      {
        xyz[2] = k * 3.0;
        pts->InsertNextPoint(xyz);
        vertexPressure->InsertNextValue(i + j + k);
        vertexVelocity->InsertNextTuple3(i, j, k);
      }
    }
  }

  for (i = 0; i < I - 1; ++i)
  {
    for (j = 0; j < J - 1; ++j)
    {
      for (k = 0; k < K - 1; ++k)
      {
        cellPressure->InsertNextValue(i + j + k);
        cellVelocity->InsertNextTuple3(i, j, k);
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
