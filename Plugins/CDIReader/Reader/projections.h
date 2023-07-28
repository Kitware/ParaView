// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-FileCopyrightText: Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CDI_READER_PROJECTIONS_
#define CDI_READER_PROJECTIONS_

namespace projection
{
enum Projection
{
  SPHERICAL = 0,
  CYLINDRICAL_EQUIDISTANT = 1,
  CASSINI = 2,
  MOLLWEIDE = 3,
  CATALYST = 4,
  SPILHOUSE = 5
};

inline bool isproj(const int p)
{
  return p >= Projection::SPHERICAL && p <= Projection::SPILHOUSE;
}

void get_scaling(const Projection projectionMode, const bool showMultilayerView,
  const double VertLev, const double adjustedLayerThickness, double& sx, double& sy, double& sz);

int cartesianToSpherical(double x, double y, double z, double* rho, double* phi, double* theta);

/**
 * Function to convert spherical coordinates to cartesian, for use in
 * computing points in different layers of multilayer spherical view
 */
int sphericalToCartesian(double rho, double phi, double theta, double* x, double* y, double* z);

/**
 * Function to convert lon/lat coordinates to cartesian
 */
int longLatToCartesian(double lon, double lat, double* x, double* y, double* z, int projectionMode);

}; // end of namespace projection

#endif /* CDI_READER_PROJECTIONS_ */
