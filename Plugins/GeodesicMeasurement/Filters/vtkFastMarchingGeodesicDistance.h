/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastMarchingGeodesicDistance.h

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

// .NAME vtkFastMarchingGeodesicDistance - Generates a distance field on a mesh
// .SECTION Description
// The class generates a geodesic distance field from a seed or set of seeds
// one a surface mesh. This is done using the Fast marching method (Setian96).
// The Fast marching toolkit by Gabriel Peyre is used. In short, this is the
// viscosity solution of the Eikonal equation norm(grad(D))=P. The level set,
// {x \ F(x)=t} can be seen as a front advancing with speed P(x). The resulting
// function D is a distance function, and if the speed P is constant, it can
// be seen as the distance function to a set of starting points.
//
// .SECTION Inputs and Outputs
// The input to the filter must be a triangle mesh. The output is the same mesh
// with a point data attribute capturing the distance field from the user
// specified seed(s) via SetSeedList or SetSeedsFromNonZeroField.
//
// .SECTION Termination Criteria
// The fast marching may be prematurely terminated via any of the optional
// stopping criteria. These are:
// (a) Distance stop criterion: The fast marching stop if any portion of the
// front has traversed more than the specified distance from the seed(s).
// See SetDistanceStopCriterion(float)
// (b) Destination vertex stop criterion: The fast marching stops if any
// portion of the front reaches the user supplied destination vertex id(s).
// See SetDestinationVertexStopCriterion(vtkIdList)
//
// .SECTION Exclusion Regions
// Optionally, an exclusion region may be specified. Vertices with ids that
// are in the exclusion list are omitted from inclusion in the fast marching
// front. This can be used to prevent fast from bleeding into certain regions
// by supplying the point ids of the region boundary/boundaries. Conversely, it
// can be used to confine fast marching to a specific region.
//
// .SECTION Propagation weights
// The default propagation weights are constant and isotropic = 1. One may
// set the propagation weights via SetPropagationWeights. This is a float array
// with as many entries as the number of vertices on the mesh. For instance the
// an inverse function of the mesh curvature may be used to have fast marching
// propagate quickly in regions of low curvature and slow down in regions of
// high curvature. Note that the propagation weights must be strictly positive.
//
// .SECTION Miscellaneous
// The filter reports IterationEvents. It does not report progress events,
// since its not possible to pre-determine when the front might terminate.
//
// .SECTION References
// 1. Peyre, Cohen, "Geodesic Methods for Shape and Surface Processing" [2008]
// 2. Peyre, Cohen, "Geodesic Computations for Fast and Accurate Surface
//    Remeshing and Parameterization", Progress in Nonlinear Differential
//    Equations and Their Applications, 2005.

#ifndef vtkFastMarchingGeodesicDistance_h
#define vtkFastMarchingGeodesicDistance_h

#include "vtkGeodesicMeasurementFiltersModule.h" // for export macro
#include "vtkPolyDataGeodesicDistance.h"

class vtkPolyData;
class vtkDataArray;
class vtkGeodesicMeshInternals;

class VTKGEODESICMEASUREMENTFILTERS_EXPORT vtkFastMarchingGeodesicDistance
  : public vtkPolyDataGeodesicDistance
{
public:
  static vtkFastMarchingGeodesicDistance* New();

  // Description:
  // Standard methods for printing and determining type information.
  vtkTypeMacro(vtkFastMarchingGeodesicDistance, vtkPolyDataGeodesicDistance);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // The maximum distance this filter has marched from the seeds.
  vtkGetMacro(MaximumDistance, float);

  // Description:
  // Set the value to set as the point attribute data for vertices that
  // haven't been visited (perhaps due to a termination criterion) by the
  // fast marching. Defaults to -1.
  vtkSetMacro(NotVisitedValue, float);
  vtkGetMacro(NotVisitedValue, float);

  // Description:
  // Get the number of points visited by fast marching
  vtkGetMacro(NumberOfVisitedPoints, vtkIdType);

  // Description:
  // Optionally stopping criteria may be specified. This method may be used to
  // restrict fast marching to a 'distance' radius from the seed(s). The
  // default is -1 which implies no stopping criteria.
  vtkSetMacro(DistanceStopCriterion, float);
  vtkGetMacro(DistanceStopCriterion, float);

  // Description:
  // Optionally stopping criteria may be specified. This method may be used to
  // stop fast marching when a specific destination vertex (or vertices) have
  // been reached. The default is to have no stopping criteria.
  virtual void SetDestinationVertexStopCriterion(vtkIdList* vertices);
  vtkGetObjectMacro(DestinationVertexStopCriterion, vtkIdList);

  // Description:
  // Optionally, an exclusion region may be specified. Vertices with ids that
  // are in the exclusion list are omitted from inclusion in the fast marching
  // front. This can be used to prevent fast from bleeding into certain regions
  // by supplying the point ids of the region boundary/boundaries.
  virtual void SetExclusionPointIds(vtkIdList* vertices);
  vtkGetObjectMacro(ExclusionPointIds, vtkIdList);

  // Description:
  // Optionally, point weights may be specified. This amounts to specifying a
  // non-uniform speed function. Each point may for instance be weighted
  // inversely by the point curvature resulting in the front propagating
  // rapidly through areas of low curvature and slowing down in regions of high
  // curvature. The size of the weights array must be the same as that of the
  // surface mesh. If weights aren't specified, it amounts to having a constant
  // propagation weight of 1 everywhere.
  virtual void SetPropagationWeights(vtkDataArray*);
  vtkGetObjectMacro(PropagationWeights, vtkDataArray);

  // Description:
  // Events invoked by the filter

  enum
  {
    IterationEvent = 10590
  };

protected:
  vtkFastMarchingGeodesicDistance();
  ~vtkFastMarchingGeodesicDistance() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Create GW_GeodesicMesh given an instance of a vtkPolyData
  void SetupGeodesicMesh(vtkPolyData* in);

  // Setup the optional termination criteria, if set
  void SetupCallbacks();

  // Do the fast marching
  int Compute() override;

  // Add the seeds
  virtual void AddSeedsInternal();

  // Add the seeds based on the non-zero values of a nonZeroField
  void SetSeedsFromNonZeroField(vtkDataArray* nonZeroField);

  // Copy the resulting distance field from GeoMesh into the float array
  void CopyDistanceField(vtkPolyData* pd);

  // The internal GW_GeodsicMesh structure
  vtkGeodesicMeshInternals* Internals;

  // Time the GW_GeodesicMesh datastructure was last built from a vtkPolyData
  vtkTimeStamp GeodesicMeshBuildTime;

  // The maximum distance we've marched.
  float MaximumDistance;

  // Distance value to assign to verts not visited
  float NotVisitedValue;

  // Number of points visited by fast marching
  vtkIdType NumberOfVisitedPoints;

  // Distance stop criteria
  float DistanceStopCriterion;

  // Destination vertex stop criteria
  vtkIdList* DestinationVertexStopCriterion;

  // Exclusion regions
  vtkIdList* ExclusionPointIds;

  // Propagation, ie speed function weights
  vtkDataArray* PropagationWeights;

  friend class vtkFastMarchingGeodesicPath;
  friend class vtkGeodesicMeshInternals;
  void* GetGeodesicMesh();

  // Counter to invoke iteration events every N fast marching steps
  unsigned long FastMarchingIterationEventResolution;
  unsigned long IterationIndex;

private:
  vtkFastMarchingGeodesicDistance(const vtkFastMarchingGeodesicDistance&) = delete;
  void operator=(const vtkFastMarchingGeodesicDistance&) = delete;
};

#endif
