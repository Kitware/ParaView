// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:  vtkExtractCells.cxx
  Language:  C++
  Date:    $Date$
  Version:   $Revision$ 

=========================================================================*/

#include "vtkExtractCells.h"

#include "vtkIdType.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include <algorithm>
#include <set>

vtkCxxRevisionMacro(vtkExtractCells, "1.3");
vtkStandardNewMacro(vtkExtractCells);

void vtkExtractCells::FreeCellList()
{
  this->SetCellList(NULL);
}
void vtkExtractCells::SetCellList(vtkIdList *l)
{
  if (this->CellList.size() > 0){

    this->CellList.erase( this->CellList.begin(), this->CellList.end());
  }

  if (l) this->AddCellList(l);
}
void vtkExtractCells::AddCellList(vtkIdList *l)
{
  if (l == NULL) return;

  int ncells = l->GetNumberOfIds();

  if (ncells == 0) return;

  for (int i=0; i<ncells; i++){

    this->CellList.insert(l->GetId(i));
  }

  this->Modified();

  return;
}
void vtkExtractCells::AddCellRange(int from, int to)
{
  if (to < from) return;

  for (int i=from; i <= to; i++){

    this->CellList.insert(i);
  }

  this->Modified();

  return;
}
void vtkExtractCells::Execute()
{
  int i, ii;

  vtkDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output= this->GetOutput();

  int numCellsInput = input->GetNumberOfCells();

  int numCells = this->CellList.size();

  if (numCells == numCellsInput){

    this->Copy();
    return;
  }

  vtkPointData *PD = input->GetPointData();
  vtkCellData *CD = input->GetCellData();

  if (numCells == 0){

    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.

    output->Allocate(1);

    output->GetPointData()->CopyAllocate(PD, VTK_CELL_SIZE);
    output->GetCellData()->CopyAllocate(CD, 1);

    vtkPoints *pts = vtkPoints::New();
    pts->SetNumberOfPoints(VTK_CELL_SIZE);

    output->SetPoints(pts);

    pts->Delete();

    return;
  }
  vtkPointData *newPD = output->GetPointData();
  vtkCellData *newCD  = output->GetCellData();

  vtkIdList *ptIdMap = reMapPointIds(input);

  int numPoints = ptIdMap->GetNumberOfIds();

  output->Allocate(numCells);

  newPD->CopyAllocate(PD, numPoints);

  newCD->CopyAllocate(CD, numCells);

  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(numPoints);

  for (i=0; i<numPoints; i++){

    vtkIdType oldId = ptIdMap->GetId(i);

    pts->SetPoint(i, input->GetPoint(oldId));

    newPD->CopyData(PD, oldId, i);
  }

  output->SetPoints(pts);

  vtkIdList *cellPoints = vtkIdList::New();

  vtkstd::set<int>::iterator cellPtr;

  for (cellPtr = this->CellList.begin(); cellPtr != this->CellList.end(); ++cellPtr){

    int cellId = *cellPtr; 

    input->GetCellPoints(cellId, cellPoints);

    for (ii=0; ii < cellPoints->GetNumberOfIds(); ii++){

      int oldId = cellPoints->GetId(ii);

      int newId = vtkExtractCells::findInSortedList(ptIdMap, oldId);

      cellPoints->SetId(ii, newId);
    }

    output->InsertNextCell(input->GetCellType(cellId), cellPoints);

    newCD->CopyData(CD, cellId, i);
  }

  cellPoints->Delete();
  ptIdMap->Delete();
  pts->Delete();

  output->Squeeze();

  return;
}
void vtkExtractCells::Copy()
{
  int i;

  vtkDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output= this->GetOutput();

  vtkUnstructuredGrid *inputGrid = vtkUnstructuredGrid::SafeDownCast(input);

  if (inputGrid){
    output->DeepCopy(inputGrid);
    return;
  }

  int numCells = input->GetNumberOfCells();

  vtkPointData *PD = input->GetPointData();
  vtkCellData *CD = input->GetCellData();

  vtkPointData *newPD = output->GetPointData();
  vtkCellData *newCD  = output->GetCellData();

  int numPoints = input->GetNumberOfPoints();

  output->Allocate(numCells);
  
  newPD->CopyAllocate(PD, numPoints);
  
  newCD->CopyAllocate(CD, numCells);
    
  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(numPoints);

  for (i=0; i<numPoints; i++){
    pts->SetPoint(i, input->GetPoint(i));
  } 

  newPD->DeepCopy(PD);

  output->SetPoints(pts);

  pts->Delete();

  vtkIdList *cellPoints = vtkIdList::New();
  
  for (int cellId=0; cellId < numCells; cellId++){

    input->GetCellPoints(cellId, cellPoints);
  
    output->InsertNextCell(input->GetCellType(cellId), cellPoints);
  }
  newCD->DeepCopy(CD);
    
  cellPoints->Delete();
    
  output->Squeeze();

  return;
}
int vtkExtractCells::findInSortedList(vtkIdList *idList, vtkIdType id)
{
  int numids = idList->GetNumberOfIds();

  if (numids < 8) return idList->IsId(id);

  int L, R, M;
  L=0; 
  R=numids-1;

  vtkIdType *ids = (vtkIdType *)idList->GetPointer(0);
  vtkIdType Id = (vtkIdType)id;

  int loc = -1;

  while (R > L){

    if (R == L+1){
      if (ids[R] == Id){
        loc = R;
      }
      else if (ids[L] == Id){
        loc = L;
      }
      break;
    }

    M = (R + L) / 2;

    if (ids[M] > Id){
      R = M;
      continue;
    } 
    else if (ids[M] < Id){
      L = M;
      continue;
    }
    else{
      loc = M;
      break;
    }
  }

  return loc;
}
vtkIdList *vtkExtractCells::reMapPointIds(vtkDataSet *grid)
{
  vtkstd::set<int> ptList;

  vtkIdList *ptIds = vtkIdList::New();

  vtkstd::set<int>::iterator cellPtr;

  for (cellPtr = this->CellList.begin(); cellPtr != this->CellList.end(); ++cellPtr){

    grid->GetCellPoints(*cellPtr, ptIds);

    int nIds = ptIds->GetNumberOfIds();

    vtkIdType *ptId = ptIds->GetPointer(0);

    for (int j=0; j<nIds; j++){
      ptList.insert(*ptId++);
    }
  }

  ptIds->SetNumberOfIds(ptList.size());

  vtkstd::set<int>::iterator pt;

  int idNum = 0;

  for (pt = ptList.begin(); pt != ptList.end(); ++pt){

    ptIds->SetId(idNum++, *pt); 
  }

  return ptIds;
}
void vtkExtractCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

