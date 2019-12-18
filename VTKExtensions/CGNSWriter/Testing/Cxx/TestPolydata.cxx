/*=========================================================================

  Program:   ParaView
  Module:    TestPolydata.cxx

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
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int TestPolydata(int argc, char* argv[])
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

  return PolydataTest(read, 0, 0);
}

int PolydataTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1)
{
  vtk_assert(nullptr != read);
  vtk_assert(b0 < read->GetNumberOfBlocks());

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtk_assert(nullptr != block0);
  vtk_assert(b1 < block0->GetNumberOfBlocks());

  vtkUnstructuredGrid* target = vtkUnstructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtk_assert(nullptr != target);

  vtk_assert(3 == target->GetNumberOfCells());
  vtk_assert(7 == target->GetNumberOfPoints());

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
}
