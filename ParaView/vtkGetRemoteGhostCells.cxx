/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGetRemoteGhostCells.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkGetRemoteGhostCells.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkGetRemoteGhostCells* vtkGetRemoteGhostCells::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGetRemoteGhostCells");
  if(ret)
    {
    return (vtkGetRemoteGhostCells*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGetRemoteGhostCells;
}

vtkGetRemoteGhostCells::vtkGetRemoteGhostCells()
{
  this->Controller = NULL;
  this->Locator = vtkPointLocator::New();
}

vtkGetRemoteGhostCells::~vtkGetRemoteGhostCells()
{
  this->SetController(NULL);
  this->Locator->Delete();
  this->Locator = NULL;
}

void vtkGetRemoteGhostCells::Execute()
{
  int validFlag, myId, numProcs, numPoints, pointId, *pointIds;
  float point[3];
  int numCells, numCellPoints;
//  int morePoints = 1;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int i = 0, j, k, l, id;
  vtkPoints *points = vtkPoints::New();
  vtkPoints *cellPoints;
  vtkCellArray *polys;
  vtkIdList *cellIds = vtkIdList::New();
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkGhostLevels *ghostLevels = vtkGhostLevels::New();
  float *myPoints, *remotePoints;
  int *numRemotePoints;
  vtkIdList **insertedCells;
  int cellId, remoteCellId;
  
  if (!this->Controller)
    {
    vtkErrorMacro("need controller to get remote ghost cells");
    return;
    }
  
  this->Locator->InitPointInsertion(vtkPoints::New(), input->GetBounds());
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();
  numPoints = input->GetNumberOfPoints();
  polys = input->GetPolys();
  
  myPoints = new float[numPoints*3];
  numRemotePoints = new int[numProcs];
  insertedCells = (vtkIdList**)malloc(numProcs * sizeof(vtkIdList));
  
  for (j = 0; j < numProcs; j++)
    {
    insertedCells[j] = vtkIdList::New();
    }
  
  for (j = 0; j < numPoints; j++)
    {
    input->GetPoint(j, point);
    this->Locator->InsertNextPoint(point);
    points->InsertNextPoint(point);
    for (k = 0; k < 3; k++)
      {
      myPoints[j*3+k] = point[k];
      }
    }
  
  for (j = 0; j < polys->GetNumberOfCells(); j++)
    {
    ghostLevels->InsertNextGhostLevel(0);
    }
  
  output->SetPoints(points);
  output->SetPolys(polys);
  
//  while (morePoints)
//    {
//    morePoints = 0;
//    for (id = 0; id < numProcs; id++)
//      {
//      if (id != myId)
//	{
//	if (i < numPoints)
//	  {
//	  morePoints = 1;
//	  validFlag = 1;
//	  input->GetPoint(i, point);
//	  this->Controller->Send((int*)(&validFlag), 1, id,
//				 VTK_VALID_POINT_TAG);
//	  this->Controller->Send(point, 3, id, VTK_POINT_COORDS_TAG);
//	  }
//	else
//	  {
//	  validFlag = 0;
//	  this->Controller->Send((int*)(&validFlag), 1, id,
//				 VTK_VALID_POINT_TAG);
//	  }
//	} // if not my process
//    } // for all processes (send next point)

  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
      this->Controller->Send((int*)(&numPoints), 1, id, VTK_NUM_POINTS_TAG);
      this->Controller->Send(myPoints, numPoints*3, id, VTK_POINT_COORDS_TAG);
      }
    }
  
  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
