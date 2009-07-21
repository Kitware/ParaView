/*=========================================================================

 Program:   Visualization Toolkit
 Module:    Quadrics_fs.glsl

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

// .NAME Quadrics_fs.glsl
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
//   - vertex position
//   - point size
//   - ray origin
//   - perspective flag
//   - quadric equation coefficients
//   - color
//   - viewport (width and height only)
//   - min point size (pointThreshold)
//
// OUT:
//   - fragment color computed from point intersected by ray shot from
//     viewpoint through point computed from current fragment coordinate
//   - fragment depth computed by projecting the intersection point into screen
//     coordinates


// OPTIMAL
#define ELLIPSOID
//#define CYLINDER
//#define CONE
//#define HYPERBOLOID1
//#define HYPERBOLOID2
//#define PARABOLOID

// SUB OPTIMAL
//#define HYPER_PARABOLOID

uniform vec2 viewport; // only width and height passed, no origin
uniform float pointSizeThreshold; // minimum point size

varying vec4 color;
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
varying float pointSize;
varying float perspective;

vec3 raydir; // ray direction in screen space
vec3 rayorigin; // ray origin in screen space

#ifndef ELLIPSOID
varying mat4 Ti;
#endif

const float FLAT_SHADE_POINT_SIZE = 1.0; //if point size < 1 use flat shading

const float FEPS = 0.0001;

const float BOUND = 1.0 + FEPS;

const vec3 MIN_BOUND = vec3(-BOUND);

const vec3 MAX_BOUND = vec3(BOUND);

//------------------------------------------------------------------------------
// BOUNDS CHECK
// in general it makes sense to check only along the z direction for:
// - paraboloids
// - hyperboloids of one sheet
// - cylinders
// - cones
// and no checking at all is required for ellipsoids
#ifndef ELLIPSOID
bool InBounds( vec3 P )
	{
	vec4 v = Ti * gl_ModelViewMatrixInverse * vec4( P, 1. );
#if defined( CYLINDER ) || defined( CONE ) || defined( HYPERBOLOID1 ) || defined( PARABOLOID )
	return v.z >= -BOUND && v.z <= BOUND;
#else
	return all( greaterThanEqual( v.xyz, MIN_BOUND ) ) &&
	all( lessThanEqual( v.xyz, MAX_BOUND ) );
#endif
	}
#endif
//------------------------------------------------------------------------------
// INTERSECTION
struct I
{
	vec3 P;
	vec3 N;
	float t;
};

// compute unit normal from gradient
vec3 ComputeNormal(vec3 P)
{
	return normalize(vec3(dot(vec4(a, d, e, 1.), vec4(P, g)), // should multiply by 2 for actual gradient
			dot(vec4(d, b, f, 1.), vec4(P, h)), // should multiply by 2 for actual gradient
			dot(vec4(e, f, c, 1.), vec4(P, i)) // should multiply by 2 for actual gradient
			));
}

// compute ray quadric intersection; if no intersection occurs I.t is < 0
// main axis length and orientation are used to clip the quadric; not
// required for closed quadrics (ellipsoids)
// | a d e g |
// | d b f h |
// | e f c i |
// | g h i j |
// ax^2 + by^2 + cz^2 + 2dxy +2exz + 2fyz + 2gx + 2hy + 2iz + j = 0
/// @todo pass vec3(a, b, c), vec3( d, e, f ) and vec3( g, h, i ) instead of single coefficients
I ComputeRayQuadricIntersection()
{
	I ip;
	ip.t = -1.0;
	vec3 P = rayorigin;
	vec3 D = raydir;
	float A = 0.0;
	float B = 0.0;
	float C = 0.0;
	if (bool(perspective))
		{
		A = dot(vec3(a, b, c), D * D) + 2. * dot(vec3(d, e, f), D.xxy * D.yzz);
		B = 2. * dot(vec3(g, h, i), D);
		C = j;
		}
	else
		{
		A = c;
		//B = -2. * dot(  vec4( c, e, f, 1. ), vec4( P.zxy, 1. ) );
		B = -2. * dot(vec4(d, e, f, i), vec4(P.zxy, 1.));
		C = dot(vec3(a, b, c), P * P) + 2. * (dot(vec3(d, e, f), P.xxy * P.yzz)
				+ dot(vec3(g, h, i), P)) + j;
		}
	float delta = B * B - 4. * A * C;
	if (delta < 0.0)
		return ip;
	float d = sqrt(delta);
	A = 1. / A;
	A *= 0.5;
	float t2 = A * (-B + d);
	float t1 = A * (-B - d);
#ifdef ELLIPSOID
	ip.P = rayorigin + D * min(t1, t2);
	ip.N = ComputeNormal(ip.P);
	ip.t = 0.;
#else
	vec3 P1 = rayorigin + D * min( t1, t2 );
	vec3 P2 = rayorigin + D * max( t1, t2 );
	if( InBounds( P1 ) )
		{
		ip.P = P1;
		ip.N = ComputeNormal( P1 );
		ip.t = 0.;
		}
	else if( InBounds( P2 ) )
		{
		ip.P = P2;
		ip.N = ComputeNormal( P2 );
		ip.t = 0.;
		}
#endif
	return ip;
}

//------------------------------------------------------------------------------
// LIGHTING, standard phong lighting model
vec3 lightDir = normalize(vec3(0.1, 0.1, 1.));
float kd = 1.0;
float ka = 0.01;
float ks = .5;
float sh = 90.0;
vec4 refcolor = vec4(1., 1., 1., 1.);
vec4 ComputeColor(vec4 color, vec3 n, vec3 P)
{
	if (pointSize < FLAT_SHADE_POINT_SIZE)
		return color;

	vec3 col = (0, 0, 0);
	vec3 N;
	float d;
	vec3 viewdir;
	float vl;
	float s;

	for (int li = 0; li < 4; li++)
		{
		lightDir = normalize(gl_LightSource[li].position);
		N = faceforward(-n, lightDir, n);
		d = dot(N, lightDir);
		viewdir = normalize(-P);
		vl = max(0., dot(reflect(-lightDir, N), viewdir));
		s = pow(vl, gl_FrontMaterial.shininess);
		col += gl_FrontMaterial.specular * s * gl_LightSource[li].specular.rgb + kd
				* d * color.rgb * gl_LightSource[li].diffuse.rgb + ka * color.rgb
				* gl_LightSource[li].ambient.rgb;
		}

	return vec4(col, color.a);

}

//------------------------------------------------------------------------------
// MAIN
void propFuncFS(void)
{
	if (pointSize < pointSizeThreshold || color.a <= 0.0)
		discard;
	vec3 fc = gl_FragCoord.xyz;
	fc.xy /= viewport;
	fc *= 2.0;
	fc -= 1.0;
	vec4 p = gl_ProjectionMatrixInverse * vec4(fc, 1.);
	if (bool(perspective))
		{
		// in perspective mode, rayorigin is always at (0, 0, 0)
		rayorigin = vec3(0., 0., 0.);
		raydir = vec3(p) / p.w;
		}
	else
		{
		// in orthographic mode, raydir is always ( 0., 0., -1. );
		raydir = vec3(0., 0., -1.);
		rayorigin = vec3(p.x / p.w, p.y / p.w, 0.);
		}
	// compute intersection
	I i = ComputeRayQuadricIntersection();
	if (i.t < 0.0)
		discard;
	// compute color
	gl_FragColor = ComputeColor(color, i.N, i.P);
	// update depth by projecting point and updating depth coordinate
	// the transposed version of the projection matrix is used to
	// perform vector, matrix row product in one line:
	// M[2][*] x V = Vt x Mt[*][2] where:
	//   % V  is a column vector
	//   % Vt is a row vector
	//   % M is a square matrix
	//   % Mt is the transpose of M
	float z = dot(vec4(i.P, 1.), gl_ProjectionMatrixTranspose[2]);
	float w = dot(vec4(i.P, 1.), gl_ProjectionMatrixTranspose[3]);
	gl_FragDepth = 0.5 * (z / w + 1.0);
}
