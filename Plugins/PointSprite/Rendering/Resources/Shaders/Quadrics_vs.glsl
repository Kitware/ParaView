/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Quadrics_vs.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Quadrics_vs.glsl
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

//
// IN:
//   - vertex
//   - quadric transformation matrix
//   - viewport (width and height only)
//   - point scaling factor
//   - min point size (pointThreshold)
//
// OUT:
//   - vertex position
//   - point size
//   - ray origin
//   - perspective flag
//   - quadric equation coefficients
//   - color
//
// NOTE: this shader is currently used for ellipsoids but can be used with
//       any quadric matrix; for quadrics other than ellipsoids additional
//       clipping information is required for computing both point size
//       and intersection point
/// @todo try to pass attributes through texture coordinates/normal/secondary color...

//#define CORRECT_POINT_Z

// OPTIMAL
#define SPHERE
//#define ELLIPSOID
//#define CYLINDER
//#define CONE
//#define HYPERBOLOID1
//#define HYPERBOLOID2
//#define PARABOLOID

// SUB OPTIMAL
//#define HYPER_PARABOLOID

// force ELLIPSOID if we are using SPHERE
#ifdef SPHERE
#ifndef ELLIPSOID
#define ELLIPSOID
#endif
#endif

uniform vec2 viewport; // only width and height passed, no origin
uniform float pointSizeThreshold; // minimum point size
uniform float MaxPixelSize;

// quadric coefficients
// | a d e g |
// | d b f h |
// | e f c i |
// | g h i j |
// ax^2 + by^2 + cz^2 + 2dxy +2exz + 2fyz + 2gx + 2hy + 2iz + j = 0
varying float a;
varying float b;
varying float c;
varying float d;
varying float e;
varying float f;
varying float g;
varying float h;
varying float i;
varying float j;

varying vec4 color; // primitive color
varying float pointSize; // computed point size
varying float perspective; // perspective flag

#ifndef SPHERE
// columns of inverse transform
attribute vec4 Ti1;
attribute vec4 Ti2;
attribute vec4 Ti3;
attribute vec4 Ti4;

// columns of transform
attribute vec4 T1;
attribute vec4 T2;
attribute vec4 T3;
attribute vec4 T4;
#endif

// bounds in clip coordinates
vec2 xbc;
vec2 ybc;

// Matrices in canonical form + trasformation matrices applied to
// bounding ellipsoids used to compute the point size: bounds are
// computed by transforming a 2D ellipsoid in canonical form with
// the t1,2 matrices and computing the union of the bounding boxes
// of the transformed ellipsoids.
#if defined( ELLIPSOID )
// quadric matrix in canonical form for ellipsoids
const mat4 D  = mat4( 1., 0., 0., 0.,
                      0., 1., 0., 0.,
                      0., 0., 1., 0.,
                      0., 0., 0., -1. );
#elif defined( CYLINDER )
// quadric matrix in canonical form for cylinders
const mat4 D  = mat4( 1.,  0.,  0.,  0.,
                      0.,  1.,  0.,  0.,
                      0.,  0.,  0.,  0.,
                      0.,  0.,  0., -1. );
const mat4 t1 = mat4( 1., 0., 0., 0.,
                      0., 1., 0., 0.,
                      0., 0., 0., 0,
                      0., 0., 1., 1. );
const mat4 t2 = mat4( 1., 0.,  0., 0.,
                      0., 1.,  0., 0.,
                      0., 0.,  0., 0.,
                      0., 0., -1., 1. );
#elif defined( CONE )
// quadric matrix in canonical form for cones
const mat4 D  = mat4( 1.,  0.,  0.,  0.,
                      0.,  1.,  0.,  0.,
                      0.,  0., -1.,  0.,
                      0.,  0.,  0.,  0. );
const mat4 t1 = mat4( 1., 0., 0., 0.,
                      0., 1., 0., 0.,
                      0., 0., 0., 0.,
                      0., 0., 1., 1. );
const mat4 t2 = mat4( 1., 0., 0., 0.,
                      0., 1., 0., 0.,
                      0., 0., 0., 0.,
                      0., 0., -1., 1. );
#elif defined( HYPERBOLOID1 )
// quadric matrix in canonical form for hyperboloids of one sheet
const mat4 D  = mat4( 1.,  0.,  0.,  0.,
                      0.,  1.,  0.,  0.,
                      0.,  0., -1.,  0.,
                      0.,  0.,  0., -1. );
