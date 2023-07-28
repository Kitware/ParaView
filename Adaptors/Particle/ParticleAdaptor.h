// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef ParticleAdaptor_h
#define ParticleAdaptor_h

#ifdef __cplusplus
extern "C"
{
#endif

  void coprocessorinitialize(void* handle);

  // Description:
  // creates an image based on particle positions passed in xyz.
  // n is the number of particles
  // xyz particle locations in interlaced format (x1,y1,z1,x2,y2,z2,...)
  // bounds the bounds of the space (xmin,ymin,zmin,xmax,ymax,zmax)
  //             leave as null if there are no bounds
  // r is the radius of the particles as rendered
  // attr the attribute to color by, should be of length n
  // min and max are the minimum and maximum value of the attributes
  //             used to specify the linear mapping to color
  // theta and phi represent the camera position angles around
  //             the perimeter
  //             camera always look at the center of the xyz,
  //             camera's up will always be positive or zero
  //             in y (depending on phi)
  // z represents distance from the center, setting this to zero will
  //             place the camera so that it can see all particles
  void coprocessorcreateimage(int timestep, double time, char* filename_base, int n, double* xyz,
    double* bounds, double r, double* attr, double min, double max, double theta, double phi,
    double z);

  void coprocessorfinalize();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ParticleAdaptor_h */
