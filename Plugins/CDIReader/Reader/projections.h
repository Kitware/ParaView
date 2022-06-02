/*=========================================================================

   Program: ParaView
   Module:  projections.h

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
