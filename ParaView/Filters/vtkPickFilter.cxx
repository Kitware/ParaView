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
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"


vtkCxxRevisionMacro(vtkPickFilter, "1.2");
vtkStandardNewMacro(vtkPickFilter);
vtkCxxSetObjectMacro(vtkPickFilter,Controller,vtkMultiProcessController);



//-----------------------------------------------------------------------------
vtkPickFilter::vtkPickFilter ()
{
  this->PickCell = 0;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->WorldPoint[0] = this->WorldPoint[1] = this->WorldPoint[2] = 0.0;
  this->NumberOfLayers = 0;

  this->PointMap = 0;
  this->RegionPointIds = 0;
  this->RegionPointLevels = 0;
  this->GenerateLayerAttribute = 1;
}

//-----------------------------------------------------------------------------
vtkPickFilter::~vtkPickFilter ()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void vtkPickFilter::Execute()
{
  this->InitializePointMap(this->GetInput()->GetNumberOfPoints());

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
  vtkDataSet* input = this->GetInput();
  vtkIdType numPts, ptId;

  // Find the nearest point in the input.
  bestDistance2 = VTK_LARGE_FLOAT;
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
      bestDistance2 = distance2;
      bestPt[0] = pt[0];
      bestPt[1] = pt[1];
      bestPt[2] = pt[2];
      }
    }

  // Keep only the best seed among the processes.
  vtkIdList* regionCellIds = vtkIdList::New();
  vtkIntArray* regionCellLevels = vtkIntArray::New();
  if ( ! this->CompareProcesses(bestDistance2) && numPts > 0)
    {
    this->InsertIdInPointMap(bestId, 0);
    }

  for (int idx = 0; idx < this->NumberOfLayers; ++idx)
    {
    this->Grow(idx+1,regionCellIds, regionCellLevels);
    }

  this->CreateOutput(regionCellIds, regionCellLevels);
  regionCellIds->Delete();
  regionCellLevels->Delete();
}

//-----------------------------------------------------------------------------
void vtkPickFilter::CellExecute()
{
  // Loop over all of the cells.
  vtkDataSet* input = this->GetInput();
  vtkIdType cellId;
  vtkCell* cell;
  int inside;
  double closestPoint[3];
  int  subId;
  double pcoords[3];
  double dist2;
  double bestDist2 = VTK_LARGE_FLOAT;
  double weights[48]; // maximum numer of points.
  vtkIdType numCells;
  vtkIdType bestId = -1;

  numCells = input->GetNumberOfCells();
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);
    inside = cell->EvaluatePosition(this->WorldPoint, closestPoint, 
                                    subId, pcoords, dist2, weights);
    if (inside)
      {
      dist2= 0.0;
      }
    if (dist2 < bestDist2)
      {
      bestId = cellId;
      bestDist2 = dist2;
      }
    }

  // Keep only the best seed cell among the processes.
  vtkIdList* regionCellIds = vtkIdList::New();
  vtkIntArray* regionCellLevels = vtkIntArray::New();
  if ( ! this->CompareProcesses(dist2) && bestId >= 0)
    {
    regionCellIds->InsertNextId(bestId);
    regionCellLevels->InsertNextValue(0);
    // Insert the cell points.
    vtkIdList* cellPtIds = vtkIdList::New();
    this->GetInput()->GetCellPoints(bestId, cellPtIds);
    vtkIdType i;
    for (i = 0; i < cellPtIds->GetNumberOfIds(); ++i)
      {
      this->InsertIdInPointMap(cellPtIds->GetId(i), 0);
      }
    cellPtIds->Delete();
    }

  // Grow the layers
  for (int idx = 0; idx < this->NumberOfLayers; ++idx)
    {
    this->Grow(idx+1, regionCellIds, regionCellLevels);
    }

  this->CreateOutput(regionCellIds, regionCellLevels);
  regionCellIds->Delete();
  regionCellLevels->Delete();
}

