/*=========================================================================

   Program: ParaView
   Module:    pqChartCoordinate.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/*!
 * \file pqChartCoordinate.cxx
 *
 * \brief
 *   The pqChartCoordinate class stores an x, y coordinate for a chart.
 *
 * \author Mark Richardson
 * \date   August 19, 2005
 */

#include "pqChartCoordinate.h"
#include <vtkstd/vector>


/// \class pqChartCoordinateIteratorData
/// \brief
///   The pqChartCoordinateIteratorData class hides the private data of
///   the pqChartCoordinateIterator class.
class pqChartCoordinateIteratorData : public vtkstd::vector<pqChartCoordinate>::iterator
{
public:
  pqChartCoordinateIteratorData& operator=(
      const vtkstd::vector<pqChartCoordinate>::iterator& iter)
    {
    vtkstd::vector<pqChartCoordinate>::iterator::operator=(iter);
    return *this;
    }
};


/// \class pqChartCoordinateConstIteratorData
/// \brief
///   The pqChartCoordinateConstIteratorData class hides the private
///   data of the pqChartCoordinateConstIterator class.
class pqChartCoordinateConstIteratorData :
    public vtkstd::vector<pqChartCoordinate>::const_iterator
{
public:
  pqChartCoordinateConstIteratorData& operator=(
      const vtkstd::vector<pqChartCoordinate>::iterator& iter)
    {
    vtkstd::vector<pqChartCoordinate>::const_iterator::operator=(iter);
    return *this;
    }

  pqChartCoordinateConstIteratorData& operator=(
      const vtkstd::vector<pqChartCoordinate>::const_iterator& iter)
    {
    vtkstd::vector<pqChartCoordinate>::const_iterator::operator=(iter);
    return *this;
    }
};


/// \class pqChartCoordinateListData
/// \brief
///   The pqChartCoordinateListData class hides the private data of the
///   pqChartCoordinateList class.
class pqChartCoordinateListData : public vtkstd::vector<pqChartCoordinate> {};


pqChartCoordinate::pqChartCoordinate()
  : X(), Y()
{
}

pqChartCoordinate::pqChartCoordinate(const pqChartValue &px,
    const pqChartValue &py)
  : X(px), Y(py)
{
}

pqChartCoordinate::pqChartCoordinate(const pqChartCoordinate &other)
  : X(other.X), Y(other.Y)
{
}

bool pqChartCoordinate::operator==(const pqChartCoordinate &other) const
{
  return this->X == other.X && this->Y == other.Y;
}

bool pqChartCoordinate::operator!=(const pqChartCoordinate &other) const
{
  return this->X != other.X || this->Y != other.Y;
}

pqChartCoordinate &pqChartCoordinate::operator=(const pqChartCoordinate &other)
{
  this->X = other.X;
  this->Y = other.Y;
  return *this;
}


pqChartCoordinateIterator::pqChartCoordinateIterator()
{
  this->Data = new pqChartCoordinateIteratorData();
}

