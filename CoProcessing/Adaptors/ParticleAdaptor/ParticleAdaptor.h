/*=========================================================================

  Program:   ParaView
  Module:    ParticleAdaptor.h

  Copyright (c) Sandia Corporation, Kitware, Inc.
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

=========================================================================*/
#ifndef ParticleAdaptor_h
#define ParticleAdaptor_h

#ifdef __cplusplus
extern "C" {
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