//-----------------------------------------------------------------------------
void vtkPickFilter::Grow(int level, vtkIdList* regionCellIds, 
                         vtkIntArray* regionCellLevels)
{
  vtkDataSet* input = this->GetInput();
  vtkPoints* pts = vtkPoints::New();
  vtkIdList* ptCellIds = vtkIdList::New();
  vtkIdList* cellPtIds = vtkIdList::New();
  double pt[3];
  vtkIdType i, j, k;

  // This is similar to growing ghost cells, but we know
  // the starting seed/cell is small.

  // The first step is to get point ids from the last level.
  // Region boundary points have not been propagated across processes yet.
  // We need to gather the xyz locations and use 
  // a locator to get the local ids.

  // Get the XYZ points for the last level (local points).
  this->GetInputLayerPoints(pts, level-1, input);
  // Gather region  boundary points (last level) from all processes.
  this->GatherPoints(pts);

  // Use a locator to find local ids and add them to the region.
  // (discard non local pts).
  vtkIdType numPts, cellId, ptId, pt2Id;
  int ptLevel;
  numPts = pts->GetNumberOfPoints();
  for (i = 0; i < numPts; ++i)
    {
    pts->GetPoint(i, pt);
    ptId = this->FindPointId(pt, input);
    if (ptId >= 0)
      {
      this->InsertIdInPointMap(ptId, level-1);
      }
    }

  // Find the next level of cells for the region.
  numPts = this->RegionPointIds->GetNumberOfIds();
  for (i = 0; i < numPts; ++i)
    {
    ptId = this->RegionPointIds->GetId(i);
    ptLevel = this->RegionPointLevels->GetValue(i);
    if (ptLevel == level-1)
      { // Boundary Point: Find all connected cells.
      input->GetPointCells(ptId, ptCellIds);
      for (j = 0; j < ptCellIds->GetNumberOfIds(); j++)
        {
        cellId = ptCellIds->GetId(j);
        if ( ! this->ListContainsId(regionCellIds, cellId))
          { // Insert the new cell into the region.
          regionCellIds->InsertNextId(cellId);
          regionCellLevels->InsertNextValue(level);
          // Add this cells points to the region also.
          input->GetCellPoints(cellId, cellPtIds);
          for (k = 0; k < cellPtIds->GetNumberOfIds(); k++)
            {
            pt2Id = cellPtIds->GetId(k);
            // Insert the new point into the region.
            this->InsertIdInPointMap(pt2Id, level);
            }
          }
        }
      }
    }

  pts->Delete();
  ptCellIds->Delete();
  cellPtIds->Delete();
}

