// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2020 Menno Deij - van Rijswijk (MARIN)
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCell.h"
#include "vtkConvertPolyhedraFilter.h"
#include "vtkErrorObserver.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <string>

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;               \
    return EXIT_FAILURE;                                                                           \
  }

#define vtk_observe_has_error(o)                                                                   \
  if (!(o)->GetError())                                                                            \
  {                                                                                                \
    return EXIT_FAILURE;                                                                           \
  }                                                                                                \
  (o)->Clear();

#define vtk_observe_has_no_error(o)                                                                \
  if ((o)->GetError())                                                                             \
    return EXIT_FAILURE;

class TestConvertPolyhedra
{
public:
  static int DoTest()
  {
    vtkNew<vtkConvertPolyhedraFilter> r;
    vtkNew<vtkUnstructuredGrid> ug;
    vtkNew<vtkIdList> ids;
    vtkNew<vtkCellArray> faces;

    vtkNew<vtkErrorObserver> obs;
    r->AddObserver(vtkCommand::ErrorEvent, obs);

    // === expect error if input is null
    r->InsertNextPolygonalCell(nullptr, nullptr);
    vtk_observe_has_error(obs);
    r->InsertNextPolygonalCell(nullptr, ids);
    vtk_observe_has_error(obs);
    r->InsertNextPolygonalCell(ug, nullptr);
    vtk_observe_has_error(obs);

    // === expect error if less than 3 ids are given
    r->InsertNextPolygonalCell(ug, ids);
    vtk_observe_has_error(obs);

    ids->InsertNextId(0);
    r->InsertNextPolygonalCell(ug, ids);
    vtk_observe_has_error(obs);

    ids->InsertNextId(1);
    r->InsertNextPolygonalCell(ug, ids);
    vtk_observe_has_error(obs);

    ids->InsertNextId(2);

    // === VTK_TRIANGLE === //
    r->InsertNextPolygonalCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    vtk_assert(ug->GetCellType(0) == VTK_TRIANGLE);

    // === VTK_QUAD === //
    ug->Reset();
    ids->InsertNextId(3);
    r->InsertNextPolygonalCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    vtk_assert(ug->GetCellType(0) == VTK_QUAD);

    // === VTK_POLYGON === //
    ug->Reset();
    ids->InsertNextId(4);
    r->InsertNextPolygonalCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    vtk_assert(ug->GetCellType(0) == VTK_POLYGON);

    // === VTK_TETRA === //
    ug->Reset();
    ids->Reset();
    ids->InsertNextId(0);
    ids->InsertNextId(1);
    ids->InsertNextId(2);
    ids->InsertNextId(3);
    faces->Reset();
    faces->InsertNextCell({ 0, 1, 2 }); // side
    faces->InsertNextCell({ 1, 2, 3 }); // side
    faces->InsertNextCell({ 1, 3, 0 }); // side
    faces->InsertNextCell({ 0, 2, 3 }); // side

    r->InsertNextPolyhedralCell(ug, ids, faces);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    vtk_assert(ug->GetCellType(0) == VTK_TETRA);

    // === VTK_PYRAMID === //
    ug->Reset();
    ids->Reset();
    ids->InsertNextId(0);
    ids->InsertNextId(1);
    ids->InsertNextId(2);
    ids->InsertNextId(3);
    ids->InsertNextId(4);
    faces->Reset();
    faces->InsertNextCell({ 0, 1, 4 });    // side
    faces->InsertNextCell({ 4, 1, 2 });    // side
    faces->InsertNextCell({ 2, 3, 4 });    // side
    faces->InsertNextCell({ 3, 4, 0 });    // side
    faces->InsertNextCell({ 0, 1, 2, 3 }); // bottom

    r->InsertNextPolyhedralCell(ug, ids, faces);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    int ct = ug->GetCellType(0);
    vtk_assert(ct == VTK_PYRAMID);

    // === VTK_WEDGE === //
    ug->Reset();
    ids->Reset();
    ids->InsertNextId(0);
    ids->InsertNextId(1);
    ids->InsertNextId(2);
    ids->InsertNextId(3);
    ids->InsertNextId(4);
    ids->InsertNextId(5);
    faces->Reset();
    faces->InsertNextCell({ 0, 1, 2 });    // top
    faces->InsertNextCell({ 0, 3, 4, 1 }); // side
    faces->InsertNextCell({ 3, 5, 2, 0 }); // side
    faces->InsertNextCell({ 2, 5, 4, 1 }); // side
    faces->InsertNextCell({ 3, 5, 4 });    // bottom

    r->InsertNextPolyhedralCell(ug, ids, faces);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    ct = ug->GetCellType(0);
    vtk_assert(ct == VTK_WEDGE);

    // === VTK_HEXAHEDRON === //
    ug->Reset();
    ids->Reset();
    ids->InsertNextId(0);
    ids->InsertNextId(1);
    ids->InsertNextId(2);
    ids->InsertNextId(3);
    ids->InsertNextId(4);
    ids->InsertNextId(5);
    ids->InsertNextId(6);
    ids->InsertNextId(7);
    faces->Reset();
    faces->InsertNextCell({ 0, 1, 2, 3 }); // bottom
    faces->InsertNextCell({ 2, 6, 5, 1 }); // side
    faces->InsertNextCell({ 5, 4, 0, 1 }); // side
    faces->InsertNextCell({ 4, 5, 6, 7 }); // top
    faces->InsertNextCell({ 3, 7, 6, 2 }); // side
    faces->InsertNextCell({ 7, 3, 0, 4 }); // side

    r->InsertNextPolyhedralCell(ug, ids, faces);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    ct = ug->GetCellType(0);
    vtk_assert(ct == VTK_HEXAHEDRON);

    r->RemoveObserver(obs);

    return EXIT_SUCCESS;
  }
};

extern int TestPolyhedralToSimpleCellsFilter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // test is inside class such that friend declaration in
  // filter can work
  return TestConvertPolyhedra::DoTest();
}