//      this->Controller->Receive((int*)(&validFlag), 1, id,
//				VTK_VALID_POINT_TAG);
//      if (validFlag)
//	{
//	morePoints = 1;
//	this->Controller->Receive(point, 3, id, VTK_POINT_COORDS_TAG);
      this->Controller->Receive((int*)(&numRemotePoints[id]), 1, id,
				VTK_NUM_POINTS_TAG);
      remotePoints = new float[numRemotePoints[id]*3];
      this->Controller->Receive(remotePoints, numRemotePoints[id]*3, id,
				VTK_POINT_COORDS_TAG);
      for (i = 0; i < numRemotePoints[id]; i++)
	{
	point[0] = remotePoints[i*3];
	point[1] = remotePoints[i*3+1];
	point[2] = remotePoints[i*3+2];
	if ((pointId = this->Locator->IsInsertedPoint(point)) >= 0)
	  {
	  output->GetPointCells(pointId, cellIds);
	  numCells = cellIds->GetNumberOfIds();
	  
	  this->Controller->Send((int*)(&numCells), 1, id,
				 VTK_NUM_CELLS_TAG);
	  for (j = 0; j < numCells; j++)
	    {
	    cellId = cellIds->GetId(j);
	    output->GetCell(cellId, cell);
	    numCellPoints = cell->GetNumberOfPoints();
	    this->Controller->Send((int*)(&cellId), 1, id, VTK_CELL_ID_TAG);
	    this->Controller->Send((int*)(&numCellPoints), 1, id,
				   VTK_NUM_POINTS_TAG);
	    cellPoints = cell->GetPoints();
	    for (k = 0; k < numCellPoints; k++)
	      {
	      cellPoints->GetPoint(k, point);
	      this->Controller->Send(point, 3, id, VTK_POINT_COORDS_TAG);
	      } // for all points in this cell (send coords)
	    } // for all point cells (send points)
	  } // if point in my data
	else
	  {
	  numCells = 0;
	  this->Controller->Send((int*)(&numCells), 1, id,
				 VTK_NUM_CELLS_TAG);
	  } // point not in my data (no cells to send)
	} // for all points received from this process
//	} // if valid point
//      else
//	{
//	numCells = 0;
//	this->Controller->Send((int*)(&numCells), 1, id,
//			       VTK_NUM_CELLS_TAG);
//	} // not valid point (need to send numCells = 0 anyway)
      delete [] remotePoints;
      } // if not my process
    } // for all processes (find point cells)
  
  for (id = 0; id < numProcs; id++)
    {
    if (id != myId)
      {
      for (i = 0; i < numRemotePoints[id]; i++)
	{
	this->Controller->Receive((int*)(&numCells), 1, id,
				  VTK_NUM_CELLS_TAG);
	for (j = 0; j < numCells; j++)
	  {
	  this->Controller->Receive((int*)(&remoteCellId), 1, id,
				    VTK_CELL_ID_TAG);
	  this->Controller->Receive((int*)(&numCellPoints), 1, id,
				    VTK_NUM_POINTS_TAG);
	  pointIds = new int[numCellPoints];
	  for (k = 0; k < numCellPoints; k++)
	    {
	    this->Controller->Receive(point, 3, id, VTK_POINT_COORDS_TAG);
	    if ((pointIds[k] = this->Locator->IsInsertedPoint(point)) == -1)
	      {
	      pointIds[k] = this->Locator->InsertNextPoint(point);
	      points->InsertPoint(pointIds[k], point);
	      } // point not in my data already
	    } // for all points in this cell
	  output->SetPoints(points);
	  if (insertedCells[id]->IsId(remoteCellId) == -1)
	    {
	    insertedCells[id]->InsertNextId(remoteCellId);
	    polys->InsertNextCell(numCellPoints, pointIds);
	    if (myId == 0) cerr << "cell: "<<pointIds[0]<<" "<<pointIds[1]<<" "<<pointIds[2]<<endl;
	    output->SetPolys(polys);
	    ghostLevels->InsertNextGhostLevel(1);
	    }
	  output->DeleteCells();
	  output->BuildLinks();
	  delete [] pointIds;
	  } // for all cells sent by this process
	} // for all points in this process
      } // if not my process
    } // for all processes
//    i++;
//    } // while more points
  
  output->GetCellData()->SetGhostLevels(ghostLevels);
  
  points->Delete();
  points = NULL;
  cellIds->Delete();
  cellIds = NULL;
  cell->Delete();
  cell = NULL;
  ghostLevels->Delete();
  ghostLevels = NULL;
  delete [] myPoints;
  free(insertedCells);
}

void vtkGetRemoteGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Controller (" << this->Controller << ")\n";
  os << indent << "Locator (" << this->Locator << ")/n";
}
