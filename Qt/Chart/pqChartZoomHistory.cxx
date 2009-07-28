/*=========================================================================

   Program: ParaView
   Module:    pqChartZoomHistory.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

/*!
 * \file pqChartZoomHistory.cxx
 *
 * \brief
 *   The pqChartZoomViewport and pqChartZoomHistory classes are used to
 *   keep track of the user's zoom position(s).
 *
 * \author Mark Richardson
 * \date   June 10, 2005
 */

#include "pqChartZoomHistory.h"

#include <QVector>


/// \class pqChartZoomHistoryInternal
/// \brief
///   The pqChartZoomHistoryInternal class hides the private data of
///   the pqChartZoomHistory class.
class pqChartZoomHistoryInternal : public QVector<pqChartZoomViewport *> {};


//----------------------------------------------------------------------------
pqChartZoomViewport::pqChartZoomViewport()
{
  this->X = 0;
  this->Y = 0;
  this->XPercent = 100;
  this->YPercent = 100;
}

void pqChartZoomViewport::setPosition(int x, int y)
{
  this->X = x;
  this->Y = y;
}

void pqChartZoomViewport::setZoom(int x, int y)
{
  this->XPercent = x;
  this->YPercent = y;
}


//----------------------------------------------------------------------------
pqChartZoomHistory::pqChartZoomHistory()
{
  this->Internal = new pqChartZoomHistoryInternal();
  this->Current = 0;
  this->Allowed = 10;

  // Reserve space for the history list.
  this->Internal->reserve(this->Allowed);
}

pqChartZoomHistory::~pqChartZoomHistory()
{
  QVector<pqChartZoomViewport *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); iter++)
    {
    delete *iter;
    }

  delete this->Internal;
}

void pqChartZoomHistory::setLimit(int limit)
{
  if(limit > 0)
    {
    this->Allowed = limit;
    this->Internal->reserve(this->Allowed);
    }
}

void pqChartZoomHistory::addHistory(int x, int y, int xZoom, int yZoom)
{
  pqChartZoomViewport *zoom = new pqChartZoomViewport();
  zoom->setPosition(x, y);
  zoom->setZoom(xZoom, yZoom);

  // Remove history items after the current one.
  if(this->Internal->size() >= this->Allowed ||
      this->Current < this->Internal->size() - 1)
    {
    int front = this->Internal->size() - this->Allowed + 1;
    if(this->Current < this->Allowed - 1)
      {
      front = 0;
      }

    QVector<pqChartZoomViewport *>::Iterator iter = this->Internal->begin();
    for(int i = 0; iter != this->Internal->end(); ++iter, ++i)
      {
      if(i < front || i > this->Current)
        {
        delete *iter;
        *iter = 0;
        }
      }

    // First, remove the empty items from the end.
    if(this->Current < this->Internal->size() - 1)
      {
      this->Internal->resize(this->Current + 1);
      }

    // Remove any empty items from the front of the list.
    if(front > 0)
      {
      this->Internal->remove(0, front);
      }
    }

  // Add the zoom item to the end of the list and update the current
  // position.
  this->Internal->append(zoom);
  this->Current = this->Internal->size() - 1;
}

void pqChartZoomHistory::updatePosition(int x, int y)
{
  if(this->Current < this->Internal->size())
    {
    pqChartZoomViewport *zoom = (*this->Internal)[this->Current];
    zoom->setPosition(x, y);
    }
}

bool pqChartZoomHistory::isPreviousAvailable() const
{
  return this->Current > 0;
}

bool pqChartZoomHistory::isNextAvailable() const
{
  return this->Current < this->Internal->size() - 1;
}

const pqChartZoomViewport *pqChartZoomHistory::getCurrent() const
{
  if(this->Current < this->Internal->size())
    {
    return (*this->Internal)[this->Current];
    }

  return 0;
}

const pqChartZoomViewport *pqChartZoomHistory::getPrevious()
{
  this->Current--;
  if(this->Current < 0)
    {
    this->Current = 0;
    return 0;
    }
  else
    {
    return this->getCurrent();
    }
}

const pqChartZoomViewport *pqChartZoomHistory::getNext()
{
  this->Current++;
  if(this->Current < this->Internal->size())
    {
    return this->getCurrent();
    }
  else
    {
    if(this->Current > 0)
      {
      this->Current--;
      }

    return 0;
    }
}


