/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastMarchingGeodesicDistance.cxx

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

#include "vtkFastMarchingGeodesicDistance.h"

#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include "gw_core/GW_Face.h"
#include "gw_core/GW_Vertex.h"
#include "gw_geodesic/GW_GeodesicMesh.h"
#include "gw_geodesic/GW_GeodesicPath.h"
#include <assert.h>
#include <set>

#ifdef _WIN32
// new is being defined to a new method that takes in 4 parameters.
// Go back to what its supposed to be !
#ifdef new
#undef new
#endif
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkFastMarchingGeodesicDistance);
vtkCxxSetObjectMacro(vtkFastMarchingGeodesicDistance, DestinationVertexStopCriterion, vtkIdList);
vtkCxxSetObjectMacro(vtkFastMarchingGeodesicDistance, ExclusionPointIds, vtkIdList);
vtkCxxSetObjectMacro(vtkFastMarchingGeodesicDistance, PropagationWeights, vtkDataArray);

//-----------------------------------------------------------------------------
class vtkGeodesicMeshInternals
{
public:
  vtkGeodesicMeshInternals() { this->Mesh = NULL; }

  ~vtkGeodesicMeshInternals()
  {
    if (this->Mesh)
    {
      delete this->Mesh;
    }
  }

  // This callback is called every time a front vertex is visited to check
  // if we should terminate marching.
  static GW::GW_Bool FastMarchingStopCallback(GW::GW_GeodesicVertex& v, void* callbackData)
  {
    vtkFastMarchingGeodesicDistance* filter =
      static_cast<vtkFastMarchingGeodesicDistance*>(callbackData);

    // Stop if the vertex is farther than the distance stop criteria
    if (filter->DistanceStopCriterion > 0)
    {
      return (filter->DistanceStopCriterion <= v.GetDistance());
    }

    // Stop if the vertex id is one of the destination vertices
    if (filter->DestinationVertexStopCriterion->GetNumberOfIds())
    {
      if (filter->DestinationVertexStopCriterion->IsId(v.GetID()) != -1)
      {
        return true;
      }
    }

    return false;
  }

  // This callback is invoked prior to adding new vertices to the front
  static GW::GW_Bool FastMarchingVertexInsertionCallback(
    GW::GW_GeodesicVertex& v, GW::GW_Float /*distance*/, void* callbackData)
  {
    vtkFastMarchingGeodesicDistance* filter =
      static_cast<vtkFastMarchingGeodesicDistance*>(callbackData);

    // Prevent bleeding into exclusion regions
    if (filter->ExclusionPointIds->GetNumberOfIds())
    {
      if (filter->ExclusionPointIds->IsId(v.GetID()) != -1)
      {
        // do not add it.
        return false;
      }
    }

    return true;
  }

  // This callback is invoked to get the propagation weight at a given vertex.
  // The default (if not specified) is a constant weight of 1 everywhere.
  static GW::GW_Float FastMarchingPropagationWeightCallback(
    GW::GW_GeodesicVertex& v, void* callbackData)
  {
    vtkFastMarchingGeodesicDistance* filter =
      static_cast<vtkFastMarchingGeodesicDistance*>(callbackData);

    return (GW::GW_Float)filter->PropagationWeights->GetTuple1(v.GetID());
  }

  // This callback is invoked to get the propagation weight at a given vertex.
  // Th result is no weight == 1
  static inline GW::GW_Float FastMarchingPropagationNoWeightCallback(GW::GW_GeodesicVertex&, void*)
  {
    return 1.0;
  }

  GW::GW_GeodesicMesh* Mesh;
};

