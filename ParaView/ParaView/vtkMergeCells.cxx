// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:  vtkMergeCells.cxx
  Language:  C++
  Date:    $Date$
  Version:   $Revision$ 

=========================================================================*/

#include "vtkMergeCells.h"

#include "vtkIdType.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkCollection.h"
#include <stdlib.h>
#include <algorithm>

vtkCxxRevisionMacro(vtkMergeCells, "1.2");
vtkStandardNewMacro(vtkMergeCells);

vtkCxxSetObjectMacro(vtkMergeCells, UnstructuredGrid, vtkUnstructuredGrid);

vtkMergeCells::vtkMergeCells()
{
  this->TotalCells = 0;
  this->TotalPoints = 0;

  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;

  this->GlobalIdArrayName = NULL;

  this->UnstructuredGrid = NULL;
  this->Inputs = vtkCollection::New();
}

vtkMergeCells::~vtkMergeCells()
{
  this->FreeLists();
  this->Inputs->Delete();
  this->Inputs = NULL;
  this->SetUnstructuredGrid(0);
}

void vtkMergeCells::FreeLists()
{
  if (this->GlobalIdArrayName){
    delete [] this->GlobalIdArrayName;
    this->GlobalIdArrayName = NULL;
  }

  this->GlobalIdMap.clear();
}


void vtkMergeCells::MergeDataSet(vtkDataSet *set)
{
  // Just add the ugrid to the list to merge later.
  this->Inputs->AddItem(set);
}



void vtkMergeCells::FinishInput(vtkDataSet *set, int inputId,
                                vtkDataSetAttributes::FieldList *ptList,
                                vtkDataSetAttributes::FieldList *cellList)
{
  int newId, nextCell;
  int *idMap;

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (!ugrid){
    vtkErrorMacro(<< "SetUnstructuredGrid first");
    return;
  }

  if ((this->TotalCells == 0) || (this->TotalPoints == 0)){
    vtkErrorMacro(<< "SetTotalCells and SetTotalPoints first");
    return;
  }

  vtkPointData *pointArrays = set->GetPointData();
  vtkCellData *cellArrays   = set->GetCellData();

  int numPoints = set->GetNumberOfPoints();
  int numCells  = set->GetNumberOfCells();

  if (numCells == 0) return;

  if (this->GlobalIdArrayName){
    idMap = this->MapPointsToIds(set);
  }
  else{
    idMap = NULL;
  }

  int nextPt = this->NumberOfPoints;

  vtkPoints *pts = ugrid->GetPoints();

  for (int ptId=0; ptId < numPoints; ptId++){

    if (idMap){

      newId = idMap[ptId];
    }
    else{

      newId = nextPt;
    }

    if (newId == nextPt){

      pts->SetPoint(nextPt, set->GetPoint(ptId));
      ugrid->GetPointData()->CopyData(*ptList, pointArrays, inputId, ptId, nextPt);

      nextPt++;
    }
  }

  vtkIdList *cellPoints = vtkIdList::New();
  cellPoints->Allocate(VTK_CELL_SIZE);

  for (int cellId=0; cellId < numCells; cellId++){

    set->GetCellPoints(cellId, cellPoints);

    for (int i=0; i < cellPoints->GetNumberOfIds(); i++){

      int id = cellPoints->GetId(i);

      if (idMap){

        newId = idMap[id];

      }
      else{

        newId = this->NumberOfPoints + id;
      }

      cellPoints->SetId(i, newId);
    }

    nextCell = ugrid->InsertNextCell(set->GetCellType(cellId), cellPoints);

    ugrid->GetCellData()->CopyData(*cellList, cellArrays, inputId, cellId, nextCell);
  }

  cellPoints->Delete();
  if (idMap) delete [] idMap;

  this->NumberOfPoints = nextPt;
  this->NumberOfCells = nextCell;

  return;
}

