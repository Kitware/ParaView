/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMazeSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMazeSource.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkMath.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPointData.h"

vtkCxxRevisionMacro(vtkMazeSource, "1.1");
vtkStandardNewMacro(vtkMazeSource);

//----------------------------------------------------------------------------
vtkMazeSource::vtkMazeSource()
{
  this->XSize = 20;
  this->YSize = 20;
  this->RunFactor = 20;
  this->MagnetFactor = 4;
  this->RandomSeed = 0;
  this->ShowSolution = 1;

  this->Visited = NULL;
  this->NumberOfVisited;
  this->NeighborCount = NULL;
  this->NumberOfBranchCandidates = 0;
  this->RightEdge = NULL;
  this->UpEdge = NULL;
}

//----------------------------------------------------------------------------
void vtkMazeSource::Execute()
{
  int num, idx;  
  int x, y;
  double pt[3];
  vtkIdType edge[2];
  float r;

  // Avoid bad parameters.
  if (this->RandomSeed <= 0)
    {
    this->RandomSeed = 1;
    }
  if (this->RunFactor <= 1)
    {
    this->RunFactor = 2;
    }
  if (this->MagnetFactor < 1)
    {
    this->MagnetFactor = 1;
    }
  if (this->XSize < 2)
    {
    this->XSize = 2;
    }
  if (this->YSize < 2)
    {
    this->YSize = 2;
    }

  vtkMath::RandomSeed(this->RandomSeed); 

  num = this->XSize * this->YSize;

  this->Visited = new unsigned char[num];
  this->RightEdge = new unsigned char[num];
  this->UpEdge = new unsigned char[num];
  this->NeighborCount = new unsigned char[num];
  
  // Initialize
  this->NumberOfVisited = 0;
  this->NumberOfBranchCandidates = 0;
  idx = 0;
  for (y = 0; y < this->YSize; ++y)
    {
    for (x = 0; x < this->XSize; ++x)
      {
      this->Visited[idx] = 0;
      this->RightEdge[idx] = 1;
      this->UpEdge[idx] = 1;
      this->NeighborCount[idx] = 4;
      if (x == 0 || x == this->XSize-1)
        {
        --this->NeighborCount[idx];
        }
      if (y == 0 || y == this->YSize-1)
        {
        --this->NeighborCount[idx];
        }
      ++idx;
      }
    }

  while (this->NumberOfVisited < num)
    {
    // Start a new run/branch. Cell must be on the tree (except for first pick).
    idx = this->PickBranchCell();
    if ( idx < 0 )
      { // Sanity check.  
      // This should never happen, but avoid an infinite loop if it does.
      return;
      }

    // Create a run
    // Keep moving but do not move to a square already visited.
    r = vtkMath::Random();
    // The idx < 0 should never happen because of the open neighbor check.
    while (r < 1.0 - (1.0/this->RunFactor) && idx >= 0 && this->GetNumberOfOpenNeighbors(idx))
      { 
      idx = this->PickNeighborCell(idx);
      // Init r for the next while (run length) test.
      r = vtkMath::Random();
      }
    }

  // Now create the polydata.
  vtkPolyData* output = this->GetOutput();
  vtkPoints* newPts = vtkPoints::New();
  newPts->Allocate(num * 4);
  vtkCellArray* newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(num,2));
  pt[2] = 0.0;
  pt[0] = 0.0;
  pt[1] = 0.0;
  edge[0] = newPts->InsertNextPoint(pt);
  pt[1] = this->YSize-1;
  edge[1] = newPts->InsertNextPoint(pt);
  newLines->InsertNextCell(2,edge);

  pt[1] = 0.0;
  edge[0] = newPts->InsertNextPoint(pt);
  pt[0] = this->XSize-1;
  edge[1] = newPts->InsertNextPoint(pt);
  newLines->InsertNextCell(2,edge);
  
  for (x = 0; x < this->XSize; ++x)
    {
    for (y = 0; y < this->YSize; ++y)
      {
      idx = x + (y*this->XSize);
      if (idx < 0 || idx >= num) {vtkErrorMacro("Bad index");}
      if (this->RightEdge[idx])
        {
        pt[0] = x+1;
        pt[1] = y;
        edge[0] = newPts->InsertNextPoint(pt);
        pt[1] = y+1;
        edge[1] = newPts->InsertNextPoint(pt);
        newLines->InsertNextCell(2,edge);
        }
      if (this->UpEdge[idx])
        {
        pt[0] = x;
        pt[1] = y+1;
        edge[0] = newPts->InsertNextPoint(pt);
        pt[0] = x+1;
        edge[1] = newPts->InsertNextPoint(pt);
        newLines->InsertNextCell(2,edge);
        }
      }
    }

  double rgb[3];
  rgb[0] = rgb[1] = rgb[2] = 0;
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  num = newPts->GetNumberOfPoints();
  for (idx = 0; idx < num; ++idx)
    {
    colors->InsertTuple(idx, rgb);
    }
  colors->SetName("Color");
  output->GetPointData()->SetScalars(colors);
  colors->Delete();
  colors = NULL;

  output->SetPoints(newPts);
  newPts->Delete();
  newPts = NULL;

  output->SetLines(newLines);
  newLines->Delete();
  newLines = NULL;

  if (this->ShowSolution)
    {
    this->ExecuteSolution();
    }

  delete [] this->Visited;
  this->Visited = NULL;
  delete [] this->RightEdge;
  this->RightEdge = NULL;
  delete [] this->UpEdge;
  this->UpEdge = NULL;
  delete [] this->NeighborCount;
  this->NeighborCount = NULL;
}

