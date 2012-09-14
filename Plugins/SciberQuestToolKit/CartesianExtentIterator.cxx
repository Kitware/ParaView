/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
