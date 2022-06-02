/*=========================================================================

   Program: ParaView
   Module:  projections.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

/*-------------------------------------------------------------------------
   Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
  -------------------------------------------------------------------------*/

#include "projections.h"

#include "vtkMath.h"

#include <cmath>

namespace projection
{
namespace details
{
//----------------------------------------------------------------------------
inline double iterate_theta(const double lat, const double theta)
{
  const double pi = vtkMath::Pi();
  return theta - (2 * theta + sin(2 * theta) - pi * sin(lat)) / (2 + 2 * cos(2 * theta));
}

//----------------------------------------------------------------------------
inline double get_theta(const double lat)
{
  double theta = lat;
  const double pi = vtkMath::Pi();
  if (lat == pi / 2.)
    return pi / 2;
  else if (lat == -pi / 2.)
    return -pi / 2;

  for (int i = 0; i < 5; i++)
    theta = details::iterate_theta(lat, theta);
  return theta;
}

//----------------------------------------------------------------------------
double clamp_asin(double v)
{
  return std::asin(vtkMath::ClampValue(v, -1., 1.));
}

//----------------------------------------------------------------------------
double clamp_acos(double v)
{
  return std::acos(vtkMath::ClampValue(v, -1., 1.));
}

//----------------------------------------------------------------------------
void rotateVec(double v[], double r[])
{
  double theta = 1.0472;
  double cos_theta = cos(theta);
  double sin_theta = sin(theta);

  double k[3], p[3];

  k[0] = cos(3.57792);
  k[1] = sin(3.57792);
  k[2] = 0.0;

  vtkMath::Cross(k, v, p);
  double dot = vtkMath::Dot(k, v);

  r[0] = (v[0] * cos_theta) + (p[0] * sin_theta) + (k[0] * dot) * (1 - cos_theta);
  r[1] = (v[1] * cos_theta) + (p[1] * sin_theta) + (k[1] * dot) * (1 - cos_theta);
  r[2] = (v[2] * cos_theta) + (p[2] * sin_theta) + (k[2] * dot) * (1 - cos_theta);
}
}; // end of namespace details

//----------------------------------------------------------------------------
int cartesianToSpherical(double x, double y, double z, double* rho, double* phi, double* theta)
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
int sphericalToCartesian(double rho, double phi, double theta, double* x, double* y, double* z)
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

double ell_int_5(double phi)
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

  double y = phi * 2 / vtkMath::Pi();
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

//----------------------------------------------------------------------------
int longLatToCartesian(double lon, double lat, double* x, double* y, double* z, int projectionMode)
{
  double tx = 0.0;
  double ty = 0.0;
  double tz = 0.0;

  if (projectionMode == SPHERICAL)
  {
    lat += vtkMath::Pi() * 0.5;
    tx = sin(lat) * cos(lon);
    ty = sin(lat) * sin(lon);
    tz = cos(lat);
  }
  else if (projectionMode == CYLINDRICAL_EQUIDISTANT) // Lat/Lon
  {
    tx = lon;
    ty = lat;
    tz = 0.0;
  }
  else if (projectionMode == CASSINI)
  {
    tx = asin(cos(lat) * sin(lon));
    ty = atan2(sin(lat), (cos(lat) * cos(lon)));
    tz = 0.0;
  }
  else if (projectionMode == MOLLWEIDE)
  {
    const double theta = details::get_theta(lat);
    tx = (2 * sqrt(2) / vtkMath::Pi()) * lon * cos(theta);
    ty = sqrt(2) * sin(theta);
    tz = 0.0;
  }
  else if (projectionMode == CATALYST)
  {
    tx = lon;
    ty = lat;
    tz = 0.0;
  }
  else if (projectionMode == SPILHOUSE)
  {
    double v[3], p[3];
    lat += vtkMath::Pi() * 0.5;
    v[0] = sin(lat) * cos(lon);
    v[1] = sin(lat) * sin(lon);
    v[2] = cos(lat);

    details::rotateVec(v, p);
    double lon_rot, lat_rot, radius;
    (void)cartesianToSpherical(p[0], p[1], p[2], &radius, &lat_rot, &lon_rot);
    lon_rot -= 1.536194408171;
    lat_rot -= vtkMath::Pi() * 0.5;

    if (lon_rot < (-1 * vtkMath::Pi()))
      lon_rot += 2 * vtkMath::Pi();

    double a = 0., b = 0.;
    bool sm = false, sn = false;

    const double spp = tan(0.5 * lat_rot);
    a = cos(details::clamp_asin(spp)) * sin(0.5 * lon_rot);
    sm = (spp + a) < 0.;
    sn = (spp - a) < 0.;
    b = details::clamp_acos(spp);
    a = details::clamp_acos(a);

    double m = details::clamp_asin(sqrt((1. + std::min(0.0, cos(a + b)))));
    if (sm)
      m = -m;

    double n = details::clamp_asin(sqrt(fabs(1. - std::max(0.0, cos(a - b)))));
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

//----------------------------------------------------------------------------
void get_scaling(const Projection projectionMode, const bool showMultilayerView,
  const double vertLev, const double adjustedLayerThickness, double& sx, double& sy, double& sz)
{
  switch (projectionMode)
  {
    case SPHERICAL:
      sx = 200;
      sy = 200;
      if (showMultilayerView)
        sz = 200;
      else
      {
        float scale = adjustedLayerThickness * vertLev;
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
}
} // end of namespace projection