//----------------------------------------------------------------------------
unsigned char vtkMazeSource::GetNumberOfOpenNeighbors(int idx)
{
  return this->NeighborCount[idx];
}


//----------------------------------------------------------------------------
void vtkMazeSource::MarkCellAsVisited(int idx)
{
  int x, y;
  int neighborIdx;

  if (this->Visited[idx])
    { // Already marked.  Do nothing.
    return;
    }

  this->Visited[idx] = 1;
  ++this->NumberOfVisited;

  y = idx / this->XSize;
  x = idx - (y * this->XSize);

  // Remove as neighbor.
  if (x > 0)
    {
    neighborIdx = idx-1;
    --this->NeighborCount[neighborIdx];
    if (this->Visited[neighborIdx] && this->NeighborCount[neighborIdx] == 0)
      { // We lost a candidate.
      --this->NumberOfBranchCandidates;
      }
    }
  if (x < this->XSize-1)
    {
    neighborIdx = idx+1;
    --this->NeighborCount[idx+1];
    if (this->Visited[neighborIdx] && this->NeighborCount[neighborIdx] == 0)
      { // We lost a candidate.
      --this->NumberOfBranchCandidates;
      }
    }
  if (y > 0)
    {
    neighborIdx = idx-this->XSize;
    --this->NeighborCount[neighborIdx];
    if (this->Visited[neighborIdx] && this->NeighborCount[neighborIdx] == 0)
      { // We lost a candidate.
      --this->NumberOfBranchCandidates;
      }
    }
  if (y < this->YSize-1)
    {
    neighborIdx = idx+this->XSize;
    --this->NeighborCount[neighborIdx];
    if (this->Visited[neighborIdx] && this->NeighborCount[neighborIdx] == 0)
      { // We lost a candidate.
      --this->NumberOfBranchCandidates;
      }
    }

  // Is this cell a candidate now?
  // Canditates are Visited, but have neighbors which are not visited.
  if (this->NeighborCount[idx] > 0)
    {
    ++this->NumberOfBranchCandidates;
    }
}


//----------------------------------------------------------------------------
int vtkMazeSource::PickBranchCell()
{
  float r;
  int idx, num;
  int choice;

  num = this->XSize * this->YSize;

  // First pick will not be visited.
  if (this->NumberOfVisited <= 0)
    {
    r = vtkMath::Random();
    idx = (int)(r * (float)(num));
    // Only needed for the first choice.
    this->MarkCellAsVisited(idx);
    return idx;
    }

  // Pick a starting spot randomly out of the available visited cells.
  r = vtkMath::Random();
  choice = (int)(r * (float)this->NumberOfBranchCandidates);
  // Just in case.
  if (choice < 0)
    {
    choice = 0;
    }
  if (choice >= this->NumberOfBranchCandidates)
    {
    choice = this->NumberOfBranchCandidates-1;
    }
    
  for (idx = 0; idx < num; ++idx)
    {
    if (this->Visited[idx] && this->NeighborCount[idx] > 0)
      {
      if (choice == 0)
        {
        return idx;
        }
      --choice;
      }
    }
  
  vtkErrorMacro("Could not find the choosen cell.");
  return -1;
}


