/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ImageDecomp_h
#define ImageDecomp_h


#include "CartesianDecomp.h" // for CartesianDecomp

class CartesianDataBlock;
class CartesianDataBlockIODescriptor;


/// Splits a cartesian grid into a set of smaller cartesian grids.
/**
Splits a cartesian grid into a set of smaller cartesian grids using
a set of axis aligned planes. Given a point will locate and return
the sub-grid which contains it.
*/
class ImageDecomp : public CartesianDecomp
{
public:
  static ImageDecomp *New(){ return new ImageDecomp; }

  /**
  Get/Set the domain origin.
  */
  void SetOrigin(double x0, double y0, double z0);
  void SetOrigin(const double X0[3]);
  double *GetOrigin(){ return this->X0; }
  const double *GetOrigin() const { return this->X0; }

  /**
  Get/Set the grid spacing.
  */
  void SetSpacing(double dx, double dy, double dz);
  void SetSpacing(const double dX[3]);
  double *GetSpacing(){ return this->DX; }
  const double *GetSpacing() const { return this->DX; }

  /**
  Set the domain bounds based on curren values of
  extent, oring and spacing.
  */
  void ComputeBounds();

  /**
  Decompose the domain in to the requested number of blocks.
  */
  int DecomposeDomain();

protected:
  ImageDecomp();
  ~ImageDecomp();

private:
  void operator=(ImageDecomp &); // not implemented
  ImageDecomp(ImageDecomp &); // not implemented

private:
  double DX[3];                 // grid spacing
  double X0[3];                 // origin
};

#endif

// VTK-HeaderTest-Exclude: ImageDecomp.h
