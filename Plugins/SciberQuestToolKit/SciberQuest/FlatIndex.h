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
#ifndef FlatIndex_h
#define FlatIndex_h
#include <cstdlib> // for size_t

class CartesianExtent;

/// FlatIndex - A class to convert i,j,k tuples into flat indices
/**
The following formula is applied:
<pre>
mode -> k*(A)    + j*(B)    +i*(C)
--------------------------------------
3d   -> k*(ninj) + j*(ni)   + i
xy   ->            j*(ni)   + i
xz   -> k*(ni)              + i
yz   -> k*(nj)   + j
--------------------------------------
</pre>
*/
class FlatIndex
{
public:
  FlatIndex() : A(0), B(0), C(0) {}
  FlatIndex(int ni, int nj, int nk, int mode);
  FlatIndex(const CartesianExtent &ext, int nghost=0);

  void Initialize(int ni, int nj, int nk, int mode);
  void Initialize(const CartesianExtent &ext, int nghost=0);

  size_t Index(int i, int j, int k)
  {
    return k*this->A + j*this->B + i*this->C;
  }

private:
  size_t A;
  size_t B;
  size_t C;
};

#endif

// VTK-HeaderTest-Exclude: FlatIndex.h
