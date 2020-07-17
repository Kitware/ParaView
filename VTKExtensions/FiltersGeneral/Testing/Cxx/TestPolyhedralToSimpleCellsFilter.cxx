/*=========================================================================

  Program:   ParaView
  Module:    TestPolyhedralToSimpleCellsFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2020 Menno Deij - van Rijswijk (MARIN)
-------------------------------------------------------------------------*/

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
  if (o->GetError())                                                                               \
    return EXIT_FAILURE;

class TestConvertPolyhedra
{
public:
  static int DoTest()
  {
    vtkNew<vtkConvertPolyhedraFilter> r;
    vtkNew<vtkUnstructuredGrid> ug;
    vtkNew<vtkIdList> ids;

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
    auto arr = new vtkIdType[17]{
      4,          // nr faces
      3, 0, 1, 2, // side
      3, 1, 2, 3, // side
      3, 1, 3, 0, // side
      3, 0, 2, 3  // side
    };

    ids->SetArray(arr, 17);
    r->InsertNextPolyhedralCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    vtk_assert(ug->GetCellType(0) == VTK_TETRA);

    // === VTK_PYRAMID === //
    ug->Reset();
    arr = new vtkIdType[22]{
      5,            // nr faces
      3, 0, 1, 4,   // side
      3, 4, 1, 2,   // side
      3, 2, 3, 4,   // side
      3, 3, 4, 0,   // side
      4, 0, 1, 2, 3 // bottom
    };

    ids->Reset();
    ids->SetArray(arr, 22);

    r->InsertNextPolyhedralCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    int ct = ug->GetCellType(0);
    vtk_assert(ct == VTK_PYRAMID);

    // === VTK_WEDGE === //
    ug->Reset();
    arr = new vtkIdType[24]{
      5,             // nr faces
      3, 0, 1, 2,    // top
      4, 0, 3, 4, 1, // side
      4, 3, 5, 2, 0, // side
      4, 2, 5, 4, 1, // side
      3, 3, 5, 4     // bottom
    };

    ids->Reset();
    ids->SetArray(arr, 24);

    r->InsertNextPolyhedralCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    ct = ug->GetCellType(0);
    vtk_assert(ct == VTK_WEDGE);

    // === VTK_HEXAHEDRON === //
    ug->Reset();
    arr = new vtkIdType[31]{ 6, // nr faces
      4, 0, 1, 2, 3,            // bottom
      4, 2, 6, 5, 1,            // side
      4, 5, 4, 0, 1,            // side
      4, 4, 5, 6, 7,            // top
      4, 3, 7, 6, 2,            // side
      4, 7, 3, 0, 4 };

    ids->Reset();
    ids->SetArray(arr, 31);

    r->InsertNextPolyhedralCell(ug, ids);
    vtk_observe_has_no_error(obs);
    vtk_assert(ug->GetNumberOfCells() == 1);
    ct = ug->GetCellType(0);
    vtk_assert(ct == VTK_HEXAHEDRON);

    r->RemoveObserver(obs);

    return EXIT_SUCCESS;
  }
};

int TestPolyhedralToSimpleCellsFilter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // test is inside class such that friend declaration in
  // filter can work
  return TestConvertPolyhedra::DoTest();
}
