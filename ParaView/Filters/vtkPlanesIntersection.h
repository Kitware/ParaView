// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlanesIntersection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

// .NAME vtkPlanesIntersection - Computes whether the convex region
//    defined by it's planes intersects an axis aligned box.
//
// .SECTION Description
//    A subclass of vtkPlanes, this class determines whether it
//    intersects an axis aligned box.   This is motivated by the
//    need to intersect the axis aligned region of a spacial
//    decomposition of volume data with a view frustum created by
//    a rectangular portion of the view plane.  It uses the 
//    algorithm from Graphics Gems IV, page 81.
//
// .SECTION Caveat
//    An instance of vtkPlanes can be redefined by changing the planes,
//    but this subclass then will not know if the region vertices are
//    up to date.  (Region vertices can be specified in SetRegionVertices
//    or computed by the subclass.)  So Delete and recreate if you want
//    to change the set of planes.
//

#ifndef __vtkPlanesIntersection_h
#define __vtkPlanesIntersection_h

//#include <vtksnlGraphicsWin32Header.h>
#include <vtkObjectFactory.h>
#include <vtkSetGet.h>
#include <vtkPoints.h>
#include <vtkPlanes.h>
#include <vtkPlane.h>
#include <vtkPointsProjectedHull.h>


class VTK_EXPORT vtkPlanesIntersection : public vtkPlanes
{
    vtkTypeRevisionMacro(vtkPlanesIntersection, vtkPlanes);

public:
    void PrintSelf(ostream& os, vtkIndent indent);

    static vtkPlanesIntersection *New();

    // Description:
    //   It helps if you know the vertices of the convex region.
    //   If you don't, we will calculate them.  Region vertices
    //   are 3-tuples.

    void SetRegionVertices(vtkPoints *pts);
    void SetRegionVertices(float *v, int nvertices);
    void SetRegionVertices(double *v, int nvertices);
    int GetNumRegionVertices();
    int GetRegionVertices(float *v, int nvertices);
    int GetRegionVertices(double *v, int nvertices);

    // Description:
    //   Return 1 if the axis aligned box defined by R intersects
    //   the region defined by the planes, or 0 otherwise.
    
    int IntersectsRegion(vtkPoints *R);

protected:

    vtkPlanesIntersection();
    ~vtkPlanesIntersection();

private:

  int IntersectsBoundingBox(vtkPoints *R);
  int EnclosesBoundingBox(vtkPoints *R);
  int EvaluateFacePlane(int plane, vtkPoints *R);
  int IntersectsProjection(vtkPoints *R, int direction);

  void SetPlaneEquations();
  void ComputeRegionVertices();

  const void planesMatrix(int p1, int p2, int p3, double M[3][3]);
  const int duplicate(double testv[3]);
  const void planesRHS(int p1, int p2, int p3, double r[3]);
  const int outsideRegion(double v[3]);

  static int Invert3x3(double M[3][3]);

  // plane equations
  double *Plane;

  // vertices of convex regions enclosed by the planes, also
  //    the ccw hull of that region projected in 3 orthog. directions
  vtkPointsProjectedHull *regionPts;
};
#endif


