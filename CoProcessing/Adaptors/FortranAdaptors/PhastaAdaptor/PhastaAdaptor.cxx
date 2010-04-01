/*=========================================================================

  Program:   ParaView
  Module:    PhastaAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "FortranAdaptorAPI.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

extern "C" void createpointsandallocatecells_(
  int* numPoints, double* coordsArray, int* numCells)
{
  if(!GetCoProcessorData())
    {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
    }

  vtkUnstructuredGrid* Grid = vtkUnstructuredGrid::New();
  vtkPoints* nodePoints = vtkPoints::New();
  vtkDoubleArray* coords = vtkDoubleArray::New();
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(*numPoints);
  for(int i=0;i<*numPoints;i++) 
    {
    double tuple[3] = {coordsArray[i], coordsArray[i+*numPoints], 
                       coordsArray[i+*numPoints*2]};
    coords->SetTupleValue(i, tuple);
    }
  nodePoints->SetData(coords);
  coords->Delete();
  Grid->SetPoints(nodePoints);
  nodePoints->Delete();
  Grid->Allocate(*numCells);
  GetCoProcessorData()->GetInputDescriptionByName("input")->SetGrid(Grid);
  Grid->Delete();
}

extern "C" void insertblockofcells_(
  int* numCellsInBlock, int* numPointsPerCell, int* cellConnectivity)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(
    GetCoProcessorData()->GetInputDescriptionByName("input")->GetGrid());
  if(!grid)
    {
    cout << "CoProcessing: Could not access grid for cell insertion.\n";
    }
  int type = -1;
  switch(*numPointsPerCell)
    {
    case 4:
    {
    type = VTK_TETRA;
    break;
    }
    case 5: 
    {
    type = VTK_PYRAMID;
    break;
    }
    case 6:
    {
    type = VTK_WEDGE;
    break;
    }
    case 8:
    {
    type = VTK_HEXAHEDRON;
    break;
    }
    default:
    {
    cout << "CoProcessing: Incorrect amount of vertices per element: " 
         << *numPointsPerCell << endl;
    return;
    }
    }
  vtkIdType pts[8]; //assume for now we only have linear elements
  vtkIdType numPoints = grid->GetNumberOfPoints();
  for(int iCell=0;iCell<*numCellsInBlock;iCell++)
    {
    for(int i=0;i<*numPointsPerCell;i++)
      {
      pts[i] = cellConnectivity[iCell+i*(*numCellsInBlock)]-1;//-1 to get from f to c++
      
      if(pts[i] < 0 || pts[i] >= numPoints)
        {
        cout << pts[i] << " is not a valid node id\n";
        }
      }
    if(type == VTK_TETRA)
      { // change the canonical ordering of the tet to match VTK style
      vtkIdType temp = pts[0];
      pts[0] = pts[1];
      pts[1] = temp;
      }
    grid->InsertNextCell(type, *numPointsPerCell, pts);
    }
}

extern "C" void addfields_(
  int* nshg, int* vtkNotUsed(ndof), double* dofArray, int *compressibleFlow)
{
  vtkCPInputDataDescription* idd =
    GetCoProcessorData()->GetInputDescriptionByName("input");
  vtkUnstructuredGrid* UnstructuredGrid = 
    vtkUnstructuredGrid::SafeDownCast(idd->GetGrid());
  if(!UnstructuredGrid)
    {
    cout << "PhastaAdaptor.cxx: No unstructured grid to attach field data to.\n";
    return;
    }
  vtkIdType NumberOfNodes = UnstructuredGrid->GetNumberOfPoints();
  // now add numerical field data
  //velocity
  if(idd->IsFieldNeeded("velocity"))
    {
    vtkDoubleArray* velocity = vtkDoubleArray::New();
    velocity->SetName("velocity");
    velocity->SetNumberOfComponents(3);
    velocity->SetNumberOfTuples(NumberOfNodes);
    for (vtkIdType idx=0; idx<NumberOfNodes; idx++)
      {
      velocity->SetTuple3(idx, dofArray[idx],
                          dofArray[idx+ *nshg],
                          dofArray[idx+ *nshg*2]);
      }
    UnstructuredGrid->GetPointData()->AddArray(velocity);
    velocity->Delete();
    }

  //pressure
  if(idd->IsFieldNeeded("pressure"))
    {
    vtkDoubleArray* pressure = vtkDoubleArray::New();
    pressure->SetName("pressure");
    pressure->SetArray(dofArray+*nshg*3, NumberOfNodes, 1);
    UnstructuredGrid->GetPointData()->AddArray(pressure);
    pressure->Delete();
    }
  
  //Temperature
  // temperature only varies from compressible flow
  if(idd->IsFieldNeeded("temperature") && *compressibleFlow == 1)
    {
    vtkDoubleArray* temperature = vtkDoubleArray::New();
    temperature->SetName("temperature");
    temperature->SetArray(dofArray+*nshg*4, NumberOfNodes, 1);
    UnstructuredGrid->GetPointData()->AddArray(temperature);
    temperature->Delete();
    }
}
