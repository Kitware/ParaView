/*=========================================================================

  Program:   ParaView
  Module:    vtkMergeCells.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkMergeCells.h"

#include "vtkIdType.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <stdlib.h>
#include <algorithm>

vtkCxxRevisionMacro(vtkMergeCells, "1.8");
vtkStandardNewMacro(vtkMergeCells);

vtkCxxSetObjectMacro(vtkMergeCells, UnstructuredGrid, vtkUnstructuredGrid);

vtkMergeCells::vtkMergeCells()
{
  this->TotalNumberOfDataSets = 0;
  this->TotalCells = 0;
  this->TotalPoints = 0;

  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;

  this->GlobalIdArrayName = NULL;

  this->InputIsUGrid = 0;
  
  this->ptList = NULL;
  this->cellList = NULL;

  this->UnstructuredGrid = NULL;

  this->nextGrid = 0;
}

vtkMergeCells::~vtkMergeCells()
{
  this->FreeLists();
  this->SetUnstructuredGrid(0);
}

void vtkMergeCells::FreeLists()
{
  if (this->GlobalIdArrayName)
    {
    delete [] this->GlobalIdArrayName;
    this->GlobalIdArrayName = NULL;
    }

  this->GlobalIdMap.clear();

  if (this->ptList)
    {
    delete this->ptList;
    this->ptList = NULL;
    }

  if (this->cellList)
    {
    delete this->cellList;
    this->cellList = NULL;
    }
}


int vtkMergeCells::MergeDataSet(vtkDataSet *set)
{
  vtkIdType newPtId, oldPtId, newCellId;
  vtkIdType *idMap;

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (!ugrid)
    {
    vtkErrorMacro(<< "SetUnstructuredGrid first");
    return -1;
    }

  if ((this->TotalCells <= 0) || (this->TotalPoints <= 0) ||
      (this->TotalNumberOfDataSets <= 0))
    {
    vtkErrorMacro(<<
     "Must SetTotalCells, SetTotalPoints and SetTotalNumberOfDataSets (upper bounds at least)"
     " before starting to MergeDataSets");

    return -1;
    }

  vtkPointData *pointArrays = set->GetPointData();
  vtkCellData *cellArrays   = set->GetCellData();

  // Since vtkMergeCells is to be used only on distributed vtkDataSets,
  // each DataSet should have the same field arrays.  However I've been
  // told that the field arrays may get rearranged in the process of
  // Marshalling/UnMarshalling.  So we use a
  // vtkDataSetAttributes::FieldList to ensure the field arrays are
  // merged in the right order.

  if (ugrid->GetNumberOfCells() == 0)
    {
    vtkUnstructuredGrid *checkInput = vtkUnstructuredGrid::SafeDownCast(set);

    this->InputIsUGrid = (checkInput != NULL);

    this->StartUGrid(pointArrays, cellArrays);
    }
  else
    {
    this->ptList->IntersectFieldList(pointArrays);
    this->cellList->IntersectFieldList(cellArrays);
    }

  vtkIdType numPoints = set->GetNumberOfPoints();
  vtkIdType numCells  = set->GetNumberOfCells();

  if (numCells == 0) return 0;


  if (this->GlobalIdArrayName)
    {
    idMap = this->MapPointsToIds(set);
    }
  else
    {
    idMap = NULL;
    }

  vtkIdType nextPt = (vtkIdType)this->NumberOfPoints;

  vtkPoints *pts = ugrid->GetPoints();

  for (oldPtId=0; oldPtId < numPoints; oldPtId++)
    {
    if (idMap)
      {
      newPtId = idMap[oldPtId];
      }
    else 
      {
      newPtId = nextPt;
      }

    if (newPtId == nextPt)
      {
      pts->SetPoint(nextPt, set->GetPoint(oldPtId));

      ugrid->GetPointData()->CopyData(*this->ptList,
                           pointArrays, this->nextGrid, oldPtId, nextPt);

      nextPt++;
      }
    }

  if (this->InputIsUGrid)
    {
    newCellId = this->AddNewCellsUnstructuredGrid(set, idMap);
    }
  else
    {
    newCellId = this->AddNewCellsDataSet(set, idMap);
    }

  if (idMap) delete [] idMap;

  this->NumberOfPoints = nextPt;
  this->NumberOfCells = newCellId;

  this->nextGrid++;

  return 0;
}
vtkIdType vtkMergeCells::AddNewCellsDataSet(vtkDataSet *set, vtkIdType *idMap)
{
  vtkIdType oldCellId, id, newPtId, newCellId = 0, oldPtId;

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;
  vtkCellData *cellArrays = set->GetCellData();
  vtkIdType numCells      = set->GetNumberOfCells();

  vtkIdList *cellPoints = vtkIdList::New();
  cellPoints->Allocate(VTK_CELL_SIZE);

  for (oldCellId=0; oldCellId < numCells; oldCellId++)
    {
    set->GetCellPoints(oldCellId, cellPoints);

    for (id=0; id < cellPoints->GetNumberOfIds(); id++)
      {
      oldPtId = cellPoints->GetId(id);

      if (idMap)
        {
        newPtId = idMap[oldPtId];
        }
      else
        {
        newPtId = this->NumberOfPoints + oldPtId;
        }
      cellPoints->SetId(id, newPtId);
      }

    newCellId =
      (vtkIdType)ugrid->InsertNextCell(set->GetCellType(oldCellId), cellPoints);

    ugrid->GetCellData()->CopyData(*(this->cellList), cellArrays,
                                   this->nextGrid, oldCellId, newCellId);
    }

  cellPoints->Delete();

  return newCellId;
}
vtkIdType vtkMergeCells::AddNewCellsUnstructuredGrid(vtkDataSet *set,
                                                     vtkIdType *idMap)
{
  char firstSet = 0;

  if (this->nextGrid == 0) firstSet = 1;

  vtkUnstructuredGrid *newUgrid = vtkUnstructuredGrid::SafeDownCast(set);
  vtkUnstructuredGrid *Ugrid = this->UnstructuredGrid;

  // connectivity information for the new data set
      
  vtkCellArray *newCellArray = newUgrid->GetCells();
  vtkIdType *newCells = newCellArray->GetPointer();
  int *newLocs = newUgrid->GetCellLocationsArray()->GetPointer(0);
  unsigned char *newTypes = newUgrid->GetCellTypesArray()->GetPointer(0);
    
  int newNumCells = newUgrid->GetNumberOfCells();
  int newNumConnections = newCellArray->GetData()->GetNumberOfTuples();
    
  // connectivity for the merged ugrid so far
  
  vtkCellArray *cellArray = NULL;
  vtkIdType *cells = NULL;
  int *locs = NULL;
  unsigned char *types = NULL;

  int numCells = 0;
  int numConnections = 0;                            

  if (!firstSet)
    { 
    cellArray  = Ugrid->GetCells();
    cells = cellArray->GetPointer();
    locs = Ugrid->GetCellLocationsArray()->GetPointer(0);
    types = Ugrid->GetCellTypesArray()->GetPointer(0);;
  
    numCells         = Ugrid->GetNumberOfCells();
    numConnections    =  cellArray->GetData()->GetNumberOfTuples();
    }

  //  New output grid: merging of existing and incoming grids

  //           CELL ARRAY

  int totalNumCells = numCells + newNumCells;
  int totalNumConnections = numConnections + newNumConnections;

  vtkIdTypeArray *mergedcells = vtkIdTypeArray::New();
  mergedcells->SetNumberOfValues(totalNumConnections);

  if (!firstSet)
    {
    vtkIdType *idptr = mergedcells->GetPointer(0);
    memcpy(idptr, cells, sizeof(vtkIdType) * numConnections);
    }

  vtkCellArray *finalCellArray = vtkCellArray::New();
  finalCellArray->SetCells(totalNumCells, mergedcells);

  //           LOCATION ARRAY

  vtkIntArray *locationArray = vtkIntArray::New();
  locationArray->SetNumberOfValues(totalNumCells);

  int *iptr = locationArray->GetPointer(0);  // new output dataset

  if (!firstSet)
    {
    memcpy(iptr, locs, numCells * sizeof(int));   // existing set
    }
  iptr += numCells;

  memcpy(iptr, newLocs, newNumCells * sizeof(int));  // new set

  if (!firstSet)
    {
    for (int i=0; i<newNumCells; i++)
      {
      iptr[i] += numConnections;
      }
    }

  //           TYPE ARRAY

  vtkUnsignedCharArray *typeArray = vtkUnsignedCharArray::New();
  typeArray->SetNumberOfValues(totalNumCells);

  unsigned char *cptr = typeArray->GetPointer(0);

  if (!firstSet)
    {
    memcpy(cptr, types, numCells * sizeof(unsigned char));
    }
  cptr += numCells;
  
  memcpy(cptr, newTypes, newNumCells * sizeof(unsigned char));
  
  // set up new cell data
  
  vtkIdType newCellId = numCells;
  int nextCellArrayIndex = numConnections;
  vtkCellData *cellArrays = set->GetCellData();
  
  vtkIdType oldPtId, newPtId;
  
  for (vtkIdType oldCellId=0; oldCellId < newNumCells; oldCellId++)
    {
    vtkIdType size = *newCells++;
    
    mergedcells->SetValue(nextCellArrayIndex++, size);
    
    for (vtkIdType id=0; id < size; id++)
      {
      oldPtId = *newCells++;
      
      if (idMap)
        { 
        newPtId = idMap[oldPtId];
        }
      else
        {
        newPtId = this->NumberOfPoints + oldPtId;
        }
      
      mergedcells->SetValue(nextCellArrayIndex++, newPtId);
      }
    
    Ugrid->GetCellData()->CopyData(*(this->cellList), cellArrays,
                                   this->nextGrid, oldCellId, newCellId);
    
    newCellId++;
    }
  
  Ugrid->SetCells(typeArray, locationArray, finalCellArray);
  
  mergedcells->Delete();
  typeArray->Delete();
  locationArray->Delete();
  finalCellArray->Delete();

  return newCellId;
}

void vtkMergeCells::StartUGrid(vtkPointData *PD, vtkCellData *CD)
{
  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  ugrid->Initialize();

  if (!this->InputIsUGrid)
    {
    ugrid->Allocate(this->TotalCells);
    }

  vtkPoints *pts = vtkPoints::New();

  pts->SetNumberOfPoints(this->TotalPoints);  // upper bound

  ugrid->SetPoints(pts);

  pts->Delete();

  // Order of field arrays may get changed when data sets are
  // marshalled/sent/unmarshalled.  So we need to re-index the
  // field arrays before copying them using a FieldList

  this->ptList   = new vtkDataSetAttributes::FieldList(this->TotalNumberOfDataSets);
  this->cellList = new vtkDataSetAttributes::FieldList(this->TotalNumberOfDataSets);

  this->ptList->InitializeFieldList(PD);
  this->cellList->InitializeFieldList(CD);

  ugrid->GetPointData()->CopyAllocate(*ptList, this->TotalPoints);
  ugrid->GetCellData()->CopyAllocate(*cellList, this->TotalCells);

  return;
}

void vtkMergeCells::Finish()
{
  this->FreeLists();

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (this->NumberOfPoints < this->TotalPoints)
    {
    // if we don't do this, ugrid->GetNumberOfPoints() gives
    //   the wrong value

    ugrid->GetPoints()->GetData()->Resize(this->NumberOfPoints);
    }

  ugrid->Squeeze();

  return;
}

vtkIdType *vtkMergeCells::MapPointsToIds(vtkDataSet *set)
{
  vtkDataArray *globalIds = 
    set->GetPointData()->GetScalars(this->GlobalIdArrayName);

  if (!globalIds)
    {
    vtkErrorMacro("global id array is not available");
    return NULL;
    }

  vtkIdType npoints = set->GetNumberOfPoints();

  vtkIdType *idMap = new vtkIdType [npoints];

  vtkIdType nextNewLocalId = this->GlobalIdMap.size();

  // map global point Ids to Ids in the new data set

  for (vtkIdType oldId=0; oldId<npoints; oldId++)
    {
    double *id = globalIds->GetTuple(oldId);
    vtkIdType globalId = (vtkIdType)*id;

    vtkstd::pair<vtkstd::map<vtkIdType, vtkIdType>::iterator, bool> inserted =

      this->GlobalIdMap.insert(
         vtkstd::pair<vtkIdType,vtkIdType>(globalId, nextNewLocalId));

    if (inserted.second)
      {
      // this is a new global node Id

      idMap[oldId] = nextNewLocalId;

      nextNewLocalId++; 
      }
    else
      {
      // a repeat, it was not inserted

      idMap[oldId] = inserted.first->second;
      }
    }

  return idMap;
}

void vtkMergeCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TotalNumberOfDataSets: " << this->TotalNumberOfDataSets << endl;
  os << indent << "TotalCells: " << this->TotalCells << endl;
  os << indent << "TotalPoints: " << this->TotalPoints << endl;

  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;

  if (this->GlobalIdArrayName)
    {
    os << indent << "GlobalIdArrayName: " << this->GlobalIdArrayName << endl;
    }
  os << indent << "GlobalIdMap size: " << this->GlobalIdMap.size() << endl;

  os << indent << "InputIsUGrid: " << this->InputIsUGrid << endl;
  os << indent << "UnstructuredGrid: " << this->UnstructuredGrid << endl;
  os << indent << "ptList: " << this->ptList << endl;
  os << indent << "cellList: " << this->cellList << endl;
}

