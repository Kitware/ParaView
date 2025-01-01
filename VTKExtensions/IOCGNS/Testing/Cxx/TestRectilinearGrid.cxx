// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"

extern int TestRectilinearGrid(int argc, char* argv[])
{
  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);

  vtkNew<vtkRectilinearGrid> rg;
  int i(10), j(10), k(10);
  Create(rg, i, j, k);

  const char* filename = u->GetTempFilePath("rectilinear_grid.cgns");
  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(rg);
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

void Create(vtkRectilinearGrid* rg, vtkIdType I, vtkIdType J, vtkIdType K)
{
  vtkIdType i, j, k;

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

  vtkNew<vtkDoubleArray> x, y, z;
  for (i = 0; i < I; ++i)
  {
    x->InsertNextValue(i);
  }
  for (j = 0; j < J; ++j)
  {
    y->InsertNextValue(2 * j);
  }
  for (k = 0; k < K; ++k)
  {
    z->InsertNextValue(-k);
  }

  rg->SetXCoordinates(x);
  rg->SetYCoordinates(y);
  rg->SetZCoordinates(z);

  for (i = 0; i < I; ++i)
  {
    const auto di = static_cast<double>(i);
    for (j = 0; j < J; ++j)
    {
      const auto dj = static_cast<double>(j);
      for (k = 0; k < K; ++k)
      {
        const auto dk = static_cast<double>(k);

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
  rg->SetDimensions(I, J, K);
  rg->GetCellData()->AddArray(cellPressure);
  rg->GetCellData()->AddArray(cellVelocity);
  rg->GetPointData()->AddArray(vertexPressure);
  rg->GetPointData()->AddArray(vertexVelocity);
}
