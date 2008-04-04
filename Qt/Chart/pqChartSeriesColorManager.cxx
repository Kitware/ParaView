/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesColorManager.cxx

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

/// \file pqChartSeriesColorManager.cxx
/// \date 6/8/2007

#include "pqChartSeriesColorManager.h"

#include "pqChartSeriesOptionsGenerator.h"
#include <QList>
#include <QMutableListIterator>
#include <QObject>


class pqChartSeriesColorManagerInternal
{
public:
  pqChartSeriesColorManagerInternal();
  ~pqChartSeriesColorManagerInternal();

  pqChartSeriesOptionsGenerator *Generator;
  pqChartSeriesOptionsGenerator *DefaultGenerator;
  QList<const QObject *> Order;
  QList<int> EmptySpots;
};


//----------------------------------------------------------------------------
pqChartSeriesColorManagerInternal::pqChartSeriesColorManagerInternal()
  : Order(), EmptySpots()
{
  this->DefaultGenerator = new pqChartSeriesOptionsGenerator();
  this->Generator = this->DefaultGenerator;
}

pqChartSeriesColorManagerInternal::~pqChartSeriesColorManagerInternal()
{
  delete this->DefaultGenerator;
}


//----------------------------------------------------------------------------
pqChartSeriesColorManager::pqChartSeriesColorManager()
{
  this->Internal = new pqChartSeriesColorManagerInternal();
}

pqChartSeriesColorManager::~pqChartSeriesColorManager()
{
  delete this->Internal;
}

pqChartSeriesOptionsGenerator *pqChartSeriesColorManager::getGenerator()
{
  return this->Internal->Generator;
}

void pqChartSeriesColorManager::setGenerator(
    pqChartSeriesOptionsGenerator *generator)
{
  this->Internal->Generator = generator;
  if(this->Internal->Generator == 0)
    {
    this->Internal->Generator = this->Internal->DefaultGenerator;
    }
}

int pqChartSeriesColorManager::addSeriesOptions(const QObject *options)
{
  if(!options)
    {
    return -1;
    }

  // See if the options are already added.
  int index = this->Internal->Order.indexOf(options);
  if(index != -1)
    {
    return index;
    }

  // If there are empty spots, fill them first. Otherwise, add the
  // options to the end.
  if(this->Internal->EmptySpots.size() > 0)
    {
    index = this->Internal->EmptySpots.takeAt(0);
    this->Internal->Order[index] = options;
    }
  else
    {
    index = this->Internal->Order.size();
    this->Internal->Order.append(options);
    }

  return index;
}

void pqChartSeriesColorManager::removeSeriesOptions(const QObject *options)
{
  if(!options)
    {
    return;
    }

  int index = this->Internal->Order.indexOf(options);
  if(index == -1)
    {
    return;
    }

  if(index == this->Internal->Order.size() - 1)
    {
    // Remove the options from the end of the list.
    this->Internal->Order.removeLast();

    // Clean up the end of the order list if there are empty spots.
    QMutableListIterator<const QObject *> cleaner(
        this->Internal->Order);
    cleaner.toBack();
    while(cleaner.hasPrevious())
      {
      if(cleaner.previous())
        {
        break;
        }
      else
        {
        cleaner.remove();
        }
      }

    // Clean up the empty spot list.
    int count = this->Internal->Order.size() - 1;
    QList<int>::Iterator iter = this->Internal->EmptySpots.begin();
    for( ; iter != this->Internal->EmptySpots.end(); ++iter)
      {
      if(*iter > count)
        {
        this->Internal->EmptySpots.erase(iter,
            this->Internal->EmptySpots.end());
        break;
        }
      }
    }
  else
    {
    // Remove the reference to the options.
    this->Internal->Order[index] = 0;

    // Add the index to the empty spot list. Make sure the index is
    // added in order.
    bool found = false;
    QList<int>::Iterator iter = this->Internal->EmptySpots.begin();
    for( ; iter != this->Internal->EmptySpots.end(); ++iter)
      {
      if(index < *iter)
        {
        found = true;
        this->Internal->EmptySpots.insert(iter, index);
        break;
        }
      }

    if(!found)
      {
      this->Internal->EmptySpots.append(index);
      }
    }
}


