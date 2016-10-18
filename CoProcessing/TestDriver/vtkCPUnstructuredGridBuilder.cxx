/*=========================================================================

  Program:   ParaView
  Module:    vtkCPUnstructuredGridBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  this->UnstructuredGrid = 0;
  vtkUnstructuredGrid* UG = vtkUnstructuredGrid::New();
  this->SetUnstructuredGrid(UG);
  UG->Delete();
  this->IsGridModified = 1;
}

//----------------------------------------------------------------------------
vtkCPUnstructuredGridBuilder::~vtkCPUnstructuredGridBuilder()
{
  this->SetUnstructuredGrid(0);
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
    return 0;
  }

  fieldBuilder->BuildField(timeStep, time, this->UnstructuredGrid);

  this->IsGridModified = 0;
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
    return 0;
  }
  if (points != this->UnstructuredGrid->GetPoints())
  {
    this->UnstructuredGrid->SetPoints(points);
    this->IsGridModified = 1;
  }
  return 1;
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
  this->IsGridModified = 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkCPUnstructuredGridBuilder::InsertNextCell(int type, vtkIdType npts, vtkIdType* pts)
{
  // for efficiency this doesn't check if UnstructuredGrid is null
  this->IsGridModified = 1;
  return this->UnstructuredGrid->InsertNextCell(type, npts, pts);
}

//----------------------------------------------------------------------------
vtkIdType vtkCPUnstructuredGridBuilder::InsertNextCell(int type, vtkIdList* ptIds)
{
  // for efficiency this doesn't check if UnstructuredGrid is null
  this->IsGridModified = 1;
  return this->UnstructuredGrid->InsertNextCell(type, ptIds);
}

//----------------------------------------------------------------------------
void vtkCPUnstructuredGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UnstructuredGrid: " << this->UnstructuredGrid << "\n";
  os << indent << "IsGridModified: " << this->IsGridModified << "\n";
}
