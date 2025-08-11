// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPUnstructuredGridBuilder.h"

#include "vtkCPFieldBuilder.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkCPUnstructuredGridBuilder);
vtkCxxSetObjectMacro(vtkCPUnstructuredGridBuilder, UnstructuredGrid, vtkUnstructuredGrid);

//----------------------------------------------------------------------------
vtkCPUnstructuredGridBuilder::vtkCPUnstructuredGridBuilder()
{
  this->UnstructuredGrid = nullptr;
  vtkUnstructuredGrid* UG = vtkUnstructuredGrid::New();
  this->SetUnstructuredGrid(UG);
  UG->Delete();
  this->IsGridModified = true;
}

//----------------------------------------------------------------------------
vtkCPUnstructuredGridBuilder::~vtkCPUnstructuredGridBuilder()
{
  this->SetUnstructuredGrid(nullptr);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCPUnstructuredGridBuilder::GetGrid(
  unsigned long timeStep, double time, int& builtNewGrid)
{
  builtNewGrid = 0;
  vtkCPBaseFieldBuilder* fieldBuilder = this->GetFieldBuilder();
  if (!fieldBuilder)
  {
    vtkErrorMacro("FieldBuilder is not defined.");
    return nullptr;
  }

  fieldBuilder->BuildField(timeStep, time, this->UnstructuredGrid);

  this->IsGridModified = false;
  builtNewGrid = 1;
  return this->UnstructuredGrid;
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkCPUnstructuredGridBuilder::GetUnstructuredGrid()
{
  return this->UnstructuredGrid;
}

//----------------------------------------------------------------------------
bool vtkCPUnstructuredGridBuilder::SetPoints(vtkPoints* points)
{
  if (!points || !this->UnstructuredGrid)
  {
    vtkWarningMacro("Unable to set points.");
    return false;
  }
  if (points != this->UnstructuredGrid->GetPoints())
  {
    this->UnstructuredGrid->SetPoints(points);
    this->IsGridModified = true;
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkCPUnstructuredGridBuilder::Allocate(vtkIdType numCells, int extSize)
{
  if (!this->UnstructuredGrid)
  {
    vtkWarningMacro("UnstructuredGrid is NULL.");
    return;
  }
  this->UnstructuredGrid->Allocate(numCells, extSize);
  this->IsGridModified = true;
}

//----------------------------------------------------------------------------
vtkIdType vtkCPUnstructuredGridBuilder::InsertNextCell(int type, vtkIdType npts, vtkIdType* pts)
{
  // for efficiency this doesn't check if UnstructuredGrid is null
  this->IsGridModified = true;
  return this->UnstructuredGrid->InsertNextCell(type, npts, pts);
}

//----------------------------------------------------------------------------
vtkIdType vtkCPUnstructuredGridBuilder::InsertNextCell(int type, vtkIdList* ptIds)
{
  // for efficiency this doesn't check if UnstructuredGrid is null
  this->IsGridModified = true;
  return this->UnstructuredGrid->InsertNextCell(type, ptIds);
}

//----------------------------------------------------------------------------
void vtkCPUnstructuredGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UnstructuredGrid: " << this->UnstructuredGrid << "\n";
  os << indent << "IsGridModified: " << this->IsGridModified << "\n";
}