pqChartCoordinateIterator::pqChartCoordinateIterator(
    const pqChartCoordinateIterator &iter)
{
  this->Data = new pqChartCoordinateIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqChartCoordinateIterator::~pqChartCoordinateIterator()
{
  if(this->Data)
    delete this->Data;
}

bool pqChartCoordinateIterator::operator==(
    const pqChartCoordinateIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartCoordinateIterator::operator!=(
    const pqChartCoordinateIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

bool pqChartCoordinateIterator::operator==(
    const pqChartCoordinateConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartCoordinateIterator::operator!=(
    const pqChartCoordinateConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

const pqChartCoordinate &pqChartCoordinateIterator::operator*() const
{
  return *(*this->Data);
}

pqChartCoordinate &pqChartCoordinateIterator::operator*()
{
  return *(*this->Data);
}

pqChartCoordinate *pqChartCoordinateIterator::operator->()
{
  if(this->Data)
    return &(*(*this->Data));
  return 0;
}

pqChartCoordinateIterator &pqChartCoordinateIterator::operator++()
{
  if(this->Data)
    ++(*this->Data);
  return *this;
}

pqChartCoordinateIterator pqChartCoordinateIterator::operator++(int)
{
  pqChartCoordinateIterator result(*this);
  if(this->Data)
    ++(this->Data);
  return result;
}

pqChartCoordinateIterator &pqChartCoordinateIterator::operator=(
    const pqChartCoordinateIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}


pqChartCoordinateConstIterator::pqChartCoordinateConstIterator()
{
  this->Data = new pqChartCoordinateConstIteratorData();
}

pqChartCoordinateConstIterator::pqChartCoordinateConstIterator(
    const pqChartCoordinateIterator &iter)
{
  this->Data = new pqChartCoordinateConstIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqChartCoordinateConstIterator::pqChartCoordinateConstIterator(
    const pqChartCoordinateConstIterator &iter)
{
  this->Data = new pqChartCoordinateConstIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqChartCoordinateConstIterator::~pqChartCoordinateConstIterator()
{
  if(this->Data)
    delete this->Data;
}

bool pqChartCoordinateConstIterator::operator==(
    const pqChartCoordinateConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartCoordinateConstIterator::operator!=(
    const pqChartCoordinateConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

bool pqChartCoordinateConstIterator::operator==(
    const pqChartCoordinateIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqChartCoordinateConstIterator::operator!=(
    const pqChartCoordinateIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

const pqChartCoordinate &pqChartCoordinateConstIterator::operator*() const
{
  return *(*this->Data);
}

const pqChartCoordinate *pqChartCoordinateConstIterator::operator->() const
{
  if(this->Data)
    return &(*(*this->Data));
  return 0;
}

pqChartCoordinateConstIterator &pqChartCoordinateConstIterator::operator++()
{
  if(this->Data)
    ++(*this->Data);
  return *this;
}

pqChartCoordinateConstIterator pqChartCoordinateConstIterator::operator++(int)
{
  pqChartCoordinateConstIterator result(*this);
  if(this->Data)
    ++(this->Data);
  return result;
}

pqChartCoordinateConstIterator &pqChartCoordinateConstIterator::operator=(
    const pqChartCoordinateConstIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}

pqChartCoordinateConstIterator &pqChartCoordinateConstIterator::operator=(
    const pqChartCoordinateIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}


pqChartCoordinateList::pqChartCoordinateList() :
  Data(new pqChartCoordinateListData())
{
}

pqChartCoordinateList::pqChartCoordinateList(const pqChartCoordinateList& rhs) :
  Data(new pqChartCoordinateListData(*rhs.Data))
{
}

pqChartCoordinateList& pqChartCoordinateList::operator=(const pqChartCoordinateList& list)
{
  *(this->Data) = *(list.Data);
  return *this;
}

pqChartCoordinateList::~pqChartCoordinateList()
{
  delete this->Data;
}

pqChartCoordinateList::Iterator pqChartCoordinateList::begin()
{
  pqChartCoordinateIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqChartCoordinateList::Iterator pqChartCoordinateList::end()
{
  pqChartCoordinateIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

pqChartCoordinateList::ConstIterator pqChartCoordinateList::begin() const
{
  pqChartCoordinateConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqChartCoordinateList::ConstIterator pqChartCoordinateList::end() const
{
  pqChartCoordinateConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

pqChartCoordinateList::ConstIterator pqChartCoordinateList::constBegin() const
{
  pqChartCoordinateConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqChartCoordinateList::ConstIterator pqChartCoordinateList::constEnd() const
{
  pqChartCoordinateConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

bool pqChartCoordinateList::isEmpty() const
{
  if(this->Data)
    return this->Data->size() == 0;
  return true;
}

int pqChartCoordinateList::getSize() const
{
  if(this->Data)
    return static_cast<int>(this->Data->size());
  return 0;
}

void pqChartCoordinateList::clear()
{
  if(this->Data)
    this->Data->clear();
}

void pqChartCoordinateList::pushBack(const pqChartCoordinate &coord)
{
  if(this->Data)
    this->Data->push_back(coord);
}

pqChartCoordinateList &pqChartCoordinateList::operator+=(
    const pqChartCoordinateList &list)
{
  if(this->Data && list.Data)
  {
    vtkstd::vector<pqChartCoordinate>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      this->Data->push_back(*iter);
  }

  return *this;
}

pqChartCoordinate &pqChartCoordinateList::operator[](int index)
{
  return (*this->Data)[index];
}

const pqChartCoordinate &pqChartCoordinateList::operator[](int index) const
{
  return (*this->Data)[index];
}


