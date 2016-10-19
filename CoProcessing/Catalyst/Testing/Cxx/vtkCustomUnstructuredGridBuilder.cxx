/*=========================================================================

  Program:   ParaView
  Module:    vtkCustomUnstructuredGridBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCustomUnstructuredGridBuilder.h"

#include "vtkCPFieldBuilder.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkCustomUnstructuredGridBuilder);

//----------------------------------------------------------------------------
vtkCustomUnstructuredGridBuilder::vtkCustomUnstructuredGridBuilder()
{
}

//----------------------------------------------------------------------------
vtkCustomUnstructuredGridBuilder::~vtkCustomUnstructuredGridBuilder()
{
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCustomUnstructuredGridBuilder::GetGrid(
  unsigned long vtkNotUsed(timeStep), double vtkNotUsed(time), int& builtNewGrid)
{
  builtNewGrid = 0;
  if (this->IsGridModified)
  {
    this->BuildGrid();
    builtNewGrid = 1;
  }

  this->IsGridModified = 0;

  return this->GetUnstructuredGrid();
}

//----------------------------------------------------------------------------
void vtkCustomUnstructuredGridBuilder::BuildGrid()
{
  // I don't want to create an unstructured grid by hand so I'll create
  // a vtkUniformGrid and then build an UnstructuredGrid from that.
  vtkSmartPointer<vtkUniformGrid> uniformGrid = vtkSmartPointer<vtkUniformGrid>::New();
  double spacing[3] = { .2, .2, .3 };
  uniformGrid->SetSpacing(spacing);
  double origin[3] = { 20, 30, 30 };
  uniformGrid->SetOrigin(origin);
  int dimensions[3] = { 30, 30, 30 };
  uniformGrid->SetDimensions(dimensions);

  // Now create the vtkUnstructuredGrid from NewGrid.
  // First create the points/nodes of the grid.  I'll also
  // add in the point data while I'm at it.
  vtkPoints* points = vtkPoints::New();
  vtkIdType numberOfPoints = uniformGrid->GetNumberOfPoints();
  points->SetNumberOfPoints(numberOfPoints);
  double xyz[3];
  vtkDoubleArray* pointField = vtkDoubleArray::New();
  pointField->SetNumberOfComponents(3);
  pointField->SetNumberOfTuples(numberOfPoints);
  pointField->SetName("Velocity");
  double velocity[3];
  for (vtkIdType i = 0; i < numberOfPoints; i++)
  {
    // Use this GetPoint method because it is thread-safe.
    uniformGrid->GetPoint(i, xyz);
    points->SetPoint(i, xyz);
    // Just make up numbers for now.
    velocity[0] = xyz[0] * xyz[0];
    velocity[1] = xyz[1];
    velocity[2] = xyz[2] + 5.;
    pointField->SetTypedTuple(i, velocity);
  }
  this->SetPoints(points);
  points->Delete();
  this->GetUnstructuredGrid()->GetPointData()->AddArray(pointField);
  pointField->Delete();

  // Next add in the cells (assuming we don't know what type of cells).
  // I'll also add in cell data while I'm at this.  Note though that this
  // can be tricky as the cells are added in an order based on type and
  // not on calls to InsertNextCell.  Thus, I add the array to CellData
  // right away and let VTK take care of the details.
  vtkIdType numberOfCells = uniformGrid->GetNumberOfCells();
  this->Allocate(numberOfCells);
  vtkGenericCell* cell = vtkGenericCell::New();
  vtkDoubleArray* cellField = vtkDoubleArray::New();
  cellField->SetNumberOfComponents(1);
  cellField->SetName("Pressure");
  this->GetUnstructuredGrid()->GetCellData()->AddArray(cellField);
  cellField->Delete();
  for (vtkIdType i = 0; i < numberOfCells; i++)
  {
    // Use this GetCell method because it is thread-safe.
    uniformGrid->GetCell(i, cell);
    vtkIdType cellId = this->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
    // Set the pressure to the X value of the cell centroid.
    this->ComputeCellCentroid(cell, xyz);
    cellField->InsertTypedTuple(cellId, xyz);
  }
  cell->Delete();
}

//----------------------------------------------------------------------------
void vtkCustomUnstructuredGridBuilder::ComputeCellCentroid(vtkGenericCell* cell, double xyz[3])
{
  double pxyz[3];
  int subId = cell->GetParametricCenter(pxyz);
  double weights[100];
  cell->EvaluateLocation(subId, pxyz, xyz, weights);
}

//----------------------------------------------------------------------------
void vtkCustomUnstructuredGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
