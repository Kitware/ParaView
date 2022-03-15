#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

void Create(vtkPolyData* polyData, int rank, int size)
{

  polyData->Allocate(2);
  vtkNew<vtkPoints> points;
  vtkNew<vtkDoubleArray> vertexPressure;
  vtkNew<vtkDoubleArray> cellPressure;
  vtkNew<vtkDoubleArray> vertexVelocity;
  vtkNew<vtkDoubleArray> cellVelocity;

  vertexVelocity->SetNumberOfComponents(3);
  cellVelocity->SetNumberOfComponents(3);

  points->Allocate(8);

  points->InsertNextPoint(rank + 0, rank + 0, rank + 0);
  points->InsertNextPoint(rank + 0, rank + 1, rank + 0);
  points->InsertNextPoint(rank + 1, rank + 1, rank + 0);
  points->InsertNextPoint(rank + 1, rank + 0, rank + 0);

  points->InsertNextPoint(rank + 0, rank + 0, rank + 1);
  points->InsertNextPoint(rank + 0, rank + 1, rank + 1);
  points->InsertNextPoint(rank + 1, rank + 1, rank + 1);
  points->InsertNextPoint(rank + 1, rank + 0, rank + 1);

  for (int i = 0; i < 8; ++i)
  {
    vertexPressure->InsertNextValue(i);
    vertexVelocity->InsertNextTuple3(i, i, i);
  }

  polyData->SetPoints(points);

  if (rank % 2 == 0)
  {
    vtkIdType ids[] = { 0, 1, 2, 3 };
    polyData->InsertNextCell(VTK_QUAD, 4, ids);
    cellPressure->InsertNextValue(rank);
    cellVelocity->InsertNextTuple3(rank, rank, rank);
  }
  if (rank % 2 == 1 || size == 1)
  {
    vtkIdType ids[] = { 0, 1, 2 };
    polyData->InsertNextCell(VTK_TRIANGLE, 3, ids);
    cellPressure->InsertNextValue(rank);
    cellVelocity->InsertNextTuple3(rank, rank, rank);
  }

  vertexPressure->SetName("Pressure");
  cellPressure->SetName("Pressure");
  vertexVelocity->SetName("Velocity");
  cellVelocity->SetName("Velocity");

  polyData->GetPointData()->AddArray(vertexPressure);
  polyData->GetCellData()->AddArray(cellPressure);
  polyData->GetPointData()->AddArray(vertexVelocity);
  polyData->GetCellData()->AddArray(cellVelocity);
}

void CreatePolygonal(vtkPolyData* polyData, int rank)
{
  vtkNew<vtkPoints> points;
  polyData->Allocate(1);
  polyData->SetPoints(points);

  points->InsertNextPoint(0, 0, rank);
  points->InsertNextPoint(1, 0, rank);
  points->InsertNextPoint(2, 1, rank);
  points->InsertNextPoint(1, 2, rank);
  points->InsertNextPoint(0, 2, rank);

  vtkNew<vtkIdList> polygon;
  polygon->Allocate(10);

  polygon->InsertNextId(0);
  polygon->InsertNextId(1);
  polygon->InsertNextId(2);
  polygon->InsertNextId(3);
  polygon->InsertNextId(4);

  polyData->InsertNextCell(VTK_POLYGON, polygon);

  vtkNew<vtkDoubleArray> pressure, velocity;
  pressure->SetName("Pressure");
  velocity->SetName("Velocity");

  velocity->SetNumberOfComponents(3);
  pressure->InsertNextValue(rank);
  velocity->InsertNextTuple3(rank, rank, rank);

  polyData->GetCellData()->AddArray(pressure);
  polyData->GetCellData()->AddArray(velocity);
}

void CreatePolyhedral(vtkUnstructuredGrid* polyhedral, int rank)
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
  vtkNew<vtkPoints> points;

  polyhedral->SetPoints(points);

  points->InsertNextPoint(0, 0, 2 * rank + 0);
  points->InsertNextPoint(1, 0, 2 * rank + 0);
  points->InsertNextPoint(1, 1, 2 * rank + 0);
  points->InsertNextPoint(1, 2, 2 * rank + 0);
  points->InsertNextPoint(0, 2, 2 * rank + 0);

  points->InsertNextPoint(0, 0, 2 * rank + 1);
  points->InsertNextPoint(1, 0, 2 * rank + 1);
  points->InsertNextPoint(1, 1, 2 * rank + 1);
  points->InsertNextPoint(1, 2, 2 * rank + 1);
  points->InsertNextPoint(0, 2, 2 * rank + 1);

  points->InsertNextPoint(2, 0, 2 * rank + 0);
  points->InsertNextPoint(2, 1, 2 * rank + 0);
  points->InsertNextPoint(2, 2, 2 * rank + 0);

  points->InsertNextPoint(2, 0, 2 * rank + 1);
  points->InsertNextPoint(2, 1, 2 * rank + 1);
  points->InsertNextPoint(2, 2, 2 * rank + 1);

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

  polyhedral->InsertNextCell(VTK_POLYHEDRON, polyhedron);

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

  polyhedral->InsertNextCell(VTK_POLYHEDRON, polyhedron);
}