const mat4 t1 = mat4( sqrt( 2 ), 0., 0., 0.,
                      0., sqrt( 2 ), 0., 0.,
                      0., 0., 0.00, 0,
                      0., 0., 1., 1. );
const mat4 t2 = mat4( sqrt( 2 ), 0., 0., 0.,
                      0., sqrt( 2 ), 0., 0.,
                      0., 0., 0.00, 0.,
                      0., 0., -1., 1. );
#elif defined( HYPERBOLOID2 )
// quadric matrix in canonical form for hyperboloids of two sheets
// note the use of 0.5 instead of 1 to have the quadric visible
// within the unit cube with a radius of 1/sqrt(2) at z = +/-1
const mat4 D  = mat4( -1.,  0.,  0.,  0.,
                       0., -1.,  0.,  0.,
                       0.,  0.,  1.,  0.,
                       0.,  0.,  0.,  -.5 );
const mat4 t1 = mat4( inversesqrt( 2. ), 0., 0., 0.,
                      0., inversesqrt( 2. ), 0., 0.,
                      0., 0., 0., 0.,
                      0., 0., 1., 1. );
const mat4 t2 = mat4( inversesqrt( 2. ), 0., 0., 0.,
                      0., inversesqrt( 2. ), 0., 0.,
                      0., 0.,  0., 0.,
                      0., 0., -1., 1. );
#elif defined( PARABOLOID )
// quadric matrix in canonical form for paraboloids
// note the .5 components used to properly center
// the paraboloid
const mat4 D  = mat4( 1.,  0.,  0.,  0.,
                      0.,  1.,  0.,  0.,
                      0.,  0.,  0.,  -.5,
                      0.,  0.,  -.5,  0. );
const mat4 t1 = mat4( 1., 0., 0., 0.,
                      0., 1., 0., 0.,
                      0., 0., 0., 0.,
                      0., 0., 1., 1. );
const mat4 t2 = mat4( 1., 0., 0., 0.,
                      0., 1., 0., 0.,
                      0., 0., 0., 0.,
                      0., 0., 0., 1. );

#elif defined( HYPER_PARABOLOID )
// quadric matrix in canonical form for hyperbolic paraboloid
const mat4 D  = mat4( 1.,  0.,  0.,  0.,
                      0., -1.,  0.,  0.,
                      0.,  0.,  0.,  -.5,
                      0.,  0.,  -.5,  0. );
#endif

// change of basis matrix and inverse for quadric
mat4 T;
varying mat4 Ti;

const float FEPS = 0.000001;

const float DEF_Z = 1. - FEPS;