//-----------------------------------------------------------------------------
vtkFastMarchingGeodesicDistance::vtkFastMarchingGeodesicDistance()
{
  this->Internals = new vtkGeodesicMeshInternals;
  this->MaximumDistance = 0;
  this->NotVisitedValue = -1;
  this->NumberOfVisitedPoints = 0;
  this->DistanceStopCriterion = -1;
  this->DestinationVertexStopCriterion = NULL;
  this->ExclusionPointIds = NULL;
  this->PropagationWeights = NULL;
  this->IterationIndex = 0;
  this->FastMarchingIterationEventResolution = 100;
}

//-----------------------------------------------------------------------------
vtkFastMarchingGeodesicDistance::~vtkFastMarchingGeodesicDistance()
{
  this->SetDestinationVertexStopCriterion(NULL);
  this->SetExclusionPointIds(NULL);
  this->SetPropagationWeights(NULL);
  delete this->Internals;
}

//----------------------------------------------------------------------------
int vtkFastMarchingGeodesicDistance::RequestData(vtkInformation* vtkNotUsed(request),
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

  // Copy everything from the input
  output->ShallowCopy(input);

  // Initialize the GW_GeodesicMesh structure
  this->SetupGeodesicMesh(input);

  // Setup termination criteria, if any
  this->SetupCallbacks();

  // Extract seed point id list as points with non-zero values of a given field
  vtkDataArray* inNonZeroField = this->GetInputArrayToProcess(0, input);
  if (inNonZeroField)
  {
    this->SetSeedsFromNonZeroField(inNonZeroField);
  }

  // Set propagation weight field. NULL case is handled internally
  vtkDataArray* inIsotropicMetricTensorLength = this->GetInputArrayToProcess(1, input);
  this->SetPropagationWeights(inIsotropicMetricTensorLength);

  // Internally setup seeds for fast marching
  this->AddSeedsInternal();

  // Do the fast marching
  this->Compute();

  // Copy the distance field onto the output
  this->CopyDistanceField(output);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicDistance::SetupGeodesicMesh(vtkPolyData* in)
{
  // If the input or the filter parameters have changed since the last
  // execution, or we are running for the first time..
  if (this->GeodesicMeshBuildTime.GetMTime() < in->GetMTime() || !this->Internals->Mesh)
  {
    // Need to rebuild the GW_GeodesicMesh

    if (!this->Internals->Mesh)
    {
      // Delete the internal instance and re-populate
      delete this->Internals->Mesh;
      this->Internals->Mesh = new GW::GW_GeodesicMesh();
      this->Internals->Mesh->SetCallbackData(this);
    }

    // Setup the GW_GeodesicMesh mesh
    GW::GW_GeodesicMesh* mesh = this->Internals->Mesh;

    // Setup the mesh points
    double pt[3];
    vtkPoints* pts = in->GetPoints();
    const int nPts = in->GetNumberOfPoints();

    // Allocate vertices
    mesh->SetNbrVertex(nPts);

    // loop over the points and copy them over
    for (int i = 0; i < nPts; i++)
    {
      pts->GetPoint(i, pt);
      GW::GW_GeodesicVertex& point = (GW::GW_GeodesicVertex&)mesh->CreateNewVertex();
      point.SetPosition(GW::GW_Vector3D(pt[0], pt[1], pt[2]));
      mesh->SetVertex(i, &point);
    }

    vtkIdType npts = 0;
    const vtkIdType* ptIds = nullptr;
    const int nCells = in->GetNumberOfPolys();
    vtkCellArray* cells = in->GetPolys();
    if (!cells)
    {
      return;
    }
    cells->InitTraversal();

    // Allocate number of cells
    mesh->SetNbrFace(nCells);

    for (int i = 0; i < nCells; i++)
    {
      // Possible types
      //    VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE,
      //    VTK_POLY_LINE,VTK_TRIANGLE, VTK_QUAD,
      //    VTK_POLYGON, or VTK_TRIANGLE_STRIP.

      // only handle triangles
      cells->GetNextCell(npts, ptIds);

      // bail out if we encounter anything other than triangles
      if (npts != 3)
      {
        vtkErrorMacro(<< "This filter works only with triangle meshes. Triangulate first.");
        delete this->Internals->Mesh;
        this->Internals->Mesh = NULL;
        return;
      }

      // construct the face and add add it to our internal GeodesicMesh
      GW::GW_GeodesicFace& cell = (GW::GW_GeodesicFace&)mesh->CreateNewFace();
      GW::GW_Vertex* a = mesh->GetVertex(ptIds[0]);
      GW::GW_Vertex* b = mesh->GetVertex(ptIds[1]);
      GW::GW_Vertex* c = mesh->GetVertex(ptIds[2]);
      cell.SetVertex(*a, *b, *c);
      mesh->SetFace(i, &cell);
    }

    // Setup the neighborhood for each face prior to fast marching. Builds the
    // inverse map vert -> face
    mesh->BuildConnectivity();

    // Update timestamp
    this->GeodesicMeshBuildTime.Modified();
  }

  // Restart in preparation for fast marching
  this->Internals->Mesh->ResetGeodesicMesh();
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicDistance::AddSeedsInternal()
{
  if (!this->Seeds || !this->Seeds->GetNumberOfIds())
  {
    vtkErrorMacro(<< "Please supply at least one seed.");
    return;
  }

  // Add the seeds to the internal geodesic mesh instance
  const int n = this->Seeds->GetNumberOfIds();
  GW::GW_GeodesicMesh* mesh = this->Internals->Mesh;
  for (int i = 0; i < n; i++)
  {
    mesh->AddStartVertex(
      *((GW::GW_GeodesicVertex*)mesh->GetVertex((GW::GW_U32)(this->Seeds->GetId(i)))));
  }
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicDistance::SetSeedsFromNonZeroField(vtkDataArray* nonZeroField)
{
  vtkIdType numpts = nonZeroField->GetNumberOfTuples();
  // First extract seeds ids
  vtkNew<vtkIdList> seedsId;
  for (vtkIdType pointIdx = 0; pointIdx < numpts; pointIdx++)
  {
    if (nonZeroField->GetTuple1(pointIdx) != 0.0)
    {
      seedsId->InsertNextId(pointIdx);
    }
  }
  this->SetSeeds(seedsId.Get());
}

//-----------------------------------------------------------------------------
int vtkFastMarchingGeodesicDistance::Compute()
{
  this->MaximumDistance = 0;

  this->Internals->Mesh->SetUpFastMarching();

  // Do the fast marching

  while (!this->Internals->Mesh->PerformFastMarchingOneStep())
  {
    if ((++this->IterationIndex) % this->FastMarchingIterationEventResolution == 0)
    {
      // Invoke iteration events every so often (see the resolution)
      // parameter.
      this->InvokeEvent(vtkFastMarchingGeodesicDistance::IterationEvent);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicDistance::CopyDistanceField(vtkPolyData* pd)
{
  GW::GW_GeodesicMesh* mesh = this->Internals->Mesh;

  float distance;
  this->MaximumDistance = 0;
  this->NumberOfVisitedPoints = 0;

  const int n = mesh->GetNbrVertex();

  // get the field array to populate into
  vtkFloatArray* arr = this->GetGeodesicDistanceField(pd);

  // Loop over the GW_GeodesicVertex's and copy the field over
  // depending on the state of the vertex (visited or not).

  for (int i = 0; i < n; i++)
  {
    GW::GW_GeodesicVertex* vertex = (GW::GW_GeodesicVertex*)(mesh->GetVertex((GW::GW_U32)i));

    if (vertex->GetState() > 1)
    {
      // This point is in the traversal list

      ++this->NumberOfVisitedPoints;

      // get the fast marching distance
      distance = vertex->GetDistance();

      // record the farthest geodesic distance we've marched
      if (distance > this->MaximumDistance)
      {
        this->MaximumDistance = distance;
      }

      // if the array is defined store the distance. It is possible to run
      // this filter without the overhead of allocating and copying the
      // distance field array. (For instance the GeodesicPath is interested
      // in computing the path from the internally computed field. There is
      // no need to copy over a vtkDataArray with the distance field).
      if (arr)
      {
        arr->SetValue(i, distance);
      }
    }
    else
    {
      // This point has not been marched to yet

      if (arr)
      {
        arr->SetValue(i, this->NotVisitedValue);
      }
    }

  } // end loop over all vertices
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicDistance::SetupCallbacks()
{
  // Setup various callbacks invoked during fast marching.

  // Termination criteria. The ForceStopCallbackFunction is used to test if we
  // should end the fast marching or not.
  // We use this callback to check if a set of user defined destination
  // vertices have been reached, or if we've marched beyond a user specified
  // distance.
  if (this->DistanceStopCriterion > 0 || (this->DestinationVertexStopCriterion &&
                                           this->DestinationVertexStopCriterion->GetNumberOfIds()))
  {
    this->Internals->Mesh->RegisterForceStopCallbackFunction(
      vtkGeodesicMeshInternals::FastMarchingStopCallback);
  }
  else
  {
    this->Internals->Mesh->RegisterForceStopCallbackFunction(NULL);
  }

  // Setup callback prior to adding a new vertex into the front...
  // The VertexInsersionCallbackFunction is invoked prior to adding a new
  // vertex to the front. Here we check if the added vertices belong to the
  // "ExclusionPointIds".
  if (this->ExclusionPointIds && this->ExclusionPointIds->GetNumberOfIds())
  {
    this->Internals->Mesh->RegisterVertexInsersionCallbackFunction(
      vtkGeodesicMeshInternals::FastMarchingVertexInsertionCallback);
  }
  else
  {
    this->Internals->Mesh->RegisterVertexInsersionCallbackFunction(NULL);
  }

  // Setup callback to get the propagation weights
  // The WeightCallbackFunction is used to define the metric on the mesh.
  if (this->PropagationWeights &&
    static_cast<GW::GW_U32>(this->PropagationWeights->GetNumberOfTuples()) ==
      this->Internals->Mesh->GetNbrVertex())
  {
    this->Internals->Mesh->RegisterWeightCallbackFunction(
      vtkGeodesicMeshInternals::FastMarchingPropagationWeightCallback);
  }
  else
  {
    // assumes uniform weight of 1.
    this->Internals->Mesh->RegisterWeightCallbackFunction(
      vtkGeodesicMeshInternals::FastMarchingPropagationNoWeightCallback);
  }
}

//-----------------------------------------------------------------------------
void* vtkFastMarchingGeodesicDistance::GetGeodesicMesh()
{
  return this->Internals->Mesh;
}

//-----------------------------------------------------------------------------
void vtkFastMarchingGeodesicDistance::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MaximumDistance: " << this->MaximumDistance << endl;
  os << indent << "NotVisitedValue: " << this->NotVisitedValue << endl;
  os << indent << "NumberOfVisitedPoints: " << this->NumberOfVisitedPoints << endl;
  os << indent << "DistanceStopCriterion: " << this->DistanceStopCriterion << endl;
  os << indent << "DestinationVertexStopCriterion: " << this->DestinationVertexStopCriterion
     << endl;
  if (this->DestinationVertexStopCriterion)
  {
    this->DestinationVertexStopCriterion->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "ExclusionPointIds: " << this->ExclusionPointIds << endl;
  if (this->ExclusionPointIds)
  {
    this->ExclusionPointIds->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "PropagationWeights: " << this->PropagationWeights << endl;
  if (this->PropagationWeights)
  {
    this->PropagationWeights->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent
     << "FastMarchingIterationEventResolution: " << this->FastMarchingIterationEventResolution
     << endl;
  os << indent << "IterationIndex: " << this->IterationIndex << endl;
  // GeodesicMeshBuildTime
}
