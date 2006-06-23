/*=========================================================================

   Program: ParaView
   Module:    pqHistogramSelection.cxx

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
 * \file pqHistogramSelection.cxx
 *
 * \brief
 *   The pqHistogramSelection and pqHistogramSelectionList classes are
 *   used to define the selection for a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   June 2, 2005
 */

#include "pqHistogramSelection.h"
#include <vtkstd/list>



/// \class pqHistogramSelectionIteratorData
/// \brief
///   The pqHistogramSelectionIteratorData class hides the private data of
///   the pqHistogramSelectionIterator class.
class pqHistogramSelectionIteratorData :
    public vtkstd::list<pqHistogramSelection *>::iterator
{
public:
  pqHistogramSelectionIteratorData& operator=(
      const vtkstd::list<pqHistogramSelection *>::iterator& iter)
    {
    vtkstd::list<pqHistogramSelection *>::iterator::operator=(iter);
    return *this;
    }
};


/// \class pqHistogramSelectionConstIteratorData
/// \brief
///   The pqHistogramSelectionConstIteratorData class hides the private
///   data of the pqHistogramSelectionConstIterator class.
class pqHistogramSelectionConstIteratorData :
    public vtkstd::list<pqHistogramSelection *>::const_iterator
{
public:
  pqHistogramSelectionConstIteratorData& operator=(
      const vtkstd::list<pqHistogramSelection *>::iterator& iter)
    {
    vtkstd::list<pqHistogramSelection *>::const_iterator::operator=(iter);
    return *this;
    }

  pqHistogramSelectionConstIteratorData& operator=(
      const vtkstd::list<pqHistogramSelection *>::const_iterator& iter)
    {
    vtkstd::list<pqHistogramSelection *>::const_iterator::operator=(iter);
    return *this;
   }
};

/// \class pqHistogramSelectionListData
/// \brief
///   The pqHistogramSelectionListData class hides the private data of the
///   pqHistogramSelectionList class.
class pqHistogramSelectionListData : public vtkstd::list<pqHistogramSelection *> {};


pqHistogramSelection::pqHistogramSelection()
  : First(), Second()
{
  this->Type = pqHistogramSelection::Value;
}

pqHistogramSelection::pqHistogramSelection(const pqHistogramSelection &other)
  : First(other.First), Second(other.Second)
{
  this->Type = other.Type;
}

pqHistogramSelection::pqHistogramSelection( const pqChartValue &first,
    const pqChartValue &second)
  : First(), Second()
{
  this->Type = pqHistogramSelection::Value;
  this->First = first;
  this->Second = second;
}

void pqHistogramSelection::reverse()
{
  pqChartValue temp = this->First;
  this->First = this->Second;
  this->Second = temp;
}

void pqHistogramSelection::adjustRange(const pqChartValue &min,
    const pqChartValue &max)
{
  if(this->First < min)
    this->First = min;
  else if(this->First > max)
    this->First = max;
  if(this->Second < min)
    this->Second = min;
  else if(this->Second > max)
    this->Second = max;
}

void pqHistogramSelection::moveRange(const pqChartValue &offset)
{
  this->First += offset;
  this->Second += offset;
}

void pqHistogramSelection::setRange(const pqChartValue &first,
    const pqChartValue &last)
{
  this->First = first;
  this->Second = last;
}

pqHistogramSelection &pqHistogramSelection::operator=(
    const pqHistogramSelection &other)
{
  this->Type = other.Type;
  this->First = other.First;
  this->Second = other.Second;
  return *this;
}


pqHistogramSelectionIterator::pqHistogramSelectionIterator()
{
  this->Data = new pqHistogramSelectionIteratorData();
}