//-----------------------------------------------------------------------------
vtkIdType vtkPickFilter::InsertIdInPointMap(vtkIdType inId, int level)
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
  this->RegionPointLevels->InsertNextValue(level);
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
  this->RegionPointLevels = vtkIntArray::New();

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
  if (this->RegionPointLevels)
    {
    this->RegionPointLevels->Delete();
    this->RegionPointLevels = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkPickFilter::GetInputLayerPoints(vtkPoints* pts, int level, 
                                        vtkDataSet* input)
{
  double pt[3];
  vtkIdType i, numPts;

  pts->Initialize();
  numPts = this->RegionPointIds->GetNumberOfIds();
  for (i = 0; i < numPts; ++i)
    {
    if (this->RegionPointLevels->GetValue(i) == level)
      {
      input->GetPoint(this->RegionPointIds->GetId(i), pt);
      pts->InsertNextPoint(pt);
      }
    }
}

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkPickFilter::GatherPoints(vtkPoints* pts)
#else
void vtkPickFilter::GatherPoints(vtkPoints*)
#endif
{
#ifdef VTK_USE_MPI
  vtkMPICommunicator* com = NULL;
  if (this->Controller == NULL)
    {
    return;
    }
  com = vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
  if (com == NULL)
    {
    return;
    }
  int idx, sum;
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myId = this->Controller->GetLocalProcessId();
  int size = pts->GetNumberOfPoints() * 3;
  int* recvLenghts = new int[numProcs * 2];
  int* recvOffsets = recvLengths+numProcs;
  com->AllGather(&size, recvLengths, 1);
  // Compute the displacements.
  sum = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    recvOffsets[idx] = sum;
    sum += recvLengths[idx];
    }

  // Gather the points from all processes.
  if (pts->GetDataType() == VTK_FLOAT)
    {
    float* ptr = static_cast<float>(pts->GetVoidPointer());
    float* allPts = new float[sum];
    com->AllGatherV(ptr, allPts, size, recvLengths, recvOffsets);
    pts->Initialize();
    pts->Allocate(sum/3);
    for (idx = 0; idx < sum; idx += 3)
      {
      pts->InsertPoint(idx, allPts+idx);
      }
    }
  else if (pts->GetDataType() == VTK_DOUBLE)
    {
    double* ptr = static_cast<double>(pts->GetVoidPointer());
    double* allPts = new float[sum];
    com->AllGatherV(ptr, allPts, size, recvLengths, recvOffsets);
    pts->Initialize();
    pts->Allocate(sum/3);
    for (idx = 0; idx < sum; idx += 3)
      {
      pts->InsertPoint(idx, allPts+idx);
      }
    }
  else
    {
    vtkErrorMacro("I only handle double and float points.");
    } 
  delete [] recvLengths;
#endif // VTK_USE_MPI
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
      this->Controller->Send(&bestProc, 1, idx, 2340100);
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
void vtkPickFilter::CreateOutput(vtkIdList* regionCellIds, 
                                 vtkIntArray* regionCellLevels)
{
  vtkDataSet* input = this->GetInput();
  vtkUnstructuredGrid* output = this->GetOutput();
  double pt[3];

  if (regionCellIds->GetNumberOfIds() == 0)
    {
    if (this->RegionPointIds->GetNumberOfIds() == 0)
      {
      return;
      }
    // This case will never have more than one point.
    vtkIdType ptId = this->RegionPointIds->GetId(0);
    vtkIdType ptIds[1];
    // Special case: one point. Create a vertex.
    input->GetPoint(ptId, pt);
    output->Allocate(1);
    vtkPoints* newPoints= vtkPoints::New();
    output->GetPointData()->CopyAllocate(input->GetPointData(), 1);
    newPoints->InsertNextPoint(pt[0], pt[1], pt[2]);
    output->GetPointData()->CopyData(input->GetPointData(), ptId, 0);
    ptIds[0] = 0;
    output->InsertNextCell(VTK_VERTEX, 1, ptIds);
    output->SetPoints(newPoints);
    newPoints->Delete();
    if (this->GenerateLayerAttribute)
      {
      vtkIntArray* levelArray = vtkIntArray::New();
      levelArray->DeepCopy(this->RegionPointLevels);
      levelArray->SetName("Pick Level");
      output->GetPointData()->AddArray(levelArray);
      levelArray->Delete();
      }
    return;
    }

  // First copy the points.
  vtkPoints* newPoints = vtkPoints::New();
  vtkIdType numPts, outId, inId;
  numPts = this->RegionPointIds->GetNumberOfIds();
  newPoints->Allocate(numPts);
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPts);
  for (outId = 0; outId < numPts; ++outId)
    {
    inId = this->RegionPointIds->GetId(outId);
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
  output->GetCellData()->CopyAllocate(input->GetCellData(), numCells);
  vtkIdType num, i;
  for (outId = 0; outId < numCells; ++outId)
    {
    inId = regionCellIds->GetId(outId);
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

  if (this->GenerateLayerAttribute)
    {
    vtkIntArray* levelArray;
    levelArray = vtkIntArray::New();
    levelArray->DeepCopy(this->RegionPointLevels);
    levelArray->SetName("Pick Level");
    output->GetPointData()->AddArray(levelArray);
    levelArray->Delete();

    levelArray = vtkIntArray::New();
    levelArray->DeepCopy(regionCellLevels);
    levelArray->SetName("Pick Level");
    output->GetCellData()->AddArray(levelArray);
    levelArray->Delete();
    output->GetCellData()->SetActiveScalars("Pick Level");
    }


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

  os << indent << "NumberOfLayers: " << this->NumberOfLayers << endl;
  os << indent << "GenerateLayerAttribute: " << this->GenerateLayerAttribute << endl;
}



