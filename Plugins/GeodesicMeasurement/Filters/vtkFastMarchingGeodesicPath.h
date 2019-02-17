/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastMarchingGeodesicPath.h

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

// .NAME vtkFastMarchingGeodesicDistance - Computes geodesic path using gradient descent of a
// distance function
// .SECTION Description
// This class computes geodesic paths between a destination and one or more
// sources on the mesh. To compute the path, a geodesic distance map to one
// or more seeds on the mesh is computed using fast marching. Subsequently,
// the geodesic path joining the path's starting point to its closest
// source point is computed via gradient descent on the distance map.
//
// .SECTION Parameters
// <p>1) Maximum path length: A maximum path length may optionally be set. If
// set, gradient descent can terminate prematurely, without reaching a source
// vertex.
// <p>2) Interpolation: The path traced by gradient descent traverses on the
// mesh between vertices. The "InterpolationOrder" can be used to constrain
// traversal along the mesh vertices by setting it to 0. The computed path
// points lie on the edges of the mesh in case of first order interpolation,
// while in the case of zeroth order interpolation, they lie on the vertices.
//
// .SECTION Outputs
// The filter returns as output a polydata containing the geodesic path.
// The closest point ids on the mesh to the path are contained in
// ZerothOrderPathPointIds. The list FirstOrderPathPointIds contains the
// bounding vertices of the path. These are organized in pairs, each of which
// represents an edge on the mesh, through which the path points pass. The path
// length may be queried via GetGeodesicLength.
//
// .SECTION See also
// vtkFastMarchingGeodesicDistance
//
// .SECTION References
// 1. Peyre, Cohen, "Geodesic Methods for Shape and Surface Processing" [2008]
// 2. Peyre, Cohen, "Geodesic Computations for Fast and Accurate Surface
//    Remeshing and Parameterization", Progress in Nonlinear Differential
//    Equations and Their Applications, 2005.
#ifndef vtkFastMarchingGeodesicPath_h
#define vtkFastMarchingGeodesicPath_h

#include "vtkGeodesicMeasurementFiltersModule.h" // for export macro
#include "vtkGeodesicPath.h"

class vtkIdList;
class vtkPoints;
class vtkFastMarchingGeodesicDistance;

class VTKGEODESICMEASUREMENTFILTERS_EXPORT vtkFastMarchingGeodesicPath : public vtkGeodesicPath
{
public:
  static vtkFastMarchingGeodesicPath* New();

  // Description:
  // Standard methods for printing and determining type information.
  vtkTypeMacro(vtkFastMarchingGeodesicPath, vtkGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // The instance of the geodesic filter.
  vtkGetObjectMacro(Geodesic, vtkFastMarchingGeodesicDistance);

  // Description:
  // Interpolation order of the path traced through the surface mesh. A zeroth
  // order path passes through vertices of the mesh. A first order path passes
  // in between vertices. Each point in the first order path is guaranteed to
  // lie on an edge. Default is first order.
  vtkSetClampMacro(InterpolationOrder, int, 0, 1);
  vtkGetMacro(InterpolationOrder, int);

  // Description:
  // Get the point ids corresponding to the path points. These are point ids
  // on the input mesh.
  // - The Zeroth order point ids contain the ids of vertices on the mesh
  // closest to the path. These correspond exactly to the path points if the
  // InterpolationOrder is 0.
  // - The first order path point ids are ids of the edge. There are twice as
  // many as the number of points in the path, organized in pairs comprising
  // the point ids of the  edge. These ids are populated only when the
  // InterpolationOrder is 1.
  vtkGetObjectMacro(ZerothOrderPathPointIds, vtkIdList);
  vtkGetObjectMacro(FirstOrderPathPointIds, vtkIdList);

  // Description:
  // A maximum path length may be specified, in which case, the gradient based
  // back-tracking from 'Begin' to 'seeds' may stop prematurely once it
  // exceeds the specified length. The default is infinite.
  vtkSetMacro(MaximumPathPoints, float);
  vtkGetMacro(MaximumPathPoints, float);

  // Description:
  // Add termination point ids. These are troughs towards which the
  // path will traverse and at which it terminates.
  virtual void SetSeeds(vtkIdList*);
  virtual vtkIdList* GetSeeds();

  // Description:
  // The point id from which the path begins
  vtkSetMacro(BeginPointId, vtkIdType);
  vtkGetMacro(BeginPointId, vtkIdType);

  // Description:
  // Get the length of the traced path
  vtkGetMacro(GeodesicLength, double);

protected:
  vtkFastMarchingGeodesicPath();
  ~vtkFastMarchingGeodesicPath() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Do the fast marching and gradient backtracking
  virtual void ComputePath(vtkPolyData*);

  float MaximumPathPoints;
  double GeodesicLength;
  int InterpolationOrder;
  vtkIdList* ZerothOrderPathPointIds;
  vtkIdList* FirstOrderPathPointIds;
  vtkIdType BeginPointId;
  vtkFastMarchingGeodesicDistance* Geodesic;

private:
  vtkFastMarchingGeodesicPath(const vtkFastMarchingGeodesicPath&) = delete;
  void operator=(const vtkFastMarchingGeodesicPath&) = delete;
};

#endif
