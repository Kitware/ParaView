/*=========================================================================

  Program:   ParaView
  Module:    vtkCleanUnstructuredGrid.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCleanUnstructuredGrid.h"

#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkCollection.h"
#include "vtkPointLocator.h"

vtkCxxRevisionMacro(vtkCleanUnstructuredGrid, "1.6");
vtkStandardNewMacro(vtkCleanUnstructuredGrid);

//----------------------------------------------------------------------------
vtkCleanUnstructuredGrid::vtkCleanUnstructuredGrid()
{
  this->Locator = vtkPointLocator::New();
}

//----------------------------------------------------------------------------
vtkCleanUnstructuredGrid::~vtkCleanUnstructuredGrid()
{
  this->Locator->Delete();
  this->Locator = NULL;
}

//----------------------------------------------------------------------------
void vtkCleanUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCleanUnstructuredGrid::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output= this->GetOutput();

  if (input->GetNumberOfCells() == 0)
    {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.
    output->Allocate(1);
    output->GetPointData()->CopyAllocate(input->GetPointData(), VTK_CELL_SIZE);
    output->GetCellData()->CopyAllocate(input->GetCellData(), 1);
    vtkPoints *pts = vtkPoints::New();
    pts->SetNumberOfPoints(VTK_CELL_SIZE);
    output->SetPoints(pts);
    pts->Delete();
    return;
  }


  output->GetPointData()->CopyAllocate(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // First, create a new points array that eliminate duplicate points.
  // Also create a mapping from the old point id to the new.
  vtkPoints* newPts = vtkPoints::New();
  vtkIdType num = input->GetNumberOfPoints();
  vtkIdType id;
  vtkIdType newId;
  vtkIdType* ptMap = new vtkIdType[num];
  double pt[3];

  this->Locator->SetTolerance(0.00001*input->GetLength());
  this->Locator->InitPointInsertion(newPts, input->GetBounds(), num);

  for (id = 0; id < num; ++id)
    {
    input->GetPoint(id, pt);
    if (this->Locator->InsertUniquePoint(pt, newId))
      {
      output->GetPointData()->CopyData(input->GetPointData(),id,newId);
      }
    ptMap[id] = newId;
    }
  output->SetPoints(newPts);
  newPts->Delete();

  // New copy the cells.
  vtkIdList *cellPoints = vtkIdList::New();
  num = input->GetNumberOfCells();
  output->Allocate(num);
  for (id = 0; id < num; ++id)
    {
    input->GetCellPoints(id, cellPoints);
    for (int i=0; i < cellPoints->GetNumberOfIds(); i++)
      {
      int cellPtId = cellPoints->GetId(i);
      newId = ptMap[cellPtId];
      cellPoints->SetId(i, newId);
      }
    output->InsertNextCell(input->GetCellType(id), cellPoints);
    }

  delete [] ptMap;
  cellPoints->Delete();
  output->Squeeze();
}
