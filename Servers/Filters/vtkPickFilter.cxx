/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickFilter.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiProcessController.h"
#include "vtkCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#endif

vtkCxxRevisionMacro(vtkPickFilter, "1.6");
vtkStandardNewMacro(vtkPickFilter);
vtkCxxSetObjectMacro(vtkPickFilter,Controller,vtkMultiProcessController);



//-----------------------------------------------------------------------------
vtkPickFilter::vtkPickFilter ()
{
  this->PickCell = 0;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->WorldPoint[0] = this->WorldPoint[1] = this->WorldPoint[2] = 0.0;

  this->PointMap = 0;
  this->RegionPointIds = 0;
  this->BestInputIndex = -1;
}

//-----------------------------------------------------------------------------
vtkPickFilter::~vtkPickFilter ()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkPickFilter::GetInput(int idx)
{
  if (this->NumberOfInputs <= idx)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPickFilter::AddInput(vtkDataSet *input)
{
  int i = this->GetNumberOfInputs();
  this->vtkProcessObject::SetNthInput(i, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkPickFilter::RemoveAllInputs()
{
  int num, idx;
  num = this->NumberOfInputs;
  for (idx = 0; idx < num; ++idx)
    {
    this->SetNthInput(idx, NULL);
    }
}

//-----------------------------------------------------------------------------
void vtkPickFilter::Execute()
{
  this->BestInputIndex = -1;

  if (this->PickCell)
    {
    this->CellExecute();
    }
  else
    {
    this->PointExecute();
    }

  this->DeletePointMap();
}

//-----------------------------------------------------------------------------
void vtkPickFilter::PointExecute()
{
  double pt[3];
  double distance2;
  double bestPt[3];
  double bestDistance2;
  vtkIdType bestId = 0;
  double tmp;
  int numInputs = this->GetNumberOfInputs();
  int inputIdx;
  vtkDataSet* input;
  vtkIdType numPts, ptId;

  if (numInputs == 0)
    {
    return;
    }

  // Find the nearest point in the input.
  bestDistance2 = VTK_LARGE_FLOAT;
  this->BestInputIndex = -1;

  for (inputIdx = 0; inputIdx < numInputs; ++inputIdx)
    {
    input = this->GetInput(inputIdx);
    numPts = input->GetNumberOfPoints();
    for (ptId = 0; ptId < numPts; ++ptId)
      {
      input->GetPoint(ptId, pt);
      tmp = pt[0]-this->WorldPoint[0];
      distance2 = tmp*tmp;
      tmp = pt[1]-this->WorldPoint[1];
      distance2 += tmp*tmp;
      tmp = pt[2]-this->WorldPoint[2];
      distance2 += tmp*tmp;
      if (distance2 < bestDistance2)
        {
        bestId = ptId;
        this->BestInputIndex = inputIdx;
        bestDistance2 = distance2;
        bestPt[0] = pt[0];
        bestPt[1] = pt[1];
        bestPt[2] = pt[2];
        }
      }
    }

  // Keep only the best seed among the processes.
  vtkIdList* regionCellIds = vtkIdList::New();
  if ( ! this->CompareProcesses(bestDistance2) && numPts > 0)
    {
    // Only one point in map.
    this->InitializePointMap(1);
    this->InsertIdInPointMap(bestId);
    }

  this->CreateOutput(regionCellIds);
  regionCellIds->Delete();
}

//-----------------------------------------------------------------------------
void vtkPickFilter::CellExecute()
{
  // Loop over all of the cells.
  vtkDataSet* input;
  int numInputs, inputIdx;
  vtkIdType cellId;
  vtkCell* cell;
  int inside;
  double closestPoint[3];
  int  subId;
  double pcoords[3];
  double dist2;
  double bestDist2 = VTK_LARGE_FLOAT;
  double* weights;
  vtkIdType numCells;
  vtkIdType bestId = -1;

  numInputs = this->NumberOfInputs;
  if (numInputs == 0)
    {
    return;
    }

  for (inputIdx = 0; inputIdx < numInputs; ++inputIdx)
    {
    input = this->GetInput(inputIdx);
    weights = new double[input->GetMaxCellSize()];
    numCells = input->GetNumberOfCells();
    for (cellId=0; cellId < numCells; cellId++)
      {
      cell = input->GetCell(cellId);
      inside = cell->EvaluatePosition(this->WorldPoint, closestPoint, 
                                      subId, pcoords, dist2, weights);
      // Inside does not work the way I thought for 2D cells.
      //if (inside)
      //  {
      //  dist2= 0.0;
      //  }
      if (dist2 < bestDist2)
        {
        bestId = cellId;
        bestDist2 = dist2;
        this->BestInputIndex = inputIdx;
        }
      }
    delete [] weights;
    weights = NULL;
    }

  // Keep only the best seed cell among the processes.
  vtkIdList* regionCellIds = vtkIdList::New();
  if ( ! this->CompareProcesses(bestDist2) && bestId >= 0)
    {
    input = this->GetInput(this->BestInputIndex);
    this->InitializePointMap(input->GetNumberOfPoints());
    regionCellIds->InsertNextId(bestId);
    // Insert the cell points.
    vtkIdList* cellPtIds = vtkIdList::New();
    input->GetCellPoints(bestId, cellPtIds);
    vtkIdType i;
    for (i = 0; i < cellPtIds->GetNumberOfIds(); ++i)
      {
      this->InsertIdInPointMap(cellPtIds->GetId(i));
      }
    cellPtIds->Delete();
    }

  this->CreateOutput(regionCellIds);
  regionCellIds->Delete();
}

//-----------------------------------------------------------------------------
vtkIdType vtkPickFilter::InsertIdInPointMap(vtkIdType inId)
{
  vtkIdType outId;
  outId = this->PointMap->GetId(inId);
  if (outId >= 0)
    {
    return outId;
    }
  outId = this->RegionPointIds->GetNumberOfIds();
  this->PointMap->SetId(inId, outId);
  this->RegionPointIds->InsertNextId(inId);
  return outId;
}

//-----------------------------------------------------------------------------
void vtkPickFilter::InitializePointMap(vtkIdType numberOfInputPoints)
{
  if (this->PointMap)
    {
    this->DeletePointMap();
    }
  this->PointMap = vtkIdList::New();
  this->PointMap->Allocate(numberOfInputPoints);
  this->RegionPointIds = vtkIdList::New();

  vtkIdType i;
  for (i = 0; i < numberOfInputPoints; ++i)
    {
    this->PointMap->InsertId(i, -1);
    }
}

//-----------------------------------------------------------------------------
void vtkPickFilter::DeletePointMap()
{
  if (this->PointMap)
    {
    this->PointMap->Delete();
    this->PointMap = NULL;
    }
  if (this->RegionPointIds)
    {
    this->RegionPointIds->Delete();
    this->RegionPointIds = NULL;
    }
}

//-----------------------------------------------------------------------------
int vtkPickFilter::CompareProcesses(double bestDist2)
{
  if (this->Controller == NULL)
    {
    return 0;
    }

  double dist2;
  int bestProc = 0;
  // Every process send their best distance to process 0.
  int myId = this->Controller->GetLocalProcessId();
  if (myId == 0)
    {
    int numProcs = this->Controller->GetNumberOfProcesses();
    int idx;
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Receive(&dist2, 1 ,idx, 234099);
      if (dist2 < bestDist2)
        {
        bestDist2 = dist2;
        bestProc = idx;
        }
      }
    // Send the result back to all the processes.
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Send(&bestProc, 1, idx, 234100);
      }
    }
  else
    { // Other processes.
    this->Controller->Send(&bestDist2, 1, 0, 234099);
    this->Controller->Receive(&bestProc, 1, 0, 234100);
    }
  if (myId != bestProc)
    { // Return without creating an output.
    return 1;
    }

  return 0;
}

//-----------------------------------------------------------------------------
// I made this general so we could grow the region from the seed.
void vtkPickFilter::CreateOutput(vtkIdList* regionCellIds)
{
  if (this->BestInputIndex < 0)
    {
    return;
    }
  vtkDataSet* input = this->GetInput(this->BestInputIndex);
  vtkUnstructuredGrid* output = this->GetOutput();
  double pt[3];
  // Preserve the original Ids.
  vtkIdTypeArray* cellIds = vtkIdTypeArray::New();
  vtkIdTypeArray* ptIds = vtkIdTypeArray::New();

  // First copy the points.
  vtkPoints* newPoints = vtkPoints::New();
  vtkIdType numPts, outId, inId;
  numPts = this->RegionPointIds->GetNumberOfIds();
  newPoints->Allocate(numPts);
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPts);
  ptIds->Allocate(numPts);
  for (outId = 0; outId < numPts; ++outId)
    {
    inId = this->RegionPointIds->GetId(outId);
    ptIds->InsertNextValue(inId);
    input->GetPoint(inId, pt);
    newPoints->InsertNextPoint(pt[0], pt[1], pt[2]);
    output->GetPointData()->CopyData(input->GetPointData(), inId, outId);
    }
  output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = NULL;

  // Now copy the cells.
  vtkIdList* inCellPtIds = vtkIdList::New();
  vtkIdList* outCellPtIds = vtkIdList::New();    
  vtkIdType numCells = regionCellIds->GetNumberOfIds();
  output->Allocate(numCells);
  cellIds->Allocate(numCells);
  output->GetCellData()->CopyAllocate(input->GetCellData(), numCells);
  vtkIdType num, i;
  for (outId = 0; outId < numCells; ++outId)
    {
    inId = regionCellIds->GetId(outId);
    cellIds->InsertNextValue(inId);
    input->GetCellPoints(inId, inCellPtIds);
    // Translate the cell to output point ids.
    num = inCellPtIds->GetNumberOfIds();
    outCellPtIds->Initialize();
    outCellPtIds->Allocate(num);
    for (i = 0; i < num; ++i)
      {
      outCellPtIds->InsertId(i, this->PointMap->GetId(inCellPtIds->GetId(i)));
      }
    output->InsertNextCell(input->GetCellType(inId), outCellPtIds);
    output->GetCellData()->CopyData(input->GetCellData(), inId, outId);
    }

  inCellPtIds->Delete();
  outCellPtIds->Delete();

  cellIds->SetName("Id");
  output->GetCellData()->AddArray(cellIds);
  cellIds->Delete();
  cellIds = NULL;
  ptIds->SetName("Id");
  output->GetPointData()->AddArray(ptIds);
  ptIds->Delete();
  ptIds = NULL;
}


