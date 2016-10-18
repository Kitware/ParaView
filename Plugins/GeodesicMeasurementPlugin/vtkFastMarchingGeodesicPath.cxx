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
/*=========================================================================

  Copyright (c) 2013 Karthik Krishnan.
  Contributed to the VisualizationToolkit by the author under the terms
  of the Visualization Toolkit copyright

=========================================================================*/
#include "vtkFastMarchingGeodesicPath.h"

#include "vtkCellArray.h"
#include "vtkExecutive.h"
#include "vtkFastMarchingGeodesicDistance.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "gw_core/GW_Config.h"
#include "gw_core/GW_Face.h"
#include "gw_core/GW_Vertex.h"
#include "gw_geodesic/GW_GeodesicMesh.h"
#include "gw_geodesic/GW_GeodesicPath.h"
#include <assert.h>
#include <set>

#ifdef _WIN32
// new is being defined to a new method that takes in 4 parameters within
// one of the GeodesicMesh headers. Go back to what its supposed to be or
// this can wreck havoc.
#ifdef new
#undef new
#endif
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkFastMarchingGeodesicPath);

//-----------------------------------------------------------------------------
vtkFastMarchingGeodesicPath::vtkFastMarchingGeodesicPath()
{
  this->MaximumPathPoints = GW_INFINITE; // no limit
  this->InterpolationOrder = 1;          // linear by default
  this->BeginPointId = -1;               // undefined
  this->Geodesic = vtkFastMarchingGeodesicDistance::New();
  this->ZerothOrderPathPointIds = vtkIdList::New();
  this->FirstOrderPathPointIds = vtkIdList::New();
  this->GeodesicLength = 0;
}

//-----------------------------------------------------------------------------
vtkFastMarchingGeodesicPath::~vtkFastMarchingGeodesicPath()
{
  this->ZerothOrderPathPointIds->Delete();
  this->FirstOrderPathPointIds->Delete();
  this->Geodesic->Delete();
}

//----------------------------------------------------------------------------
int vtkFastMarchingGeodesicPath::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input)
  {
    return 0;
  }

  this->Geodesic->SetInputData(input);

  // Setup the termination criteria such that we stop marching when we reach
  // the destination.
  vtkNew<vtkIdList> terminationIds;
  terminationIds->InsertNextId(this->BeginPointId);
  this->Geodesic->SetDestinationVertexStopCriterion(terminationIds.GetPointer());

  // This will re-run fast marching and compute the distance field from the
  // seeded points, if necessary (if the mesh or seeds have changed)
  this->Geodesic->Update();

  // - Compute the path by gradient backtracking of the distance field.
  // - Also copy the path from the GW datastructures into a vtkPolyData. The
  //   result is a polydata containing an open polyline with a single cell.
  // - Also copy the point ids of the closest and the bounding vertices of
  //   the path.
  this->ComputePath(output);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicPath::SetSeeds(vtkIdList* seeds)
{
  this->Geodesic->SetSeeds(seeds);
}