// Initializes the ouput given the set of inputs.
void vtkMergeCells::StartUGrid(vtkDataSetAttributes::FieldList *ptList,
                               vtkDataSetAttributes::FieldList *cellList)
{
  if ((this->TotalCells <= 0) || (this->TotalPoints <= 0)){
    vtkErrorMacro(<<
     "Must SetTotalCells and SetTotalPoints before starting to MergeDataSets");

    return;
  }

  // Putting this before list generation to try to solve a problem.
  vtkUnstructuredGrid *output = this->UnstructuredGrid;
  output->Initialize();
  output->Allocate(this->TotalCells); //allocate storage for geometry/topology
  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(this->TotalPoints);
  output->SetPoints(pts);
  pts->Delete();

  int numPts = 0;
  int numCells = 0;
  int firstPD=1;
  int firstCD=1;
  this->Inputs->InitTraversal();
  vtkDataSet *ds;
  while ( (ds = (vtkDataSet*)(this->Inputs->GetNextItemAsObject())) )
    {
    if ( ds->GetNumberOfPoints() <= 0 && ds->GetNumberOfCells() <= 0 )
      {
      continue; //no input, just skip
      }

    numPts += ds->GetNumberOfPoints();
    numCells += ds->GetNumberOfCells();

    if ( firstPD )
      {
      ptList->InitializeFieldList(ds->GetPointData());
      firstPD = 0;
      }
    else
      {
      ptList->IntersectFieldList(ds->GetPointData());
      }
      
    if ( firstCD )
      {
      cellList->InitializeFieldList(ds->GetCellData());
      firstCD = 0;
      }
    else
      {
      cellList->IntersectFieldList(ds->GetCellData());
      }
    }//for all inputs

  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to append!");
    return;
    }
  if (numPts > this->TotalPoints)
    {
    // With this approach, the number of points/cells
    // do not really need to be set ahead of time.
    vtkWarningMacro("NumberOfPoints Mismatch.");
    }  
  if (numCells > this->TotalCells)
    {
    // With this approach, the number of points/cells
    // do not really need to be set ahead of time.
    vtkWarningMacro("NumberOfCells Mismatch.");
    }  
  // Now can allocate arrays.
  output->GetPointData()->CopyAllocate(*ptList,numPts);
  output->GetCellData()->CopyAllocate(*cellList,numCells);
}

void vtkMergeCells::Finish()
{
  int numInputs = this->Inputs->GetNumberOfItems();
  vtkDataSetAttributes::FieldList ptList(numInputs);
  vtkDataSetAttributes::FieldList cellList(numInputs);

  // Allocate output.
  this->StartUGrid(&ptList, &cellList);

  vtkDataSet *input;
  int inputId = 0;
  this->Inputs->InitTraversal();
  while ( (input = (vtkDataSet*)(this->Inputs->GetNextItemAsObject())) )
    {
    this->FinishInput(input, inputId, &ptList, &cellList);
    ++inputId;
    }

  this->Inputs->RemoveAllItems();
  this->FreeLists();

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (this->NumberOfPoints < this->TotalPoints){

    // if we don't do this, ugrid->GetNumberOfPoints() gives
    //   the wrong value

    ugrid->GetPoints()->GetData()->Resize(this->NumberOfPoints);
  }

  ugrid->Squeeze();

  return;
}

int *vtkMergeCells::MapPointsToIds(vtkDataSet *set)
{
  vtkIntArray *ids = NULL;

  vtkDataArray *globalIds = 
    set->GetPointData()->GetScalars(this->GlobalIdArrayName);

  if (globalIds) ids = vtkIntArray::SafeDownCast(globalIds);

  if (!ids){
    vtkErrorMacro("global id array is not available");
    return NULL;
  }
  int npoints = set->GetNumberOfPoints();

  int *idMap = new int [npoints];

  int nextNewLocalId = this->GlobalIdMap.size();

  // map global point Ids to Ids in the new data set

  for (int i=0; i<npoints; i++){

    int globalId = ids->GetValue(i);

    vtkstd::pair<vtkstd::map<int, int>::iterator, bool> inserted =

      this->GlobalIdMap.insert(vtkstd::pair<int,int>(globalId, nextNewLocalId));

    if (inserted.second){

      // this is a new global node Id

      idMap[i] = nextNewLocalId;

      nextNewLocalId++; 
    }
    else{

      // a repeat, it was not inserted

      idMap[i] = inserted.first->second;
    }
  }
  return idMap;
}

void vtkMergeCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TotalCells: " << this->TotalCells << endl;
  os << indent << "TotalPoints: " << this->TotalPoints << endl;

  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;

  if (this->GlobalIdArrayName)
    {
    os << indent << "GlobalIdArrayName: " << this->GlobalIdArrayName << endl;
    }
  os << indent << "GlobalIdMap size: " << this->GlobalIdMap.size() << endl;

  os << indent << "UnstructuredGrid: " << this->UnstructuredGrid << endl;
}