pqHistogramSelectionIterator::pqHistogramSelectionIterator(
    const pqHistogramSelectionIterator &iter)
{
  this->Data = new pqHistogramSelectionIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqHistogramSelectionIterator::~pqHistogramSelectionIterator()
{
  if(this->Data)
    {
    delete this->Data;
    this->Data = 0;
    }
}

bool pqHistogramSelectionIterator::operator==(
    const pqHistogramSelectionIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqHistogramSelectionIterator::operator!=(
    const pqHistogramSelectionIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

bool pqHistogramSelectionIterator::operator==(
    const pqHistogramSelectionConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqHistogramSelectionIterator::operator!=(
    const pqHistogramSelectionConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

const pqHistogramSelection *pqHistogramSelectionIterator::operator*() const
{
  if(this->Data)
    return *(*this->Data);
  return 0;
}

pqHistogramSelection *&pqHistogramSelectionIterator::operator*()
{
  return *(*this->Data);
}

pqHistogramSelection *pqHistogramSelectionIterator::operator->()
{
  if(this->Data)
    return *(*this->Data);
  return 0;
}

pqHistogramSelectionIterator &pqHistogramSelectionIterator::operator++()
{
  if(this->Data)
    ++(*this->Data);
  return *this;
}

pqHistogramSelectionIterator pqHistogramSelectionIterator::operator++(int)
{
  pqHistogramSelectionIterator result(*this);
  if(this->Data)
    ++(*this->Data);
  return result;
}

pqHistogramSelectionIterator &pqHistogramSelectionIterator::operator=(
    const pqHistogramSelectionIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}


pqHistogramSelectionConstIterator::pqHistogramSelectionConstIterator()
{
  this->Data = new pqHistogramSelectionConstIteratorData();
}

pqHistogramSelectionConstIterator::pqHistogramSelectionConstIterator(
    const pqHistogramSelectionIterator &iter)
{
  this->Data = new pqHistogramSelectionConstIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqHistogramSelectionConstIterator::pqHistogramSelectionConstIterator(
    const pqHistogramSelectionConstIterator &iter)
{
  this->Data = new pqHistogramSelectionConstIteratorData();
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
}

pqHistogramSelectionConstIterator::~pqHistogramSelectionConstIterator()
{
  if(this->Data)
    {
    delete this->Data;
    this->Data = 0;
    }
}

bool pqHistogramSelectionConstIterator::operator==(
    const pqHistogramSelectionConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqHistogramSelectionConstIterator::operator!=(
    const pqHistogramSelectionConstIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

bool pqHistogramSelectionConstIterator::operator==(
    const pqHistogramSelectionIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data == *iter.Data;
  else if(!this->Data && !iter.Data)
    return true;

  return false;
}

bool pqHistogramSelectionConstIterator::operator!=(
    const pqHistogramSelectionIterator &iter) const
{
  if(this->Data && iter.Data)
    return *this->Data != *iter.Data;
  else if(!this->Data && !iter.Data)
    return false;

  return true;
}

const pqHistogramSelection *pqHistogramSelectionConstIterator::operator*() const
{
  if(this->Data)
    return *(*this->Data);
  return 0;
}

const pqHistogramSelection *pqHistogramSelectionConstIterator::operator->() const
{
  if(this->Data)
    return *(*this->Data);
  return 0;
}

pqHistogramSelectionConstIterator &pqHistogramSelectionConstIterator::operator++()
{
  if(this->Data)
    ++(*this->Data);
  return *this;
}

pqHistogramSelectionConstIterator pqHistogramSelectionConstIterator::operator++(int)
{
  pqHistogramSelectionConstIterator result(*this);
  if(this->Data)
    ++(*this->Data);
  return result;
}

pqHistogramSelectionConstIterator &pqHistogramSelectionConstIterator::operator=(
    const pqHistogramSelectionConstIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}

pqHistogramSelectionConstIterator &pqHistogramSelectionConstIterator::operator=(
    const pqHistogramSelectionIterator &iter)
{
  if(this->Data && iter.Data)
    *this->Data = *iter.Data;
  return *this;
}


pqHistogramSelectionList::pqHistogramSelectionList()
{
  this->Data = new pqHistogramSelectionListData();
  this->Type = pqHistogramSelection::None;
  this->Sorted = true;
}

pqHistogramSelectionList::pqHistogramSelectionList(
    const pqHistogramSelectionList &list)
{
  this->Data = new pqHistogramSelectionListData();
  this->Type = pqHistogramSelection::None;
  this->Sorted = true;
  if(this->Data && list.Data)
    {
    this->Type = list.Type;
    this->Sorted = list.Sorted;
    vtkstd::list<pqHistogramSelection *>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      this->Data->push_back(*iter);
    }
}

pqHistogramSelectionList::~pqHistogramSelectionList()
{
  if(this->Data)
    {
    delete this->Data;
    this->Data = 0;
    }
}

void pqHistogramSelectionList::makeNewCopy(const pqHistogramSelectionList &list)
{
  if(!this->Data)
    return;

  clear();
  if(list.Data)
    {
    this->Type = list.Type;
    this->Sorted = list.Sorted;
    pqHistogramSelection *selection = 0;
    vtkstd::list<pqHistogramSelection *>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      {
      if(*iter)
        {
        selection = new pqHistogramSelection(*(*iter));
        if(selection)
          this->Data->push_back(selection);
        }
      }
    }
}

pqHistogramSelectionList::Iterator pqHistogramSelectionList::begin()
{
  pqHistogramSelectionIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqHistogramSelectionList::Iterator pqHistogramSelectionList::end()
{
  pqHistogramSelectionIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

pqHistogramSelectionList::ConstIterator pqHistogramSelectionList::begin() const
{
  pqHistogramSelectionConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqHistogramSelectionList::ConstIterator pqHistogramSelectionList::end() const
{
  pqHistogramSelectionConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

pqHistogramSelectionList::ConstIterator pqHistogramSelectionList::constBegin() const
{
  pqHistogramSelectionConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->begin();
  return iter;
}

pqHistogramSelectionList::ConstIterator pqHistogramSelectionList::constEnd() const
{
  pqHistogramSelectionConstIterator iter;
  if(this->Data && iter.Data)
    *iter.Data = this->Data->end();
  return iter;
}

bool pqHistogramSelectionList::isEmpty() const
{
  if(this->Data)
    return this->Data->size() == 0;
  return true;
}

int pqHistogramSelectionList::getSize() const
{
  if(this->Data)
    return static_cast<int>(this->Data->size());
  return 0;
}

void pqHistogramSelectionList::clear()
{
  if(this->Data)
    {
    this->Sorted = true;
    this->Type = pqHistogramSelection::None;
    this->Data->clear();
    }
}

pqHistogramSelectionList::Iterator pqHistogramSelectionList::erase(
    pqHistogramSelectionList::Iterator position)
{
  if(this->Data && position.Data)
    *position.Data = this->Data->erase(*position.Data);
  return position;
}

void pqHistogramSelectionList::pushBack(pqHistogramSelection *selection)
{
  if(this->Data)
    {
    this->Sorted = false;
    this->Data->push_back(selection);
    }
}

void pqHistogramSelectionList::sortAndMerge(pqHistogramSelectionList &toDelete)
{
  this->Sorted = true;
  if(!this->Data)
    return;

  // Use a temporary list to sort the items according to the
  // left value of the selection item.
  vtkstd::list<pqHistogramSelection *> temp;
  vtkstd::list<pqHistogramSelection *>::iterator iter = this->Data->begin();
  vtkstd::list<pqHistogramSelection *>::iterator jter;
  for( ; iter != this->Data->end(); iter++)
    {
    if(*iter)
      {
      // Make sure the selection range is in order.
      if((*iter)->Second < (*iter)->First)
        (*iter)->reverse();

      if(this->Type == pqHistogramSelection::None)
        this->Type = (*iter)->getType();
      else if(this->Type != (*iter)->getType())
        {
        toDelete.pushBack(*iter);
        continue;
        }

      for(jter = temp.begin(); jter != temp.end(); jter++)
        {
        if((*iter)->First < (*jter)->First)
          {
          temp.insert(jter, *iter);
          break;
          }
        }

      if(jter == temp.end())
        temp.push_back(*iter);

      }
    else
      toDelete.pushBack(*iter);
    }

  // Put the items back on the member list while uniting overlapping
  // selection areas.
  pqChartValue v;
  this->Data->clear();
  iter = temp.begin();
  if(iter != temp.end())
    {
    this->Data->push_back(*iter);
    jter = iter++;
    }

  for( ; iter != temp.end(); iter++)
    {
    v = (*iter)->First;
    if(--v <= (*jter)->Second)
      {
      if((*jter)->Second < (*iter)->Second)
        (*jter)->Second = (*iter)->Second;
      toDelete.pushBack(*iter);
      }
    else
      {
      this->Data->push_back(*iter);
      jter = iter;
      }
    }
}

void pqHistogramSelectionList::unite(pqHistogramSelection *item,
    pqHistogramSelectionList &toDelete)
{
  if(!this->preprocessItem(item, toDelete))
    return;

  // Search through the sorted list for an intersection.
  pqChartValue v1;
  pqChartValue v2;
  vtkstd::list<pqHistogramSelection *>::iterator iter = this->Data->begin();
  for( ; iter != this->Data->end(); iter++)
    {
    v1 = (*iter)->First;
    v2 = (*iter)->Second;
    if(item->Second < --v1)
      {
      this->Data->insert(iter, item);
      item = 0;
      break;
      }
    else if(item->First <= ++v2)
      {
      if(item->First < (*iter)->First)
        (*iter)->First = item->First;
      if(item->Second > (*iter)->Second)
        {
        // If the new item's right boundary is greater than the
        // current item's right boundary, the newly created union
        // could intersect with subsequent items.
        (*iter)->Second = item->Second;
        pqHistogramSelection *next = 0;
        pqHistogramSelection *prev = *iter;
        for(iter++; iter != this->Data->end(); )
          {
          v1 = (*iter)->First;
          if(prev->Second < --v1)
            break;
          else
            {
            next = *iter;
            iter = this->Data->erase(iter);
            toDelete.pushBack(next);
            if(prev->Second <= next->Second)
              {
              prev->Second = next->Second;
              break;
              }
            }
          }
        }

      toDelete.pushBack(item);
      item = 0;
      break;
      }
    }

  if(item)
    this->Data->push_back(item);
}

void pqHistogramSelectionList::unite(pqHistogramSelectionList &list,
    pqHistogramSelectionList &toDelete)
{
  if(list.isEmpty())
    return;

  if(this->Sorted)
    {
    // Unite the items in the new list one at a time.
    pqHistogramSelectionList::Iterator iter = list.begin();
    for( ; iter != list.end(); iter++)
      unite(*iter, toDelete);
    }
  else
    {
    *this += list;
    this->sortAndMerge(toDelete);
    }

  list.clear();
}

void pqHistogramSelectionList::subtract(pqHistogramSelection *item,
    pqHistogramSelectionList &toDelete)
{
  if(!this->preprocessItem(item, toDelete))
    return;

  // Search through the sorted list for an intersection.
  pqChartValue v;
  vtkstd::list<pqHistogramSelection *>::iterator iter = this->Data->begin();
  while(iter != this->Data->end())
    {
    if(item->Second < (*iter)->First)
      break;
    else if(item->First <= (*iter)->Second)
      {
      if(item->Second <= (*iter)->Second)
        {
        if(item->Second == (*iter)->Second)
          {
          if(item->First > (*iter)->First)
            (*iter)->Second = --item->First;
          else
            {
            toDelete.pushBack(*iter);
            iter = this->Data->erase(iter);
            }
          }
        else if(item->First > (*iter)->First)
          {
          v = item->First;
          item->First = (*iter)->First;
          (*iter)->First = ++item->Second;
          item->Second = --v;
          iter = this->Data->insert(iter, item);
          if(*iter == item)
            item = 0;
          }
        else
          (*iter)->First = ++item->Second;

        break;
        }
      else if(item->First > (*iter)->First)
        {
        v = item->First;
        item->First = ++(*iter)->Second;
        (*iter)->Second = --v;
        iter++;
        }
      else
        {
        item->First = ++(*iter)->Second;
        toDelete.pushBack(*iter);
        iter = this->Data->erase(iter);
        }
      }
    else
      iter++;
    }

  if(item)
    toDelete.pushBack(item);
}

void pqHistogramSelectionList::subtract(pqHistogramSelectionList &list,
    pqHistogramSelectionList &toDelete)
{
  if(list.isEmpty())
    return;

  // Subtract the items in the new list one at a time.
  pqHistogramSelectionList::Iterator iter = list.begin();
  for( ; iter != list.end(); iter++)
    this->subtract(*iter, toDelete);

  list.clear();
}

void pqHistogramSelectionList::Xor(pqHistogramSelection *item,
    pqHistogramSelectionList &toDelete)
{
  if(!this->preprocessItem(item, toDelete))
    return;

  // Search through the sorted list for an intersection.
  vtkstd::list<pqHistogramSelection *>::iterator iter = this->Data->begin();
  pqChartValue v1;
  pqChartValue v2;
  for( ; iter != this->Data->end(); iter++)
    {
    v1 = (*iter)->First;
    v2 = (*iter)->Second;
    if(item->Second < --v1)
      {
      this->Data->insert(iter, item);
      item = 0;
      break;
      }
    else if(item->First <= ++v2)
      {
      if(item->Second <= (*iter)->Second)
        {
        if(item->Second == v1)
          {
          (*iter)->First = item->First;
          toDelete.pushBack(item);
          }
        else if(item->Second == (*iter)->Second)
          {
          if(item->First == (*iter)->First)
            {
            toDelete.pushBack(*iter);
            this->Data->erase(iter);
            }
          else if(item->First > (*iter)->First)
            (*iter)->Second = --item->First;
          else
            {
            (*iter)->Second = --(*iter)->First;
            (*iter)->First = item->First;
            }

          toDelete.pushBack(item);
          }
        else if(item->First == (*iter)->First)
          {
          (*iter)->First = ++item->Second;
          toDelete.pushBack(item);
          }
        else
          {
          // Move the left-most range to the new item so it can
          // be inserted before the current item.
          if(item->First > (*iter)->First)
            {
            v1 = (*iter)->First;
            (*iter)->First = ++item->Second;
            item->Second = --item->First;
            item->First = v1;
            }
          else
            {
            v1 = (*iter)->First;
            (*iter)->First = ++item->Second;
            item->Second = --v1;
            }

          this->Data->insert(iter, item);
          }

        item = 0;
        break;
        }
      else
        {
        if(item->First == v2)
          {
          (*iter)->Second = item->Second;
          toDelete.pushBack(item);
          item = 0;
          }
        else if(item->First == (*iter)->First)
          {
          (*iter)->First = ++(*iter)->Second;
          (*iter)->Second = item->Second;
          toDelete.pushBack(item);
          item = 0;
          }
        else if(item->First > (*iter)->First)
          {
          v1 = (*iter)->Second;
          (*iter)->Second = --item->First;
          item->First = ++v1;
          }
        else
          {
          v1 = (*iter)->First;
          (*iter)->First = item->First;
          item->First = ++(*iter)->Second;
          (*iter)->Second = --v1;
          }

        // If the new item's right boundary is greater than the
        // current item's right boundary, the leftover range could
        // intersect with subsequent items.
        pqHistogramSelection *prev = *iter;
        if(item)
          prev = item;
        for(iter++; iter != this->Data->end(); iter++)
          {
          if(prev->Second <= (*iter)->Second)
            {
            v1 = (*iter)->First;
            if(prev->Second < --v1)
              {
              if(prev == item)
                {
                this->Data->insert(iter, item);
                item = 0;
                }
              }
            else if(prev->Second == v1)
              {
              if(prev == item)
                {
                (*iter)->First = item->First;
                toDelete.pushBack(item);
                item = 0;
                }
              else
                {
                prev->Second = (*iter)->Second;
                toDelete.pushBack(*iter);
                this->Data->erase(iter);
                }
              }
            else if(prev->Second == (*iter)->Second)
              {
              if(prev == item)
                {
                (*iter)->Second = --(*iter)->First;
                (*iter)->First = item->First;
                toDelete.pushBack(item);
                item = 0;
                }
              else
                {
                prev->Second = --(*iter)->First;
                toDelete.pushBack(*iter);
                this->Data->erase(iter);
                }
              }
            else
              {
              v1 = (*iter)->First;
              (*iter)->First = ++prev->Second;
              prev->Second = --v1;
              if(prev == item)
                {
                this->Data->insert(iter, item);
                item = 0;
                }
              }

            break;
            }
          else
            {
            v1 = (*iter)->First;
            if(prev == item)
              {
              (*iter)->First = item->First;
              item->First = ++(*iter)->Second;
              (*iter)->Second = --v1;
              }
            else
              {
              (*iter)->First = ++(*iter)->Second;
              (*iter)->Second = prev->Second;
              prev->Second = --v1;
              prev = *iter;
              }
            }
          }

        break;
        }
      }
    }

  if(item)
    this->Data->push_back(item);
}

void pqHistogramSelectionList::Xor(pqHistogramSelectionList &list,
    pqHistogramSelectionList &toDelete)
{
  if(list.isEmpty())
    return;

  // Xor the items in the new list one at a time.
  pqHistogramSelectionList::Iterator iter = list.begin();
  for( ; iter != list.end(); iter++)
    this->Xor(*iter, toDelete);

  list.clear();
}

pqHistogramSelectionList &pqHistogramSelectionList::operator=(
    const pqHistogramSelectionList &list)
{
  this->clear();
  if(this->Data && list.Data)
    {
    this->Sorted = list.Sorted;
    vtkstd::list<pqHistogramSelection *>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      this->Data->push_back(*iter);
    }

  return *this;
}

pqHistogramSelectionList &pqHistogramSelectionList::operator+=(
    const pqHistogramSelectionList &list)
{
  if(this->Data && list.Data)
    {
    this->Sorted = false;
    vtkstd::list<pqHistogramSelection *>::iterator iter = list.Data->begin();
    for( ; iter != list.Data->end(); iter++)
      this->Data->push_back(*iter);
    }

  return *this;
}

bool pqHistogramSelectionList::preprocessItem(pqHistogramSelection *item,
    pqHistogramSelectionList &toDelete)
{
  if(!item)
    return false;
  else if(!this->Data)
    {
    toDelete.pushBack(item);
    return false;
    }

  // Make sure the list is sorted.
  if(!this->Sorted)
    this->sortAndMerge(toDelete);

  if(this->Type == pqHistogramSelection::None)
    this->Type = item->getType();
  else if(item->getType() != this->Type)
    {
    toDelete.pushBack(item);
    return false;
    }

  // Make sure the item is ordered correctly.
  if(item->Second < item->First)
    item->reverse();
  return true;
}