//----------------------------------------------------------------------------
int vtkMazeSource::PickNeighborCell(int cellId)
{
  int direction;
  int x, y, nx, ny;
  int neighborCellId;
  float r;
  float weights[4];
  float weightTable[5];

  weightTable[4] = 1.0;
  weightTable[3] = 1.0;
  weightTable[2] = this->MagnetFactor;
  weightTable[1] = weightTable[2] * this->MagnetFactor;
  weightTable[0] = weightTable[1] * this->MagnetFactor;

  // Initialize for boundaries.
  weights[0] = weights[1] = weights[2] = weights[3] = 0.0;

  // Pick  a direction (0:left, 1 right, 2 down, 3 up).

  // Assign weights to all of the directions.
  // Larger weights for cells with fewer neighbors will cause the path
  // to hug boundaries.
  y = cellId / this->XSize;
  x = cellId - (y* this->XSize);
  if (x > 0 && ! this->Visited[cellId-1])
    {
    weights[0] = weightTable[this->NeighborCount[cellId-1]];
    }
  if (x < this->XSize-1 && ! this->Visited[cellId+1])
    {
    weights[1] = weightTable[this->NeighborCount[cellId+1]];
    }
  if (y > 0 && ! this->Visited[cellId-this->XSize])
    {
    weights[2] = weightTable[this->NeighborCount[cellId-this->XSize]];
    }
  if (y < this->YSize-1 && ! this->Visited[cellId+this->XSize])
    {
    weights[3] = weightTable[this->NeighborCount[cellId+this->XSize]];
    }

  // Choose a direction.
  r = vtkMath::Random() * (weights[0]+weights[1]+weights[2]+weights[3]);
  direction = 0;
  while (direction < 4 && r > weights[direction])
    {
    r -= weights[direction];
    ++direction;
    }
  if (direction == 4)
    { // This should not happen, but with floating point ...
    vtkErrorMacro("Bad direction.");
    direction = 3;
    }

  // Compute the proposed new position.
  nx = x;
  ny = y;
  if (direction == 0)
    {
    nx = x-1;
    }        
  if (direction == 1)
    {
    nx = x+1;
    }        
  if (direction == 2)
    {
    ny = y-1;
    }        
  if (direction == 3)
    {
    ny = y+1;
    }
  // If a valid move, then move.  With the above check, this condition
  // should always be true.
  if (nx >= 0 && nx < this->XSize && ny >= 0 && ny < this->YSize)
    {
    neighborCellId = nx+ (ny*this->XSize);
    if (this->Visited[neighborCellId] == 0)
      { // Good move.
      x = nx;
      y = ny;
      this->MarkCellAsVisited(neighborCellId);
      // Get rid of wall (idx is set to the new block).
      if (direction == 0)
        {
        this->RightEdge[neighborCellId] = 0;
        }
      if (direction == 1)
        {
        this->RightEdge[neighborCellId-1] = 0;
        }
      if (direction == 2)
        {
        this->UpEdge[neighborCellId] = 0;
        }
      if (direction == 3)
        {
        this->UpEdge[neighborCellId-this->XSize] = 0;
        }
      return neighborCellId;
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
// Follow the left wall.
void vtkMazeSource::RecordMove(int idx, int newIdx, vtkIntArray* path)
{
  int length = path->GetNumberOfTuples();

  if (length > 0 && path->GetValue(length-1) == newIdx)
    { // Backtracking.  Erase path.
    path->SetNumberOfValues(length-1);
    }
  else
    { // Record new path.
    path->InsertNextValue(idx);
    }    
}

//----------------------------------------------------------------------------
// Follow the left wall.
//
// !!! We really can do without the visited array.
void vtkMazeSource::ExecuteSolution()
{
  int idx, newIdx, num, x, y, direction;
  vtkIntArray* path = vtkIntArray::New();
  vtkUnsignedCharArray* colors;

  path->Allocate(2*(this->XSize+this->YSize));

  x = 0;
  y = this->YSize-1;
  idx = x+ (y*this->XSize);
  // Direction here is different (0: right, 1: up, 2: left, 3: down)
  direction = 0;

  while (x != this->XSize-1 || y != 0)
    {
    if (direction == 0)
      { // Looking right
      if (y < this->YSize-1 && ! this->UpEdge[idx])
        { // If there is nothing to our left, then turn and move forward.
        direction = 1;
        ++y;
        newIdx = idx + this->XSize;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else if (x < this->XSize-1 && ! this->RightEdge[idx])
        { // If there is nothing in front of us, move forward.
        ++x;
        newIdx = idx + 1;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else
        { // Rotate right
        direction = 3;
        }
      }
    else if (direction == 1)
      { // Looking up
      if (x > 0 && ! this->RightEdge[idx-1])
        { // If there is nothing to our left, then turn and move forward.
        direction = 2;
        --x;
        newIdx = idx-1;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else if (y < this->YSize-1 && ! this->UpEdge[idx])
        { // If there is nothing in front of us, move forward.
        ++y;
        newIdx = idx + this->XSize;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else
        { // Rotate right
        direction = 0;
        }
      }
    else if (direction == 2)
      { // Looking left
      if (y > 0 && ! this->UpEdge[idx-this->XSize])
        { // If there is nothing to our left, then turn and move forward.
        direction = 3;
        --y;
        newIdx = idx-this->XSize;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else if (x > 0 && ! this->RightEdge[idx-1])
        { // If there is nothing in front of us, move forward.
        --x;
        newIdx = idx - 1;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else
        { // Rotate right
        direction = 1;
        }
      }
    else if (direction == 3)
      { // Looking down
      if (x < this->XSize-1 && ! this->RightEdge[idx])
        { // If there is nothing to our left, then turn and move forward.
        direction = 0;
        ++x;
        newIdx = idx+1;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else if (y > 0 && ! this->UpEdge[idx-this->XSize])
        { // If there is nothing in front of us, move forward.
        --y;
        newIdx = idx - this->XSize;
        this->RecordMove(idx, newIdx, path); 
        idx = newIdx;
        }
      else
        { // Rotate right
        direction = 2;
        }
      }
    }
  // Mark the last element.
  path->InsertNextValue(idx);

  // Now draw the path.
  colors = vtkUnsignedCharArray::SafeDownCast(
                         this->GetOutput()->GetPointData()->GetScalars());
  double rgb[3];
  double pt[3];
  vtkIdType edge[2];
  vtkPoints* pts = this->GetOutput()->GetPoints();
  vtkCellArray* edges = this->GetOutput()->GetLines();
  num = path->GetNumberOfTuples();
  newIdx = path->GetValue(0);
  rgb[0] = 255;  rgb[1] = 0; rgb[2] = 0;
  pt[2] = 0.0;
  pt[1] = (double)((int)(newIdx / this->XSize));
  pt[0] = newIdx - (int)(pt[1] * this->XSize);
  pt[0] += 0.5;
  pt[1] += 0.5;
  edge[1] = pts->InsertNextPoint(pt);
  colors->InsertNextTuple(rgb);
  for (idx = 1; idx < num; ++idx)
    {
    newIdx = path->GetValue(idx);  
    pt[1] = (double)((int)(newIdx / this->XSize));
    pt[0] = newIdx - (int)(pt[1] * this->XSize);
    pt[0] += 0.5;
    pt[1] += 0.5;
    edge[0] = edge[1];
    edge[1] = pts->InsertNextPoint(pt);
    colors->InsertNextTuple(rgb);
    edges->InsertNextCell(2, edge);
    }

  path->Delete();
  path = NULL;
}


//----------------------------------------------------------------------------
void vtkMazeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << "XSize: " << this->XSize << endl;
  os << "YSize: " << this->YSize << endl;
  os << "RunFactor: " << this->RunFactor << endl;
  os << "MagnetFactor: " << this->MagnetFactor << endl;
  os << "RandomSeed: " << this->RandomSeed << endl;
  os << "ShowSolution: " << this->ShowSolution << endl;
}