//------------------------------------------------------------------------------
/// Compute point size and center using the technique described in:
/// "GPU-Based Ray-Casting of Quadratic Surfaces"
/// by Christian Sigg, Tim Weyrich, Mario Botsch, Markus Gross.
void ComputePointSizeAndPositionInClipCoordEllipsoid()
{
  mat4 R = transpose( gl_ModelViewProjectionMatrix * T );
  float A = dot( R[ 3 ], D * R[ 3 ] );
  float B = -2. * dot( R[ 0 ], D * R[ 3 ] );
  float C = dot( R[ 0 ], D * R[ 0 ] );
  xbc[ 0 ] = ( -B - sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  xbc[ 1 ] = ( -B + sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  float sx = abs( xbc[ 0 ] - xbc[ 1 ] ) * .5 * viewport.x;

  A = dot( R[ 3 ], D * R[ 3 ] );
  B = -2. * dot( R[ 1 ], D * R[ 3 ] );
  C = dot( R[ 1 ], D * R[ 1 ] );
  ybc[ 0 ] = ( -B - sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  ybc[ 1 ] = ( -B + sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  float sy = abs( ybc[ 0 ] - ybc[ 1 ]  ) * .5 * viewport.y;

  pointSize = ceil( max( sx, sy ) );
  gl_PointSize = pointSize;
#ifdef CORRECT_POINT_Z
  // gl_Position has to be precomputed before getting here
  // the reason for which we want the z coordinate to be correct is for debugging
  // purpose only: when displaying point shapes as quads the point center will match
  // the the quadric center
  gl_Position.xy = vec2( .5 * ( xbc.x + xbc.y ), .5 * ( ybc.x + ybc.y ) ) * gl_Position.w;
#else
  gl_Position = vec4( .5 * ( xbc.x + xbc.y ), .5 * ( ybc.x + ybc.y ), DEF_Z, 1. );
#endif
}

//------------------------------------------------------------------------------
/// Compute point size and center using the technique described in:
/// "GPU-Based Ray-Casting of Quadratic Surfaces"
/// by Christian Sigg, Tim Weyrich, Mario Botsch, Markus Gross.
/// The technique described in the paper only works with ellipsoids, the following code
/// extends the technique to make it work with other quadric tyepes:
/// - Cylinder: the cylinder bounding box is computed by computing the union
///   of the bounding boxes of two 2D ellipsoids centered at (0, 0, +/- 1) which are
///   the cylinder bounds along the z axis in parameter space.
/// - Hyperboloid of one sheet: the hyperboloid bounding box is computed by
///   computing the union of the bounding boxes of two 2D ellipsoids
///   centered at (0, 0, +/- 1) and whose x and y axes are scaled by sqrt(2)
///   which are the hyperboloid bounds in parameter space.
/// - Hyperboloid of two sheets: the hyperboloid bounding box is computed by
///   computing the union of the bounding boxes of two 2D ellipsoids
///   centered at (0, 0, +/- 2) and whose x and y axes are scaled by sqrt(2),
///   which are the hyperboloid bounds in parameter space.
/// - Cone: the cone bounding box is computed by computing the union of the bounding
///   boxes of two 2D ellipsoids centered at (0, 0, +/- 1) which are the cone
///   bounds along the z axis in parameter space.
/// - Paraboloid: the paraboloid bounding box is computed by computing the union
///   of the bounding boxes of two 2D ellipsoids centered at (0, 0, 0) and (0, 0, 1)
///   which are the paraboloid bounds along the z axis in parameter space.
#if defined( CYLINDER ) || defined( CONE ) || defined( HYPERBOLOID1 ) || defined( HYPERBOLOID2 )  || defined( PARABOLOID )
void ComputePointSizeAndPositionInClipCoord()
{

  // always use ellipse to compute bounds
  const mat4 D  = mat4( 1., 0., 0., 0.,
                        0., 1., 0., 0.,
                        0., 0., 0., 0.,
                        0., 0., 0., -1. );

  mat4 R = transpose( gl_ModelViewProjectionMatrix * T * t1 );
  float A = dot( R[ 3 ], D * R[ 3 ] );
  float B = -2. * dot( R[ 0 ], D * R[ 3 ] );
  float C = dot( R[ 0 ], D * R[ 0 ] );
  vec2 xbc1;
  xbc1[ 0 ] = ( -B - sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  xbc1[ 1 ] = ( -B + sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );

  A = dot( R[ 3 ], D * R[ 3 ] );
  B = -2. * dot( R[ 1 ], D * R[ 3 ] );
  C = dot( R[ 1 ], D * R[ 1 ] );
  vec2 ybc1;
  ybc1[ 0 ] = ( -B - sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  ybc1[ 1 ] = ( -B + sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );

  R = transpose( gl_ModelViewProjectionMatrix * T * t2 );
  A = dot( R[ 3 ], D * R[ 3 ] );
  B = -2. * dot( R[ 0 ], D * R[ 3 ] );
  C = dot( R[ 0 ], D * R[ 0 ] );
  vec2 xbc2;
  xbc2[ 0 ] = ( -B - sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A ) ;
  xbc2[ 1 ] = ( -B + sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );

  A = dot( R[ 3 ], D * R[ 3 ] );
  B = -2. * dot( R[ 1 ], D * R[ 3 ] );
  C = dot( R[ 1 ], D * R[ 1 ] );
  vec2 ybc2;
  ybc2[ 0 ] = ( -B - sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );
  ybc2[ 1 ] = ( -B + sqrt( B * B - 4. * A * C ) ) / ( 2.0 * A );


  xbc[ 0 ] = min( xbc1[ 0 ], min( xbc1[ 1 ], min( xbc2[ 0 ], xbc2[ 1 ] ) ) );
  xbc[ 1 ] = max( xbc1[ 0 ], max( xbc1[ 1 ], max( xbc2[ 0 ], xbc2[ 1 ] ) ) );

  ybc[ 0 ] = min( ybc1[ 0 ], min( ybc1[ 1 ], min( ybc2[ 0 ], ybc2[ 1 ] ) ) );
  ybc[ 1 ] = max( ybc1[ 0 ], max( ybc1[ 1 ], max( ybc2[ 0 ], ybc2[ 1 ] ) ) );

  float sx = ( xbc[ 1 ] - xbc[ 0 ] ) * .5 * viewport.x;
  float sy = ( ybc[ 1 ] - ybc[ 0 ] ) * .5 * viewport.y;

  pointSize =  ceil( max( sx, sy ) );
  gl_PointSize = pointSize;

#ifdef CORRECT_POINT_Z
  // gl_Position has to be precomputed before getting here
  // the reason for which we want the z coordinate to be correct is for debugging
  // purpose only: when displaying point shapes as quads the point center will match
  // the the quadric center
  gl_Position.xy = vec2( .5 * ( xbc.x + xbc.y ), .5 * ( ybc.x + ybc.y ) ) * gl_Position.w;
#else
  gl_Position = vec4( .5 * ( xbc.x + xbc.y ), .5 * ( ybc.x + ybc.y ), DEF_Z, 1. );
#endif
}
#endif

//------------------------------------------------------------------------------
// Generic bounding box computation, works with any quadric type by splatting
// in clip space the bounding box in parameter space;
// in most cases you'll have to use a point scaling factor from 1.05 to 1.5
void ComputePointSizeAndPositionWithProjection()
{

  mat4 M = gl_ModelViewProjectionMatrix * T;

  const float dxm = -1.;
  const float dxp =  1.;
  const float dym = -1.;
  const float dyp =  1.;
  const float dzm = -1.;
  const float dzp =  1.;
  vec4 P1 = M * vec4( dxm, dym, dzm, 1. );
  vec4 P2 = M * vec4( dxp, dym, dzm, 1. );
  vec4 P3 = M * vec4( dxp, dyp, dzm, 1. );
  vec4 P4 = M * vec4( dxm, dyp, dzm, 1. );
  vec4 P5 = M * vec4( dxm, dym, dzp, 1. );
  vec4 P6 = M * vec4( dxp, dym, dzp, 1. );
  vec4 P7 = M * vec4( dxp, dyp, dzp, 1. );
  vec4 P8 = M * vec4( dxm, dyp, dzp, 1. );

  P1 /= P1.w;
  P2 /= P2.w;
  P3 /= P3.w;
  P4 /= P4.w;
  P5 /= P5.w;
  P6 /= P6.w;
  P7 /= P7.w;
  P8 /= P8.w;

  float xmin = min( P1.x,
                 min( P2.x,
                   min( P3.x,
                     min( P4.x,
                       min( P5.x,
                         min( P6.x,
                           min( P7.x, P8.x ) ) ) ) ) ) );
  float ymin = min( P1.y,
                 min( P2.y,
                   min( P3.y,
                     min( P4.y,
                       min( P5.y,
                         min( P6.y,
                          min( P7.y, P8.y ) ) ) ) ) ) );

  float xmax = max( P1.x,
                 max( P2.x,
                   max( P3.x,
                     max( P4.x,
                       max( P5.x,
                         max( P6.x,
                           max( P7.x, P8.x ) ) ) ) ) ) );

  float ymax = max( P1.y,
                 max( P2.y,
                   max( P3.y,
                     max( P4.y,
                       max( P5.y,
                         max( P6.y,
                           max( P7.y, P8.y ) ) ) ) ) ) );


  float sx = ( xmax - xmin ) * 0.5 * viewport.x;
  float sy = ( ymax - ymin ) * 0.5 * viewport.y;

//  gl_PointSize =  ceil( pointScaling * max( sx, sy ) );
  pointSize =  ceil( max( sx, sy ) );
  gl_PointSize = pointSize;
#ifdef CORRECT_POINT_Z
  // gl_Position has to be precomputed before getting here
  // the reason for which we want the z coordinate to be correct is for debugging
  // purpose only: when displaying point shapes as quads the point center will match
  // the the quadric center
  gl_Position.xy = vec2( .5 * ( xmin + xmax ), .5 * ( ymin + ymax ) ) * gl_Position.w;
#else
  gl_Position = vec4( .5 * ( xmin + xmax ), .5 * ( ymin + ymax ), DEF_Z, 1. );
#endif
}

#ifdef SPHERE
float GetRadius();
#endif

void  ComputePointSizeAndPosition()
{
#if defined( ELLIPSOID )
  ComputePointSizeAndPositionWithProjection();
  //ComputePointSizeAndPositionInClipCoordEllipsoid();
#elif defined( CYLINDER ) || defined( CONE ) || defined( HYPERBOLOID1 ) || defined( HYPERBOLOID2 )  || defined( PARABOLOID )
  ComputePointSizeAndPositionInClipCoord();
#else
  ComputePointSizeAndPositionWithProjection();
#endif
}

//------------------------------------------------------------------------------
// MAIN
void propFuncVS()
{  
  color = gl_Color;

  // compute position, this is required only when displaying the point quad
#ifdef CORRECT_POINT_Z
  gl_Position = ftransform();
#endif

  //set perspective flag by inspecting the projection matrix
  perspective = float( gl_ProjectionMatrix[ 3 ][ 3 ] < FEPS && abs( gl_ProjectionMatrix[ 2 ][ 3 ] ) > FEPS );

  #ifdef SPHERE
  float radius = GetRadius();
  float iradius;
  if(radius < FEPS)
    iradius = 1.0/FEPS;
  else
    iradius = 1.0/radius;

  T =  mat4(  radius, 0., 0., 0.,
              0., radius, 0., 0.,
              0., 0., radius, 0.,
              gl_Vertex.x, gl_Vertex.y, gl_Vertex.z, 1.0 );

  Ti =  mat4( iradius, 0., 0., 0.,
              0., iradius, 0., 0.,
              0., 0., iradius, 0.,
              -gl_Vertex.x*iradius, -gl_Vertex.y*iradius, -gl_Vertex.z*iradius, 1.0 );


  #else
  // inverse of transformation matrix
  Ti = mat4( Ti1, Ti2, Ti3, Ti4 );
  // transformation matrix
  T  = mat4( T1, T2, T3, T4 );
  #endif

  // compute point size and gl_Position; uses Ti and T which have to be
  // computed before calling the function
  ComputePointSizeAndPosition();

  if(pointSize > MaxPixelSize)
    {
    gl_PointSize = MaxPixelSize;
    float factor = gl_PointSize / pointSize;
    float realRadius = radius*factor;
    float realIRadius = 1.0/realRadius;
    T =  mat4(  realRadius, 0., 0., 0.,
                0., realRadius, 0., 0.,
                0., 0., realRadius, 0.,
                gl_Vertex.x, gl_Vertex.y, gl_Vertex.z, 1.0 );

    Ti =  mat4( realIRadius, 0., 0., 0.,
                0., realIRadius, 0., 0.,
                0., 0., realIRadius, 0.,
                -gl_Vertex.x*realIRadius, -gl_Vertex.y*realIRadius, -gl_Vertex.z*realIRadius, 1.0 );
    
    ComputePointSizeAndPosition();
    }
  // if pixel size valid set quadric's coefficients
  if( pointSize > pointSizeThreshold )
  {
         // transposed inverse of transformation matrix
    mat4 Tit = transpose( Ti );
    // transform quadric matrix into world coordinates and
    // assign values to coefficients to be passed to fragment shader
    mat4 Q = gl_ModelViewMatrixInverseTranspose * Tit * D * Ti * gl_ModelViewMatrixInverse;
         //////////////////
    // | a d e g |
    // | d b f h |
    // | e f c i |
    // | g h i j |
    // ax^2 + by^2 + cz^2 + 2dxy +2exz + 2fyz + 2gx + 2hy + 2iz + j = 0
    a = Q[ 0 ][ 0 ];
    b = Q[ 1 ][ 1 ];
    c = Q[ 2 ][ 2 ];
    d = Q[ 1 ][ 0 ];
    e = Q[ 2 ][ 0 ];
    f = Q[ 2 ][ 1 ];
    g = Q[ 3 ][ 0 ];
    h = Q[ 3 ][ 1 ];
    i = Q[ 3 ][ 2 ];
    j = Q[ 3 ][ 3 ];
  }
}
