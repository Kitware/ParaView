// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:  vtkPlanesIntersection.cxx
  Language:  C++
  Date:    $Date$
  Version:   $Revision$

=========================================================================*/

#include "vtkMath.h"
#include "vtkPlanesIntersection.h"

vtkCxxRevisionMacro(vtkPlanesIntersection, "1.2");
vtkStandardNewMacro(vtkPlanesIntersection);

//#define TESTING_INTERSECTIONS
// Experiment shows that we get plane equation values on the
//  order of 10e-6 when the point is actually on the plane

#define SMALL_DOUBLE (10e-5)

const int inside   = 0;
const int outside  = 1;        
const int straddle = 2;

const int xdim=0;  // don't change these three values
const int ydim=1;
const int zdim=2;

vtkPlanesIntersection::vtkPlanesIntersection()
{
  this->Plane   = NULL;
  this->regionPts = NULL;
}
vtkPlanesIntersection::~vtkPlanesIntersection()
{
}
void vtkPlanesIntersection::SetRegionVertices(vtkPoints *v)
{
  int i;
  if (this->regionPts) this->regionPts->Delete();
  this->regionPts = vtkPointsProjectedHull::New();

  if (v->GetDataType() == VTK_DOUBLE){
    this->regionPts->DeepCopy(v);
  }
  else{
    this->regionPts->SetDataTypeToDouble();

    int npts = v->GetNumberOfPoints();
    this->regionPts->SetNumberOfPoints(npts);

    float *pt;
    for (i=0; i<npts; i++){
      pt = v->GetPoint(i);

      regionPts->SetPoint(i, (double)pt[0], (double)pt[1], (double)pt[2]);
    }
  }
}
void vtkPlanesIntersection::SetRegionVertices(float *v, int nvertices)
{
  int i;
  double *dv = new double[nvertices*3];

  for (i=0; i<nvertices*3; i++){
    dv[i] = v[i];
  }

  this->SetRegionVertices(dv, nvertices);

  delete [] dv;
}
void vtkPlanesIntersection::SetRegionVertices(double *v, int nvertices)
{
  int i;
  if (this->regionPts) this->regionPts->Delete();
  this->regionPts = vtkPointsProjectedHull::New();
  
  this->regionPts->SetDataTypeToDouble();
  this->regionPts->SetNumberOfPoints(nvertices);
  
  for (i=0; i<nvertices; i++){
    
    this->regionPts->SetPoint(i, v + (i*3));
  }
}
int vtkPlanesIntersection::GetRegionVertices(float *v, int nvertices)
{
  int i;
  double *dv = new double[nvertices*3];

  int npts = this->GetRegionVertices(dv, nvertices);

  for (i=0; i<npts*3; i++){
    v[i] = (float)dv[i];
  }

  delete [] dv;

  return npts;
}
int vtkPlanesIntersection::GetRegionVertices(double *v, int nvertices)
{
  int i;
  if (this->regionPts == NULL) this->ComputeRegionVertices();

  int npts = this->regionPts->GetNumberOfPoints();

  if (npts > nvertices) npts = nvertices;

  for (i=0; i<npts; i++){
    this->regionPts->GetPoint(i, v + i*3);
  }

  return npts;
}
int vtkPlanesIntersection::GetNumRegionVertices()
{
  if (this->regionPts == NULL) this->ComputeRegionVertices();

  return this->regionPts->GetNumberOfPoints();
}
int vtkPlanesIntersection::IntersectsRegion(vtkPoints *R)
{
  int plane;
  int allInside;
  int nplanes = this->GetNumberOfPlanes();

  if (nplanes < 4){
    vtkErrorMacro("invalid region - less than 4 planes");
    return 0;
  }

  if (this->regionPts == NULL){
    this->ComputeRegionVertices();
    if (this->regionPts->GetNumberOfPoints() < 4){
      vtkErrorMacro("Invalid region: zero-volume intersection");
      return 0;
    }
  }

  if (R->GetNumberOfPoints() < 8){
    vtkErrorMacro("invalid box");
    return 0;
  }

  int *where = new int[nplanes];

  int intersects = -1;

//  Here's the algorithm from Graphics Gems IV, page 81,
//            
//  R is an axis aligned box (could represent a region in a spatial
//    spatial partitioning of a volume of data).
//            
//  P is a set of planes defining a convex region in space (could be
//    a view frustum).
//            
//  The question is does P intersect R.  We expect to be doing the 
//    calculation for one P and many Rs.

//    You may wonder why we don't do what vtkClipPolyData does, which
//    computes the following on every point of it's PolyData input:
//
//      for each point in the input
//        for each plane defining the convex region
//          evaluate plane eq to determine if outside, inside or on plane
//
//     For each cell, if some points are inside and some outside, then
//     vtkClipPolyData decides it straddles the region and clips it.  If
//     every point is inside, it tosses it.
//
//     The reason is that the Graphics Gems algorithm is faster in some
//     cases (we may only need to evaluate one vertex of the box).  And
//     also because if the convex region passes through the box without
//     including any vertices of the box, all box vertices will be
//     "outside" and the algorithm will fail.  vtkClipPolyData assumes
//     cells are very small relative to the clip region.  In general
//     the axis-aligned box may be a large portion of world coordinate
//     space, and the convex region a view frustum representing a
//     small portion of the screen.


//  1.  If R does not intersect P's bounding box, return 0.

  if (this->IntersectsBoundingBox(R) == 0){
    intersects = 0;
  }

//  2.  If P's bounding box is entirely inside R, return 1.

  else if (this->EnclosesBoundingBox(R) == 1){
    intersects = 1;
  }

//  3.  For each face plane F of P
//
//      Suppose the plane equation is negative inside P and
//      positive outside P. Choose the vertex (n) of R which is
//      most in the direction of the negative pointing normal of
//      the plane.  The opposite vertex (p) is most in the
//      direction of the positive pointing normal.  (This is
//      a very quick calculation.)
//
//      If n is on the positive side of the plane, R is
//      completely outside of P, so return 0.
//
//      If n and p are both on the negative side, then R is on
//      the "inside" of F.  Keep track to see if all R is inside
//      all planes defining the region.

  else{

     if (this->Plane == NULL) this->SetPlaneEquations(); 

     allInside = 1;

     for (plane=0; plane < nplanes; plane++){

       where[plane] = this->EvaluateFacePlane(plane, R);

       if (allInside &&  (where[plane] != inside)){
         allInside = 0;
       }

       if (where[plane] == outside){

         intersects = 0;

         break;
       }
     }
  }

  if (intersects == -1){

//  4.  If n and p were "inside" all faces, R is inside P 
//      so return 1.

    if ( allInside){

      intersects = 1;
    }
//  5.  For each of three orthographic projections (X, Y and Z)
//
//      Compute the equations of the edge lines of P in those views.
//
//      If R's projection lies outside any of these lines (using 2D
//      version of n & p tests), return 0.

    else if ((this->IntersectsProjection(R, xdim) == 0) ||
             (this->IntersectsProjection(R, ydim) == 0) ||
             (this->IntersectsProjection(R, zdim) == 0)    ){

      intersects = 0;
    }
    else{

//    6.  Return 1.

      intersects = 1;
    }
  }

  delete [] where;

  return (intersects==1);
}

