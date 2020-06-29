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

#include "PhastaAdaptor.h"

#include "FortranAdaptorAPI.h"
#include "PhastaAdaptorAPIMangling.h"

#include "vtkCPAdaptorAPI.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"

//------------------------------------------------------------------------------
void createpointsandallocatecells(int* numPoints, double* coordsArray, int* numCells)
{
  if (!vtkCPAdaptorAPI::GetCoProcessorData())
  {
    vtkGenericWarningMacro("Unable to access CoProcessorData.");
    return;
  }

  vtkUnstructuredGrid* Grid = vtkUnstructuredGrid::New();
  vtkPoints* nodePoints = vtkPoints::New();
  vtkDoubleArray* coords = vtkDoubleArray::New();
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(*numPoints);
  for (int i = 0; i < *numPoints; i++)
  {
    double tuple[3] = { coordsArray[i], coordsArray[i + *numPoints],
      coordsArray[i + *numPoints * 2] };
    coords->SetTypedTuple(i, tuple);
  }
  nodePoints->SetData(coords);
  coords->Delete();
  Grid->SetPoints(nodePoints);
  nodePoints->Delete();
  Grid->Allocate(*numCells);
  vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->SetGrid(Grid);
  Grid->Delete();
}

//------------------------------------------------------------------------------
void insertblockofcells(int* numCellsInBlock, int* numPointsPerCell, int* cellConnectivity)
{
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input")->GetGrid());
  if (!grid)
  {
    vtkGenericWarningMacro("CoProcessing: Could not access grid for cell insertion.");
  }
  int type = -1;
  switch (*numPointsPerCell)
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
      vtkGenericWarningMacro(
        "CoProcessing: Incorrect amount of vertices per element: " << *numPointsPerCell);
      return;
    }
  }
  vtkIdType pts[8]; // assume for now we only have linear elements
  vtkIdType numPoints = grid->GetNumberOfPoints();
  for (int iCell = 0; iCell < *numCellsInBlock; iCell++)
  {
    for (int i = 0; i < *numPointsPerCell; i++)
    {
      pts[i] = cellConnectivity[iCell + i * (*numCellsInBlock)] - 1; //-1 to get from f to c++

      if (pts[i] < 0 || pts[i] >= numPoints)
      {
        vtkGenericWarningMacro(<< pts[i] << " is not a valid node id.");
      }
    }
    if (type == VTK_TETRA)
    { // change the canonical ordering of the tet to match VTK style
      vtkIdType temp = pts[0];
      pts[0] = pts[1];
      pts[1] = temp;
    }
    grid->InsertNextCell(type, *numPointsPerCell, pts);
  }
}

//------------------------------------------------------------------------------
void addfields(int* nshg, int* vtkNotUsed(ndof), double* dofArray, int* compressibleFlow)
{
  vtkCPInputDataDescription* idd =
    vtkCPAdaptorAPI::GetCoProcessorData()->GetInputDescriptionByName("input");
  vtkUnstructuredGrid* UnstructuredGrid = vtkUnstructuredGrid::SafeDownCast(idd->GetGrid());
  if (!UnstructuredGrid)
  {
    vtkGenericWarningMacro("No unstructured grid to attach field data to.");
    return;
  }
  vtkIdType NumberOfNodes = UnstructuredGrid->GetNumberOfPoints();
  // now add numerical field data
  // velocity
  if (idd->IsFieldNeeded("velocity", VTK_DOUBLE))
  {
    vtkDoubleArray* velocity = vtkDoubleArray::New();
    velocity->SetName("velocity");
    velocity->SetNumberOfComponents(3);
    velocity->SetNumberOfTuples(NumberOfNodes);
    for (vtkIdType idx = 0; idx < NumberOfNodes; idx++)
    {
      velocity->SetTuple3(idx, dofArray[idx], dofArray[idx + *nshg], dofArray[idx + *nshg * 2]);
    }
    UnstructuredGrid->GetPointData()->AddArray(velocity);
    velocity->Delete();
  }

  // pressure
  if (idd->IsFieldNeeded("pressure", VTK_DOUBLE))
  {
    vtkDoubleArray* pressure = vtkDoubleArray::New();
    pressure->SetName("pressure");
    pressure->SetArray(dofArray + *nshg * 3, NumberOfNodes, 1);
    UnstructuredGrid->GetPointData()->AddArray(pressure);
    pressure->Delete();
  }

  // Temperature
  // temperature only varies from compressible flow
  if (idd->IsFieldNeeded("temperature", VTK_DOUBLE) && *compressibleFlow == 1)
  {
    vtkDoubleArray* temperature = vtkDoubleArray::New();
    temperature->SetName("temperature");
    temperature->SetArray(dofArray + *nshg * 4, NumberOfNodes, 1);
    UnstructuredGrid->GetPointData()->AddArray(temperature);
    temperature->Delete();
  }
}