//-----------------------------------------------------------------------------
vtkIdList* vtkFastMarchingGeodesicPath::GetSeeds()
{
  return this->Geodesic->GetSeeds();
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicPath::ComputePath(vtkPolyData* pd)
{
  // clear the length and the path point ids
  this->GeodesicLength = 0;
  this->ZerothOrderPathPointIds->Initialize();
  this->FirstOrderPathPointIds->Initialize();

  // The points defining the path
  vtkSmartPointer<vtkPoints> pathPoints = vtkSmartPointer<vtkPoints>::New();
  pathPoints->Initialize();

  GW::GW_GeodesicMesh* mesh = (GW::GW_GeodesicMesh*)(this->Geodesic->GetGeodesicMesh());
  GW::GW_GeodesicVertex* begin =
    (GW::GW_GeodesicVertex*)(mesh->GetVertex((GW::GW_U32) this->BeginPointId));

  // Sanity check to ensure that the start point for the gradient tracing
  // does indeed lie on the mesh.
  if (!begin)
  {
    vtkErrorMacro(<< "BeginPointId was not found to lie on the mesh.");
    return;
  }

  // Do a gradient tracing from a point
  GW::GW_GeodesicPath track;
  track.ComputePath(*begin, this->MaximumPathPoints);

  // Get the track
  GW::T_GeodesicPointList ptList = track.GetPointList();

  GW::GW_GeodesicPoint* pt;
  float parametricPos;
  GW::GW_GeodesicVertex *endVert1, *endVert2;
  GW::GW_Vector3D endPt1, endPt2;
  double pathPt[3] = { 0.0, 0.0, 0.0 };
  double lastPathPt[3] = { 0.0, 0.0, 0.0 };
  vtkIdType endPtId1, endPtId2, lastInsertedPtId = -1;
  vtkIdType i = 0, i0 = 0;

  const size_t nPts = ptList.size();
  pathPoints->SetNumberOfPoints(nPts);

  // the closet path points on the mesh
  this->ZerothOrderPathPointIds->SetNumberOfIds(nPts);

  // With linear interpolation we return a pair of point ids (corresponding to
  // the triangle edge end points) for each path point.
  if (this->InterpolationOrder == 1)
  {
    this->FirstOrderPathPointIds->SetNumberOfIds(nPts * 2);
  }

  // Loop through points on the track
  for (GW::CIT_GeodesicPointList cit = ptList.begin(), citEnd = ptList.end(); cit != citEnd;
       ++cit, ++i, lastPathPt[0] = pathPt[0], lastPathPt[1] = pathPt[1], lastPathPt[2] = pathPt[2])
  {
    // This is a point on the track
    pt = *cit;

    // The parametric position of the vertex on the edge
    parametricPos = pt->GetCoord();

    // Get the end points of the edge on which the path lies.
    endVert1 = pt->GetVertex1();
    endVert2 = pt->GetVertex2();
    endPt1 = endVert1->GetPosition();
    endPt2 = endVert2->GetPosition();
    endPtId1 = endVert1->GetID();
    endPtId2 = endVert2->GetID();

    // Store the edge point ids. The ZerothOrderPointIds contain the closest
    // one. The FirstOrderPointIds contains the other one.

    if (parametricPos > 0.5) // pt is closer to endPtId1
    {

      if (lastInsertedPtId != endPtId1)
      {
        // avoid repeats
        lastInsertedPtId = endPtId1;
        this->ZerothOrderPathPointIds->SetId(i0, endPtId1);
        pathPt[0] = endPt1[0];
        pathPt[1] = endPt1[1];
        pathPt[2] = endPt1[2];
        if (this->InterpolationOrder == 0)
        {
          pathPoints->SetPoint(i0, pathPt[0], pathPt[1], pathPt[2]);
        }
        ++i0;
      }

      // Store both end points of the path in FirstOrderPathPointIds
      if (this->InterpolationOrder == 1)
      {
        this->FirstOrderPathPointIds->SetId(2 * i, endPtId1);
        this->FirstOrderPathPointIds->SetId(2 * i + 1, endPtId2);
      }
    }
    else // pt is closer to endPtId2
    {
      if (lastInsertedPtId != endPtId2)
      {
        // avoid repeats
        lastInsertedPtId = endPtId2;
        this->ZerothOrderPathPointIds->SetId(i0, endPtId2);
        pathPt[0] = endPt2[0];
        pathPt[1] = endPt2[1];
        pathPt[2] = endPt2[2];
        if (this->InterpolationOrder == 0)
        {
          pathPoints->SetPoint(i0, pathPt[0], pathPt[1], pathPt[2]);
        }
        ++i0;
      }

      // Store both end points of the path in FirstOrderPathPointIds
      if (this->InterpolationOrder == 1)
      {
        this->FirstOrderPathPointIds->SetId(2 * i, endPtId2);
        this->FirstOrderPathPointIds->SetId(2 * i + 1, endPtId1);
      }
    }

    if (this->InterpolationOrder == 1)
    {
      // Linearly interpolate the edge vertices based on the parametric
      // position on the edge
      pathPt[0] = parametricPos * endPt1[0] + (1 - parametricPos) * endPt2[0];
      pathPt[1] = parametricPos * endPt1[1] + (1 - parametricPos) * endPt2[1];
      pathPt[2] = parametricPos * endPt1[2] + (1 - parametricPos) * endPt2[2];

      pathPoints->SetPoint(i, pathPt[0], pathPt[1], pathPt[2]);
    }

    // Compute the curve length
    if (i)
    {
      this->GeodesicLength += sqrt(vtkMath::Distance2BetweenPoints(lastPathPt, pathPt));
    }

  } // end loop over vertices in the gradient trace

  // Set the size to the actual size, which may be less than the track.npts
  // because we avoid repeats.
  this->ZerothOrderPathPointIds->SetNumberOfIds(i0);
  if (this->InterpolationOrder == 0)
  {
    pathPoints->SetNumberOfPoints(i0);
  }

  // Set this path on the output. Its an open polyline with a single cell.

  vtkIdType nUniquePoints = pathPoints->GetNumberOfPoints();
  pd->SetPoints(pathPoints);
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  lines->InsertNextCell(nUniquePoints);
  for (i = 0; i < nUniquePoints; i++)
  {
    lines->InsertCellPoint(i);
  }
  pd->SetLines(lines);
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << this->Geodesic << "\n";
  if (this->Geodesic)
  {
    this->Geodesic->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "BeginPointId: " << this->BeginPointId << "\n";
  os << indent << "InterpolationOrder: " << this->InterpolationOrder << "\n";
  os << indent << "GeodesicLength: " << this->GeodesicLength << "\n";
  os << indent << "MaximumPathPoints: " << this->MaximumPathPoints << "\n";
  os << indent << "ZerothOrderPathPointIds: " << this->ZerothOrderPathPointIds << "\n";
  os << indent << "FirstOrderPathPointIds: " << this->FirstOrderPathPointIds << "\n";
}
