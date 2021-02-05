#include "projections.h"
#include "helpers.h"
#include "vtkMath.h"
#include <math.h>

int CartesianToSpherical(double x, double y, double z, double* rho, double* phi, double* theta)
{
  double trho = sqrt((x * x) + (y * y) + (z * z));
  double ttheta = atan2(y, x);
  double tphi = acos(z / (trho));
  if (vtkMath::IsNan(trho) || vtkMath::IsNan(ttheta) || vtkMath::IsNan(tphi))
  {
    return -1;
  }

  *rho = trho;
  *theta = ttheta;
  *phi = tphi;

  return 0;
}

//----------------------------------------------------------------------------
//  Function to convert spherical coordinates to cartesian, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------
int SphericalToCartesian(double rho, double phi, double theta, double* x, double* y, double* z)
{
  double tx = rho * sin(phi) * cos(theta);
  double ty = rho * sin(phi) * sin(theta);
  double tz = rho * cos(phi);
  if (vtkMath::IsNan(tx) || vtkMath::IsNan(ty) || vtkMath::IsNan(tz))
  {
    return -1;
  }

  *x = tx;
  *y = ty;
  *z = tz;

  return 0;
}

//----------------------------------------------------------------------------
//  Compute mollweide projection theta
//  https://en.wikipedia.org/wiki/Mollweide_projection
//----------------------------------------------------------------------------

inline double iterate_theta(const double lon, const double lat, const double theta)
{
  const double pi = vtkMath::Pi();
  return theta - (2 * theta + sin(2 * theta) - pi * sin(lat)) / (2 + 2 * cos(2 * theta));
}

inline double get_theta(const double lon, const double lat)
{
  double theta = lat;
  const double pi = vtkMath::Pi();
  if (lat == pi / 2.)
    return pi / 2;
  else if (lat == -pi / 2.)
    return -pi / 2;

  for (int i = 0; i < 5; i++)
    theta = iterate_theta(lon, lat, theta);
  return theta;
}

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#ifndef RSQRT2
#define RSQRT2 0.7071067811865475244008443620
#endif

static double ell_int_5(double phi)
{
  /* Procedure to compute elliptic integral of the first kind
   * where k^2=0.5.  Precision good to better than 1e-7
   * The approximation is performed with an even Chebyshev
   * series, thus the coefficients below are the even values
   * and where series evaluation  must be multiplied by the argument. */
  constexpr double C0 = 2.19174570831038;
  static const double C[] = {
    -8.58691003636495e-07,
    2.02692115653689e-07,
    3.12960480765314e-05,
    5.30394739921063e-05,
    -0.0012804644680613,
    -0.00575574836830288,
    0.0914203033408211,
  };

  double y = phi * M_2_PI;
  y = 2. * y * y - 1.;
  double y2 = 2. * y;
  double d1 = 0.0;
  double d2 = 0.0;
  for (double c : C)
  {
    double temp = d1;
    d1 = y2 * d1 - d2 + c;
    d2 = temp;
  }

  return phi * (y * d1 - d2 + 0.5 * C0);
}

double aasin(double v)
{
  double av;

  if ((av = fabs(v)) >= 1.)
  {
    return (v < 0. ? -M_PI_2 : M_PI_2);
  }
  return asin(v);
}

double aacos(double v)
{
  double av;

  if ((av = fabs(v)) >= 1.)
  {
    return (v < 0. ? M_PI : 0.);
  }
  return acos(v);
}

void crossProduct(double vect_A[], double vect_B[], double cross_P[])
{
  cross_P[0] = vect_A[1] * vect_B[2] - vect_A[2] * vect_B[1];
  cross_P[1] = vect_A[2] * vect_B[0] - vect_A[0] * vect_B[2];
  cross_P[2] = vect_A[0] * vect_B[1] - vect_A[1] * vect_B[0];
}

double dotProduct(double vect_A[], double vect_B[])
{
  double product = 0;
  for (int i = 0; i < 3; i++)
    product = product + vect_A[i] * vect_B[i];

  return product;
}

void rotateVec(double v[], double r[])
{
  double theta = 1.0472;
  double cos_theta = cos(theta);
  double sin_theta = sin(theta);

  double k[3], p[3];

  k[0] = cos(3.57792);
  k[1] = sin(3.57792);
  k[2] = 0.0;

  crossProduct(k, v, p);
  double dot = dotProduct(k, v);

  r[0] = (v[0] * cos_theta) + (p[0] * sin_theta) + (k[0] * dot) * (1 - cos_theta);
  r[1] = (v[1] * cos_theta) + (p[1] * sin_theta) + (k[1] * dot) * (1 - cos_theta);
  r[2] = (v[2] * cos_theta) + (p[2] * sin_theta) + (k[2] * dot) * (1 - cos_theta);
}

