// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"

extern int TestImageData(int argc, char* argv[])
{
  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);

  vtkNew<vtkImageData> id;
  int i(10), j(10), k(10);
  Create(id, i, j, k);

  const char* filename = u->GetTempFilePath("image_data.cgns");
  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(id);
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

void Create(vtkImageData* sg, vtkIdType I, vtkIdType J, vtkIdType K)
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

  sg->SetDimensions(I, J, K);
  sg->GetCellData()->AddArray(cellPressure);
  sg->GetCellData()->AddArray(cellVelocity);
  sg->GetPointData()->AddArray(vertexPressure);
  sg->GetPointData()->AddArray(vertexVelocity);
}
