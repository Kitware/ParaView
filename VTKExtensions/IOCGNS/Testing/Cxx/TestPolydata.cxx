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
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

extern int TestPolydata(int argc, char* argv[])
{
  vtkNew<vtkPolyData> pd;
  Create(pd);

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  const char* filename = u->GetTempFilePath("polydata.cgns");

  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(pd);
  int rc = w->Write();

  if (rc != 1)
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkCGNSReader> r;
  r->EnableAllBases();
  r->SetFileName(filename);
  r->Update();

  vtkMultiBlockDataSet* read = r->GetOutput();
  delete[] filename;

  return PolydataTest(read, 0, 0, "Zone 1");
}

int PolydataTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, const char* name)
{
  vtkLogIfF(ERROR, nullptr == read, "Read dataset is NULL");
  vtkLogIfF(ERROR, b0 >= read->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtkLogIfF(ERROR, nullptr == block0, "Block0 is NULL");
  vtkLogIfF(ERROR, b1 >= block0->GetNumberOfBlocks(), "Number of blocks does not match");
  const char* blockName = block0->GetMetaData(b1)->Get(vtkCompositeDataSet::NAME());
  vtkLogIfF(ERROR, 0 != strncmp(name, blockName, std::min(strlen(name), strlen(blockName))),
    "Name '%s' is not expected. Expected is %s", blockName, name);

  vtkUnstructuredGrid* target = vtkUnstructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtkLogIfF(ERROR, nullptr == target, "Output grid is NULL");

  vtkLogIfF(ERROR, 3 != target->GetNumberOfCells(), "Expected 3 cells, got %lld",
    target->GetNumberOfCells());
  vtkLogIfF(ERROR, 7 != target->GetNumberOfPoints(), "Expected 7 points, got %lld",
    target->GetNumberOfPoints());

  return EXIT_SUCCESS;
}

void Create(vtkPolyData* pd)
{
  vtkNew<vtkPoints> pts;

  pd->SetPoints(pts);
  pts->Allocate(7);
  pd->Allocate(3);

  pts->InsertNextPoint(0, 0, 0);
  pts->InsertNextPoint(2, 0, 0);
  pts->InsertNextPoint(2, 2, 0);
  pts->InsertNextPoint(0, 2, 0);

  pts->InsertNextPoint(4, 1, 0);
  pts->InsertNextPoint(4, 4, 0);
  pts->InsertNextPoint(0, 4, 0);

  vtkNew<vtkIdList> cell;
  cell->InsertNextId(0);
  cell->InsertNextId(1);
  cell->InsertNextId(2);
  cell->InsertNextId(3);
  pd->InsertNextCell(VTK_QUAD, cell);

  cell->Reset();
  cell->InsertNextId(1);
  cell->InsertNextId(4);
  cell->InsertNextId(2);
  pd->InsertNextCell(VTK_TRIANGLE, cell);

  cell->Reset();
  cell->InsertNextId(2);
  cell->InsertNextId(4);
  cell->InsertNextId(5);
  cell->InsertNextId(6);
  cell->InsertNextId(3);
  pd->InsertNextCell(VTK_POLYGON, cell);

  vtkNew<vtkDoubleArray> ptPres;
  vtkNew<vtkDoubleArray> clPres;

  for (vtkIdType i = 0; i < pts->GetNumberOfPoints(); ++i)
  {
    ptPres->InsertNextValue(i);
  }
  for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i)
  {
    clPres->InsertNextValue(i);
  }
  ptPres->SetName("Pressure");
  clPres->SetName("Pressure");

  pd->GetPointData()->AddArray(ptPres);
  pd->GetCellData()->AddArray(clPres);
}