//----------------------------------------------------------------------------
//  Function to convert lon/lat coordinates to cartesian
//----------------------------------------------------------------------------
int LLtoXYZ(double lon, double lat, double* x, double* y, double* z, int projectionMode)
{
  double tx = 0.0;
  double ty = 0.0;
  double tz = 0.0;

  if (projectionMode == 0) // Project Spherical
  {
    lat += vtkMath::Pi() * 0.5;
    tx = sin(lat) * cos(lon);
    ty = sin(lat) * sin(lon);
    tz = cos(lat);
  }
  else if (projectionMode == 1) // Lat/Lon
  {
    tx = lon;
    ty = lat;
    tz = 0.0;
  }
  else if (projectionMode == 2) // Cassini
  {
    tx = asin(cos(lat) * sin(lon));
    ty = atan2(sin(lat), (cos(lat) * cos(lon)));
    tz = 0.0;
  }
  else if (projectionMode == 3) // Mollweide
  {
    const double theta = get_theta(lon, lat);
    tx = (2 * sqrt(2) / vtkMath::Pi()) * lon * cos(theta);
    ty = sqrt(2) * sin(theta);
    tz = 0.0;

    //    tx = (2 * sqrt(2) / vtkMath::Pi()) * lon *
    //      cos((lat) - ((2 * lat + sin(2 * lat) - vtkMath::Pi() * sin(lat)) / (2 + 2 * cos(lat))));
    //    ty = sqrt(2) *
    //      sin((lat) - ((2 * lat + sin(2 * lat) - vtkMath::Pi() * sin(lat)) / (2 + 2 * cos(lat))));
    //    tz = 0.0;
  }
  else if (projectionMode == 4) // Catalyst (lat/lon)
  {
    tx = lon;
    ty = lat;
    tz = 0.0;
  }
  else if (projectionMode == 5) // Spilhause
  {
    double v[3], p[3];
    lat += vtkMath::Pi() * 0.5;
    v[0] = sin(lat) * cos(lon);
    v[1] = sin(lat) * sin(lon);
    v[2] = cos(lat);

    rotateVec(v, p);
    double lon_rot, lat_rot, radius;
    bool temp = CartesianToSpherical(p[0], p[1], p[2], &radius, &lat_rot, &lon_rot);
    lon_rot -= 1.536194408171;
    lat_rot -= M_PI_2;

    if (lon_rot < (-1 * M_PI))
      lon_rot += 2 * M_PI;

    double a = 0., b = 0.;
    bool sm = false, sn = false;

    const double spp = tan(0.5 * lat_rot);
    a = cos(aasin(spp)) * sin(0.5 * lon_rot);
    sm = (spp + a) < 0.;
    sn = (spp - a) < 0.;
    b = aacos(spp);
    a = aacos(a);

    double m = aasin(sqrt((1. + std::min(0.0, cos(a + b)))));
    if (sm)
      m = -m;

    double n = aasin(sqrt(fabs(1. - std::max(0.0, cos(a - b)))));
    if (sn)
      n = -n;

    tx = ell_int_5(m);
    ty = ell_int_5(n);
    tz = 0.0;
  }

  if (vtkMath::IsNan(tx) || vtkMath::IsNan(ty) || vtkMath::IsNan(tz))
  {
    return -1;
  }

  *x = tx;
  *y = ty;
  *z = tz;

  return 1;
}

void get_scaling(const projection ProjectionMode, const bool ShowMultilayerView,
  const double VertLev, const double adjustedLayerThickness, double& sx, double& sy, double& sz)
{
  switch (ProjectionMode)
  {
    case SPHERICAL:
      sx = 200;
      sy = 200;
      if (ShowMultilayerView)
        sz = 200;
      else
      {
        float scale = adjustedLayerThickness * VertLev;
        sz = 200 - scale;
      }
      break;
    case CYLINDRICAL_EQUIDISTANT:
      sx = 300 / vtkMath::Pi();
      sy = 300 / vtkMath::Pi();
      sz = 0;
      break;
    case CASSINI:
      sx = 80.;
      sy = 80.;
      sz = 0.;
      break;
    case MOLLWEIDE:
      sx = 120.;
      sy = 120.;
      sz = 0.;
      break;
    case SPILHOUSE:
      sx = 120.;
      sy = 120.;
      sz = 0;
      break;
    case CATALYST:
      sx = 1;
      sy = 1;
      sz = 0;
      break;
  }
  return;
}
