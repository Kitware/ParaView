// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

int TestPolyhedral(int argc, char* argv[])
{
  vtkNew<vtkUnstructuredGrid> ph;
  CreatePolyhedral(ph);

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  const char* filename = u->GetTempFilePath("polyhedral.cgns");

  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetInputData(ph);
  w->SetFileName(filename);
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

  return PolyhedralTest(r->GetOutput(), 0, 0);
}

int PolyhedralTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1)
{
  vtkLogIfF(ERROR, nullptr == read, "Read dataset is NULL");
  vtkLogIfF(ERROR, b0 >= read->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtkLogIfF(ERROR, nullptr == block0, "Block0 is NULL");
  vtkLogIfF(ERROR, b1 >= block0->GetNumberOfBlocks(), "Number of blocks does not match");

  vtkUnstructuredGrid* target = vtkUnstructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtkLogIfF(ERROR, nullptr == target, "Output grid is NULL");

  vtkLogIfF(ERROR, 2 != target->GetNumberOfCells(), "Expected 2 cells, got %lld",
    target->GetNumberOfCells());
  vtkLogIfF(ERROR, 16 != target->GetNumberOfPoints(), "Expected 16 points, got %lld",
    target->GetNumberOfPoints());

  return EXIT_SUCCESS;
}

void CreatePolyhedral(vtkUnstructuredGrid* ph)
{

  /*  An unstructured grid  with two polyhedra

        top-view:

        back
        o----o----o
        |    |    |
        |    |    |                    right/middle side         left side               front/back
  Left  |    o    o  Right             o----o----o               o---------o             o----o----o
        |    |    |                    |    |    |               |         |             |    |    |
        |    |    |                    |    |    |               |         |             |    |    |
        o----o----o                    o----o----o               o---------o             o----o----o
        front

  */
  vtkNew<vtkPoints> pts;

  ph->SetPoints(pts);

  pts->InsertNextPoint(0, 0, 0);
  pts->InsertNextPoint(1, 0, 0);
  pts->InsertNextPoint(1, 1, 0);
  pts->InsertNextPoint(1, 2, 0);
  pts->InsertNextPoint(0, 2, 0);

  pts->InsertNextPoint(0, 0, 1);
  pts->InsertNextPoint(1, 0, 1);
  pts->InsertNextPoint(1, 1, 1);
  pts->InsertNextPoint(1, 2, 1);
  pts->InsertNextPoint(0, 2, 1);

  pts->InsertNextPoint(2, 0, 0);
  pts->InsertNextPoint(2, 1, 0);
  pts->InsertNextPoint(2, 2, 0);

  pts->InsertNextPoint(2, 0, 1);
  pts->InsertNextPoint(2, 1, 1);
  pts->InsertNextPoint(2, 2, 1);

  vtkNew<vtkIdList> polyhedron;
  polyhedron->Allocate(50);
  polyhedron->InsertNextId(7); // faces

  polyhedron->InsertNextId(4); // face0-4verts
  polyhedron->InsertNextId(0);
  polyhedron->InsertNextId(1);
  polyhedron->InsertNextId(6);
  polyhedron->InsertNextId(5);

  polyhedron->InsertNextId(4); // face1-4verts
  polyhedron->InsertNextId(1);
  polyhedron->InsertNextId(2);
  polyhedron->InsertNextId(7);
  polyhedron->InsertNextId(6);

  polyhedron->InsertNextId(4); // face2-4verts
  polyhedron->InsertNextId(2);
  polyhedron->InsertNextId(3);
  polyhedron->InsertNextId(8);
  polyhedron->InsertNextId(7);

  polyhedron->InsertNextId(4); // face3-4verts
  polyhedron->InsertNextId(3);
  polyhedron->InsertNextId(4);
  polyhedron->InsertNextId(9);
  polyhedron->InsertNextId(8);

  polyhedron->InsertNextId(4); // face4-4verts
  polyhedron->InsertNextId(4);
  polyhedron->InsertNextId(0);
  polyhedron->InsertNextId(5);
  polyhedron->InsertNextId(9);

  polyhedron->InsertNextId(5); // face5-5verts
  polyhedron->InsertNextId(5);
  polyhedron->InsertNextId(6);
  polyhedron->InsertNextId(7);
  polyhedron->InsertNextId(8);
  polyhedron->InsertNextId(9);

  polyhedron->InsertNextId(5); // face6-5verts
  polyhedron->InsertNextId(4);
  polyhedron->InsertNextId(3);
  polyhedron->InsertNextId(2);
  polyhedron->InsertNextId(1);
  polyhedron->InsertNextId(0);

  ph->InsertNextCell(VTK_POLYHEDRON, polyhedron);

  polyhedron->Reset();

  polyhedron->InsertNextId(8); // faces

  polyhedron->InsertNextId(4); // face0-4verts
  polyhedron->InsertNextId(1);
  polyhedron->InsertNextId(10);
  polyhedron->InsertNextId(13);
  polyhedron->InsertNextId(6);

  polyhedron->InsertNextId(4); // face1-4verts
  polyhedron->InsertNextId(10);
  polyhedron->InsertNextId(11);
  polyhedron->InsertNextId(14);
  polyhedron->InsertNextId(13);

  polyhedron->InsertNextId(4); // face2-4verts
  polyhedron->InsertNextId(11);
  polyhedron->InsertNextId(12);
  polyhedron->InsertNextId(15);
  polyhedron->InsertNextId(14);

  polyhedron->InsertNextId(4); // face3-4verts
  polyhedron->InsertNextId(3);
  polyhedron->InsertNextId(12);
  polyhedron->InsertNextId(15);
  polyhedron->InsertNextId(8);

  polyhedron->InsertNextId(4); // face4-4verts
  polyhedron->InsertNextId(3);
  polyhedron->InsertNextId(2);
  polyhedron->InsertNextId(7);
  polyhedron->InsertNextId(8);

  polyhedron->InsertNextId(4); // face5-4verts
  polyhedron->InsertNextId(2);
  polyhedron->InsertNextId(1);
  polyhedron->InsertNextId(6);
  polyhedron->InsertNextId(7);

  polyhedron->InsertNextId(6); // face6-6verts
  polyhedron->InsertNextId(6);
  polyhedron->InsertNextId(13);
  polyhedron->InsertNextId(14);
  polyhedron->InsertNextId(15);
  polyhedron->InsertNextId(8);
  polyhedron->InsertNextId(7);

  polyhedron->InsertNextId(6); // face7-6verts
  polyhedron->InsertNextId(3);
  polyhedron->InsertNextId(12);
  polyhedron->InsertNextId(11);
  polyhedron->InsertNextId(10);
  polyhedron->InsertNextId(1);
  polyhedron->InsertNextId(2);

  ph->InsertNextCell(VTK_POLYHEDRON, polyhedron);
}