void Create(vtkUnstructuredGrid* unstructuredGrid, int rank, int size)
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkDoubleArray> vertexPressure;
  vtkNew<vtkDoubleArray> cellPressure;
  vtkNew<vtkDoubleArray> vertexVelocity;
  vtkNew<vtkDoubleArray> cellVelocity;

  vertexVelocity->SetNumberOfComponents(3);
  cellVelocity->SetNumberOfComponents(3);

  points->Allocate(8);

  points->InsertNextPoint(rank + 0, rank + 0, rank + 0);
  points->InsertNextPoint(rank + 0, rank + 1, rank + 0);
  points->InsertNextPoint(rank + 1, rank + 1, rank + 0);
  points->InsertNextPoint(rank + 1, rank + 0, rank + 0);

  points->InsertNextPoint(rank + 0, rank + 0, rank + 1);
  points->InsertNextPoint(rank + 0, rank + 1, rank + 1);
  points->InsertNextPoint(rank + 1, rank + 1, rank + 1);
  points->InsertNextPoint(rank + 1, rank + 0, rank + 1);

  for (int i = 0; i < 8; ++i)
  {
    vertexPressure->InsertNextValue(i);
    vertexVelocity->InsertNextTuple3(i, i, i);
  }

  unstructuredGrid->SetPoints(points);

  if (rank % 2 == 0)
  {
    vtkIdType ids[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, ids);
    cellPressure->InsertNextValue(rank);
    cellVelocity->InsertNextTuple3(rank, rank, rank);
  }
  if (rank % 2 == 1 || size == 1)
  {
    vtkIdType ids[] = { 0, 1, 2, 5 };
    unstructuredGrid->InsertNextCell(VTK_TETRA, 4, ids);
    cellPressure->InsertNextValue(rank);
    cellVelocity->InsertNextTuple3(rank, rank, rank);
  }

  vertexPressure->SetName("Pressure");
  cellPressure->SetName("Pressure");
  vertexVelocity->SetName("Velocity");
  cellVelocity->SetName("Velocity");

  unstructuredGrid->GetPointData()->AddArray(vertexPressure);
  unstructuredGrid->GetCellData()->AddArray(cellPressure);
  unstructuredGrid->GetPointData()->AddArray(vertexVelocity);
  unstructuredGrid->GetCellData()->AddArray(cellVelocity);
}

void CreatePartial(vtkUnstructuredGrid* polyhedral, int rank)
{
  vtkNew<vtkPoints> points;
  polyhedral->SetPoints(points);

  points->InsertNextPoint(0, 0, 2 * rank + 0);
  points->InsertNextPoint(1, 0, 2 * rank + 0);
  points->InsertNextPoint(1, 1, 2 * rank + 0);
  points->InsertNextPoint(1, 2, 2 * rank + 0);
  points->InsertNextPoint(0, 2, 2 * rank + 0);

  points->InsertNextPoint(0, 0, 2 * rank + 1);
  points->InsertNextPoint(1, 0, 2 * rank + 1);
  points->InsertNextPoint(1, 1, 2 * rank + 1);
  points->InsertNextPoint(1, 2, 2 * rank + 1);
  points->InsertNextPoint(0, 2, 2 * rank + 1);

  points->InsertNextPoint(2 + 0, 0, 2 * rank + 0);
  points->InsertNextPoint(2 + 1, 0, 2 * rank + 0);
  points->InsertNextPoint(2 + 1, 2, 2 * rank + 0);
  points->InsertNextPoint(2 + 0, 2, 2 * rank + 0);
  points->InsertNextPoint(2 + 0, 1, 2 * rank + 0);

  points->InsertNextPoint(2 + 0, 0, 2 * rank + 1);
  points->InsertNextPoint(2 + 1, 0, 2 * rank + 1);
  points->InsertNextPoint(2 + 1, 2, 2 * rank + 1);
  points->InsertNextPoint(2 + 0, 2, 2 * rank + 1);
  points->InsertNextPoint(2 + 0, 1, 2 * rank + 1);

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

  polyhedral->InsertNextCell(VTK_POLYHEDRON, polyhedron);

  polyhedron->Reset();

  polyhedron->InsertNextId(7); // faces

  polyhedron->InsertNextId(4); // face0-4verts
  polyhedron->InsertNextId(10);
  polyhedron->InsertNextId(11);
  polyhedron->InsertNextId(16);
  polyhedron->InsertNextId(15);

  polyhedron->InsertNextId(4); // face1-4verts
  polyhedron->InsertNextId(11);
  polyhedron->InsertNextId(12);
  polyhedron->InsertNextId(17);
  polyhedron->InsertNextId(16);

  polyhedron->InsertNextId(4); // face2-4verts
  polyhedron->InsertNextId(12);
  polyhedron->InsertNextId(13);
  polyhedron->InsertNextId(18);
  polyhedron->InsertNextId(17);

  polyhedron->InsertNextId(4); // face3-4verts
  polyhedron->InsertNextId(13);
  polyhedron->InsertNextId(14);
  polyhedron->InsertNextId(19);
  polyhedron->InsertNextId(18);

  polyhedron->InsertNextId(4); // face4-4verts
  polyhedron->InsertNextId(10);
  polyhedron->InsertNextId(14);
  polyhedron->InsertNextId(19);
  polyhedron->InsertNextId(15);

  polyhedron->InsertNextId(5); // face5-5verts
  polyhedron->InsertNextId(10);
  polyhedron->InsertNextId(11);
  polyhedron->InsertNextId(12);
  polyhedron->InsertNextId(13);
  polyhedron->InsertNextId(14);

  polyhedron->InsertNextId(5); // face6-5verts
  polyhedron->InsertNextId(15);
  polyhedron->InsertNextId(16);
  polyhedron->InsertNextId(17);
  polyhedron->InsertNextId(18);
  polyhedron->InsertNextId(19);

  polyhedral->InsertNextCell(VTK_POLYHEDRON, polyhedron);
}
