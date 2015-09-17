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
#include "CartesianExtentIterator.h"

#include "Tuple.hxx"
#include "SQMacros.h"
#include "postream.h"

#include <climits>

//-----------------------------------------------------------------------------
CartesianExtentIterator::CartesianExtentIterator()
      :
  P(INT_MAX),
  Q(INT_MAX),
  R(INT_MAX)
{}

//-----------------------------------------------------------------------------
CartesianExtentIterator::CartesianExtentIterator(const CartesianExtent &extent)
      :
  Indexer(extent),
  Extent(extent),
  P(extent[0]),
  Q(extent[2]),
  R(extent[4])
{}

//-----------------------------------------------------------------------------
CartesianExtentIterator::CartesianExtentIterator(
  const CartesianExtent &domain,
  const CartesianExtent &extent)
      :
  Indexer(domain),
  Extent(extent),
  P(extent[0]),
  Q(extent[2]),
  R(extent[4])
{}

//-----------------------------------------------------------------------------
CartesianExtentIterator::CartesianExtentIterator(
  const CartesianExtentIterator &other)
      :
  CellIdIterator(other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
void CartesianExtentIterator::SetDomain(CartesianExtent &domain)
{
  this->Indexer.Initialize(domain);
}

//-----------------------------------------------------------------------------
void CartesianExtentIterator::SetExtent(CartesianExtent &extent)
{
  this->Extent=extent;
  this->Reset();
}

//-----------------------------------------------------------------------------
CartesianExtentIterator &CartesianExtentIterator::operator=(
      const CartesianExtentIterator &other)
{
  if (this == &other) return *this;

  this->Indexer=other.Indexer;
  this->Extent=other.Extent;
  this->P=other.P;
  this->Q=other.Q;
  this->R=other.R;

  return *this;
}

//-----------------------------------------------------------------------------
CellIdIterator &CartesianExtentIterator::operator=(const CellIdIterator &other)
{
  return this->operator=(dynamic_cast<const CartesianExtentIterator &>(other));
}

//-----------------------------------------------------------------------------
void CartesianExtentIterator::Reset()
{
  if (this->Extent.Empty())
    {
    this->P=
    this->Q=
    this->R=INT_MAX;
    }
  else
    {
    this->P=this->Extent[0];
    this->Q=this->Extent[2];
    this->R=this->Extent[4];
    }
}
