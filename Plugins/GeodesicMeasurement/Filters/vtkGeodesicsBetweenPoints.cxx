/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastMarchingGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeodesicsBetweenPoints.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCleanPolyData.h"
#include "vtkDoubleArray.h"
#include "vtkFastMarchingGeodesicPath.h"
#include "vtkFieldData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOctreePointLocator.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGeodesicsBetweenPoints);

//-----------------------------------------------------------------------------
vtkGeodesicsBetweenPoints::vtkGeodesicsBetweenPoints()
{
  this->SetNumberOfInputPorts(2);

  this->Loop = 0;
  this->LoopWithLine = 0;
}

//-----------------------------------------------------------------------------
vtkGeodesicsBetweenPoints::~vtkGeodesicsBetweenPoints() = default;

//-----------------------------------------------------------------------------
int vtkGeodesicsBetweenPoints::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  }
  else if (port == 1)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkGeodesicsBetweenPoints::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* endpointInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* endpoints =
    vtkPointSet::SafeDownCast(endpointInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input || !endpoints)
  {
    return 0;
  }

  // Build an octree point locator to find the nearest point on the input
  vtkSmartPointer<vtkOctreePointLocator> locator = vtkSmartPointer<vtkOctreePointLocator>::New();
  locator->SetDataSet(input);
  locator->BuildLocator();

  // Get the nearest point IDs on the polydata from the endpoints
  vtkSmartPointer<vtkIdList> nearestPtIds = vtkSmartPointer<vtkIdList>::New();
  for (vtkIdType ptId = 0; ptId < endpoints->GetNumberOfPoints(); ++ptId)
  {
    double pt[3];
    endpoints->GetPoint(ptId, pt);
    vtkIdType nearestPtId = locator->FindClosestPoint(pt);
    nearestPtIds->InsertNextId(nearestPtId);
  }

  // Instantiate the geodesic path filter
  vtkSmartPointer<vtkFastMarchingGeodesicPath> path =
    vtkSmartPointer<vtkFastMarchingGeodesicPath>::New();
  path->SetInputData(input);
  path->SetInterpolationOrder(1);

  // Appender to collect all the geodesic paths
  vtkSmartPointer<vtkAppendPolyData> appender = vtkSmartPointer<vtkAppendPolyData>::New();

  vtkSmartPointer<vtkIdList> seeds = vtkSmartPointer<vtkIdList>::New();
  seeds->SetNumberOfIds(1);

  double totalLength = 0.0;
  if (nearestPtIds->GetNumberOfIds() > 0)
  {
    for (vtkIdType idx = 0; idx < nearestPtIds->GetNumberOfIds() - 1; ++idx)
    {
      // Get two adjacent endpoint IDs
      vtkIdType ptId0 = nearestPtIds->GetId(idx);
      vtkIdType ptId1 = nearestPtIds->GetId(idx + 1);

      // Compute the geodesic between the endpoints
      path->SetBeginPointId(ptId0);
      seeds->SetId(0, ptId1);
      path->SetSeeds(seeds);
      path->Update();

      // Append the geodesic to the total geodesic geometry
      vtkSmartPointer<vtkPolyData> pathOutput = vtkSmartPointer<vtkPolyData>::New();
      pathOutput->ShallowCopy(path->GetOutput());

      appender->AddInputData(pathOutput);

      // Add the length to the total length
      totalLength += path->GetGeodesicLength();
    }

    if (this->Loop)
    {
      vtkIdType ptId0 = nearestPtIds->GetId(nearestPtIds->GetNumberOfIds() - 1);
      vtkIdType ptId1 = nearestPtIds->GetId(0);

      if (this->LoopWithLine)
      {
        // Create a poly data with a single line segment
        double pt0[3], pt1[3];
        input->GetPoint(ptId0, pt0);
        input->GetPoint(ptId1, pt1);

        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(2);
        points->SetPoint(0, pt0);
        points->SetPoint(1, pt1);

        vtkIdType ptIds[2] = { 0, 1 };
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        cells->InsertNextCell(2, ptIds);

        vtkSmartPointer<vtkPolyData> linePD = vtkSmartPointer<vtkPolyData>::New();
        linePD->Allocate();
        linePD->SetPoints(points);
        linePD->SetLines(cells);

        appender->AddInputData(linePD);

        totalLength += sqrt(vtkMath::Distance2BetweenPoints(pt0, pt1));
      }
      else
      {
        path->SetBeginPointId(ptId0);
        seeds->SetId(0, ptId1);
        path->SetSeeds(seeds);
        path->Update();

        // Append the geodesic to the total geodesic geometry
        vtkSmartPointer<vtkPolyData> pathOutput = vtkSmartPointer<vtkPolyData>::New();
        pathOutput->ShallowCopy(path->GetOutput());

        appender->AddInputData(pathOutput);

        // Add the length to the total length
        totalLength += path->GetGeodesicLength();
      }
    }
  }

  // Clean the output
  vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
  cleaner->PointMergingOn();
  cleaner->SetInputConnection(appender->GetOutputPort());
  cleaner->Update();

  // Copy the output of the cleaner to the output
  output->ShallowCopy(cleaner->GetOutput());

  // Set the total length in the output's field data
  vtkSmartPointer<vtkDoubleArray> totalLengthArray = vtkSmartPointer<vtkDoubleArray>::New();
  totalLengthArray->SetName("TotalLength");
  totalLengthArray->SetNumberOfComponents(1);
  totalLengthArray->SetNumberOfTuples(1);
  totalLengthArray->InsertTypedTuple(0, &totalLength);

  vtkFieldData* fieldData = output->GetFieldData();
  fieldData->AddArray(totalLengthArray);

  return 1;
}

//----------------------------------------------------------------------------
void vtkGeodesicsBetweenPoints::SetEndpointsConnection(vtkAlgorithmOutput* endpointsConnection)
{
  this->SetInputConnection(1, endpointsConnection);
}

//-----------------------------------------------------------------------------
void vtkGeodesicsBetweenPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