//-----------------------------------------------------------------------------
int vtkPickFilter::ListContainsId(vtkIdList* ids, vtkIdType id)
{
  vtkIdType i, num;

  // Although this test causes a n^2 cost, the regions will be small.
  // The alternative is to have a table based on the input cell id.
  // Since inputs can be very large, the memory cost would be high.
  num = ids->GetNumberOfIds();
  for (i = 0; i < num; ++i)
    {
    if (id == ids->GetId(i))
      {
      return 1;
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkPickFilter::FindPointId(double pt[3], vtkDataSet* input)
{
  double bounds[6];
  double pt2[3];
  double tol;
  double xMin, xMax, yMin, yMax, zMin, zMax;
  //int fixme;  // make a fast version for image and rectilinear grid.
  vtkIdType i, num;

  input->GetBounds(bounds);
  tol = (bounds[5]-bounds[4])+(bounds[3]-bounds[2])+(bounds[1]-bounds[0]);
  tol *= 0.0000001;
  xMin = pt[0]-tol;
  xMax = pt[0]+tol;
  yMin = pt[1]-tol;
  yMax = pt[1]+tol;
  zMin = pt[2]-tol;
  zMax = pt[2]+tol;
  num = input->GetNumberOfPoints();
  for (i = 0; i < num; ++i)
    {
    input->GetPoint(i, pt2);
    if (pt2[0] > xMin && pt2[0] < xMax && 
        pt2[1] > yMin && pt2[1] < yMax && 
        pt2[2] > zMin && pt2[2] < zMax) 
      {
      return i;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkPickFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WorldPoint: " 
     << this->WorldPoint[0] << ", " << this->WorldPoint[1] << ", " 
     << this->WorldPoint[2] << endl;
  
  os << indent << "Pick: "
     << (this->PickCell ? "Cell" : "Point")
     << endl;
}