// The plane equations ***********************************************

void vtkPlanesIntersection::SetPlaneEquations()
{
  int i;
  int nplanes = this->GetNumberOfPlanes();

  // vtkPlanes stores normals & pts instead of
  //   plane equation coefficients

  if (this->Plane) delete [] this->Plane;

  this->Plane = new double[nplanes*4];

  for (i=0; i<nplanes; i++){
    int j = i*4;

    float n[3];
    float x[3];

    this->GetPlane(i)->GetNormal(n);
    this->GetPlane(i)->GetOrigin(x);

    this->Plane[j]   = (double)n[0];
    this->Plane[j+1] = (double)n[1];
    this->Plane[j+2] = (double)n[2];
    this->Plane[j+3] = (double)(-(n[0]*x[0] + n[1]*x[1] + n[2]*x[2]));
  }
}

// Compute region vertices if not set explicity ********************

void vtkPlanesIntersection::ComputeRegionVertices()
{
  double M[3][3];
  double rhs[3];
  double testv[3];
  int i, j, k;
  int nplanes = this->GetNumberOfPlanes();

  if (this->regionPts) this->regionPts->Delete();
  this->regionPts = vtkPointsProjectedHull::New();

  if (nplanes <= 3){
    vtkErrorMacro( << "vtkPlanesIntersection::ComputeRegionVertices invalid region");
    return;
  }

  if (this->Plane == NULL){
    this->SetPlaneEquations();
  }

  // This is an expensive process.  Better if vertices are
  // set in SetRegionVertices().  We're testing every triple of
  // planes to see if they intersect in a point that is
  // not "outside" any plane.

  int nvertices=0;

#ifdef TESTING_INTERSECTIONS
int ntested=0;
#endif

  for (i=0; i < nplanes; i++){
    for (j=i+1; j < nplanes; j++){
      for (k=j+1; k < nplanes; k++){

         this->planesMatrix(i, j, k, M);

         int notInvertible = this->Invert3x3(M);

         if (notInvertible) continue;

         this->planesRHS(i, j, k, rhs);

         vtkMath::Multiply3x3(M, rhs, testv);

         if (duplicate(testv)) continue;

         int outside = this->outsideRegion(testv);

         if (!outside){
           this->regionPts->InsertPoint(nvertices, testv);
           nvertices++;
         }

#ifdef TESTING_INTERSECTIONS
ntested++;
if (outside) cout <<"  " <<testv[0] <<" " <<testv[1] <<" " <<testv[2] <<" OUTSIDE" <<endl;
if (!outside) cout <<"  " <<testv[0] <<" " <<testv[1] <<" " <<testv[2] <<" KEEPER" <<endl;
#endif

      }
    }
  }
#ifdef TESTING_INTERSECTIONS
cout << nvertices << " vertices " << ntested << " tested==================\n";
#endif
}
const int vtkPlanesIntersection::duplicate(double testv[3])
{
  int i;
  double pt[3];
  int npts = this->regionPts->GetNumberOfPoints();

  for (i=0; i<npts; i++){
    this->regionPts->GetPoint(i, pt);

    if ( (pt[0] == testv[0]) && (pt[1] == testv[1]) && (pt[2] == testv[2])){

#ifdef TESTING_INTERSECTIONS
cout <<"  " <<testv[0] <<" " <<testv[1] <<" " <<testv[2] <<" DUPLICATE, TOSS OUT" <<endl;
#endif
      return 1;
    }

  }
  return 0;
}
const void vtkPlanesIntersection::planesMatrix(int p1, int p2, int p3, double M[3][3])
{
  int i;
  for (i=0; i<3; i++){
     M[0][i] = this->Plane[p1*4 + i];
     M[1][i] = this->Plane[p2*4 + i];
     M[2][i] = this->Plane[p3*4 + i];
  }
}
const void vtkPlanesIntersection::planesRHS(int p1, int p2, int p3, double r[3])
{
  r[0] = -(this->Plane[p1*4 + 3]);
  r[1] = -(this->Plane[p2*4 + 3]);
  r[2] = -(this->Plane[p3*4 + 3]);
}
const int vtkPlanesIntersection::outsideRegion(double testv[3])
{
  int i;
  int outside = 0;
  int nplanes = this->GetNumberOfPlanes();

  for (i=0; i<nplanes; i++){
    int row=i*4;

    double fx = this->Plane[row] * testv[0] +
                this->Plane[row+1] * testv[1] +
                this->Plane[row+2] * testv[2] +
                this->Plane[row+3];

    if (fx > SMALL_DOUBLE){
      outside = 1;
      break;
    }
  }
  return outside;
}
int vtkPlanesIntersection::Invert3x3(double M[3][3])
{
  int i, j;
  double temp[3][3];

  double det = vtkMath::Determinant3x3(M);

  if ( (det > -SMALL_DOUBLE) && (det < SMALL_DOUBLE)) return -1;

  vtkMath::Invert3x3(M, temp);

  for (i=0; i<3; i++){
    for (j=0; j<3; j++){
       M[i][j] = temp[i][j];
    }
  }

  return 0;
}

