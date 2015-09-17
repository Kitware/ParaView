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
#include "FlatIndex.h"
#include "CartesianExtent.h"

#include <iostream>

//-----------------------------------------------------------------------------
FlatIndex::FlatIndex(int ni, int nj, int nk, int mode)
{
  this->Initialize(ni,nj,nk,mode);
}

//-----------------------------------------------------------------------------
FlatIndex::FlatIndex(const CartesianExtent &ext, int nghost)
{
  this->Initialize(ext,nghost);
}

//-----------------------------------------------------------------------------
void FlatIndex::Initialize(const CartesianExtent &ext, int nghost)
{
  int nx[3];
  ext.Size(nx);

  int mode=0;
  if (nghost)
    {
    mode=CartesianExtent::GetDimensionMode(ext,nghost);
    }
  else
    {
    mode=CartesianExtent::GetDimensionMode(ext);
    }

  this->Initialize(nx[0],nx[1],nx[2],mode);
}

//-----------------------------------------------------------------------------
void FlatIndex::Initialize(int ni, int nj, int nk, int mode)
{
  (void)nk;
  switch(mode)
    {
    case CartesianExtent::DIM_MODE_2D_XZ:
      this->A = ni;
      this->B = 0; // ni*nk;
      this->C = 1;
      break;

    case CartesianExtent::DIM_MODE_2D_YZ:
      this->A = nj;
      this->B = 1;
      this->C = 0; // nj*nk;
      break;

    case CartesianExtent::DIM_MODE_2D_XY:
      this->A = 0; // ni*nj;
      this->B = ni;
      this->C = 1;
      break;

    case CartesianExtent::DIM_MODE_3D:
      this->A = ni*nj;
      this->B = ni;
      this->C = 1;
      break;

    default:
      std::cerr << "Unsupported mode " << mode << std::endl;
      break;
    }
}
