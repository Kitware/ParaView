/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
 * \file pqChartZoomHistory.cxx
 *
 * \brief
 *   The pqChartZoomItem and pqChartZoomHistory classes are used to
 *   keep track of the user's zoom position(s).
 *
 * \author Mark Richardson
 * \date   June 10, 2005
 */

#include "pqChartZoomHistory.h"

#include <vtkstd/vector>
#include <vtkstd/algorithm>


/// \class pqChartZoomHistoryData
/// \brief
///   The pqChartZoomHistoryData class hides the private data of the
///   pqChartZoomHistory class.
class pqChartZoomHistoryData : public vtkstd::vector<pqChartZoomItem *> {};


pqChartZoomItem::pqChartZoomItem()
{
  this->X = 0;
  this->Y = 0;
  this->XPercent = 100;
  this->YPercent = 100;
}

void pqChartZoomItem::setPosition(int x, int y)
{
  this->X = x;
  this->Y = y;
}

void pqChartZoomItem::setZoom(int x, int y)
{
  this->XPercent = x;
  this->YPercent = y;
}


pqChartZoomHistory::pqChartZoomHistory()
{
  this->Data = new pqChartZoomHistoryData();
  this->Current = 0;
  this->Allowed = 10;

  if(this->Data)
    this->Data->reserve(this->Allowed);
}

pqChartZoomHistory::~pqChartZoomHistory()
{
  if(this->Data)
    {
    pqChartZoomHistoryData::iterator iter = this->Data->begin();
    for( ; iter != this->Data->end(); iter++)
      delete *iter;
    delete this->Data;
    }
}

void pqChartZoomHistory::setLimit(int limit)
{
  if(limit > 0)
    {
    this->Allowed = limit;
    if(this->Data)
      this->Data->reserve(this->Allowed);
    }
}

void pqChartZoomHistory::addHistory(int x, int y, int xZoom, int yZoom)
{
  if(this->Data)
    {
    pqChartZoomItem *zoom = new pqChartZoomItem();
    if(zoom)
      {
      zoom->setPosition(x, y);
      zoom->setZoom(xZoom, yZoom);

      // Remove history items after the current one.
      if(static_cast<int>(this->Data->size()) >= this->Allowed ||
          this->Current < static_cast<int>(this->Data->size()))
        {
        int front = static_cast<int>(this->Data->size()) - this->Allowed + 1;
        if(this->Current < this->Allowed - 1)
          front = 0;
        pqChartZoomHistoryData::iterator iter = Data->begin();
        for(int i = 0; iter != Data->end(); iter++, i++)
          {
          if(i < front || i > this->Current)
            {
            delete *iter;
            *iter = 0;
            }
          }

        Data->erase(vtkstd::remove(Data->begin(), Data->end(),
            static_cast<pqChartZoomItem *>(0)), Data->end());
        }

      // Add the zoom item to the end of the list and update
      // the current position.
      this->Data->push_back(zoom);
      this->Current = static_cast<int>(this->Data->size()) - 1;
      if(this->Current < 0)
        this->Current = 0;
      }
    }
}

void pqChartZoomHistory::updatePosition(int x, int y)
{
  if(this->Current < static_cast<int>(this->Data->size()))
    {
    pqChartZoomItem *zoom = (*this->Data)[this->Current];
    zoom->setPosition(x, y);
    }
}

const pqChartZoomItem *pqChartZoomHistory::getCurrent() const
{
  if(this->Current < static_cast<int>(this->Data->size()))
    return (*this->Data)[this->Current];
  return 0;
}

const pqChartZoomItem *pqChartZoomHistory::getPrevious()
{
  this->Current--;
  if(this->Current < 0)
    {
    this->Current = 0;
    return 0;
    }
  else
    return this->getCurrent();
}

const pqChartZoomItem *pqChartZoomHistory::getNext()
{
  this->Current++;
  if(this->Current < static_cast<int>(this->Data->size()))
    return getCurrent();
  else
    {
    if(this->Current > 0)
      this->Current--;
    return 0;
    }
}