// Region / box intersection tests *******************************

int vtkPlanesIntersection::IntersectsBoundingBox(vtkPoints *R)
{
float BoxBounds[6], RegionBounds[6];
  
  R->GetBounds(BoxBounds);

  this->regionPts->GetBounds(RegionBounds);

  if ((BoxBounds[1] <= RegionBounds[0]) ||
      (BoxBounds[0] >= RegionBounds[1]) ||
      (BoxBounds[3] <= RegionBounds[2]) ||
      (BoxBounds[2] >= RegionBounds[3]) ||
      (BoxBounds[5] <= RegionBounds[4]) ||
      (BoxBounds[4] >= RegionBounds[5])) {


#ifdef VERBOSE
  cout << "Fails to intersect region Bounding Box" << endl;
#endif

    return 0;
  }
  return 1;
}
int vtkPlanesIntersection::EnclosesBoundingBox(vtkPoints *R)
{
float BoxBounds[6], RegionBounds[6];

  R->GetBounds(BoxBounds);

  this->regionPts->GetBounds(RegionBounds);

  if ((BoxBounds[0] > RegionBounds[0]) ||
      (BoxBounds[1] < RegionBounds[1]) ||
      (BoxBounds[2] > RegionBounds[2]) ||
      (BoxBounds[3] < RegionBounds[3]) ||
      (BoxBounds[4] > RegionBounds[4]) ||
      (BoxBounds[5] < RegionBounds[5])) {

    return 0;
  }

#ifdef VERBOSE
  cout << "Encloses region Bounding Box" << endl;
#endif

  return 1;
}
int vtkPlanesIntersection::EvaluateFacePlane(int plane, vtkPoints *R)
{
  int i;
  float n[3], bounds[6]; 
  double withN[3], oppositeN[3];

  R->GetBounds(bounds);

  this->GetPlane(plane)->GetNormal(n);

  // Find vertex of R most in direction of normal, and find
  //  oppposite vertex

  for (i=0; i<3; i++){

    if (n[i] < 0){
      withN[i]     = (double)bounds[i*2];
      oppositeN[i] = (double)bounds[i*2 + 1];
    } 
    else{
      withN[i]     = (double)bounds[i*2 + 1];
      oppositeN[i] = (double)bounds[i*2];
    }
  }
  
  // Determine whether R is in negative half plane ("inside" frustum),
  //    positive half plane, or whether it straddles the plane.
  //    The normal points in direction of positive half plane.

  double negVal = this->Plane[plane*4]     * oppositeN[0] +
                 this->Plane[plane*4 + 1] * oppositeN[1] + 
                 this->Plane[plane*4 + 2] * oppositeN[2] +
                 this->Plane[plane*4 + 3];

  if (negVal >= 0){

#ifdef VERBOSE
  cout << "Outside face plane " << plane << endl;
#endif
    return outside;
  }

  double posVal = this->Plane[plane*4]     * withN[0] +
                 this->Plane[plane*4 + 1] * withN[1] + 
                 this->Plane[plane*4 + 2] * withN[2] +
                 this->Plane[plane*4 + 3];

  if (posVal <= 0){
    return inside;
  }

  else             return straddle;
}
int vtkPlanesIntersection::IntersectsProjection(vtkPoints *R, int dir)
{
int intersects;

  switch (dir){

    case xdim:

      intersects = this->regionPts->rectangleIntersectionX(R);
      break;

    case ydim:

      intersects = this->regionPts->rectangleIntersectionY(R);
      break;

    case zdim:

      intersects = this->regionPts->rectangleIntersectionZ(R);
      break;
  }

#ifdef VERBOSE
  if (!intersects){
     cout << "Fails to intersect project " << dir << endl;
  }
#endif

  return intersects;
} 

void vtkPlanesIntersection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "regionPts: " << this->regionPts << endl;
  os << indent << "Plane: " << this->Plane << endl;

}
