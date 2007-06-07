/*=========================================================================

   Program: ParaView
   Module:    pqLineChartOptions.cxx

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

/// \file pqLineChartOptions.cxx
/// \date 4/26/2007

#include "pqLineChartOptions.h"

#include "pqChartSeriesOptionsGenerator.h"
#include "pqLineChartSeriesOptions.h"

#include <QBrush>
#include <QColor>
#include <QList>
#include <QMutableListIterator>
#include <QPen>


class pqLineChartOptionsInternal
{
public:
  pqLineChartOptionsInternal();
  ~pqLineChartOptionsInternal();

  pqChartSeriesOptionsGenerator *Generator;
  pqChartSeriesOptionsGenerator *DefaultGenerator;
  QList<pqLineChartSeriesOptions *> Options;
  QList<pqLineChartSeriesOptions *> Order;
  QList<int> EmptySpots;
};


//----------------------------------------------------------------------------
pqLineChartOptionsInternal::pqLineChartOptionsInternal()
  : Options(), Order(), EmptySpots()
{
  this->DefaultGenerator = new pqChartSeriesOptionsGenerator();
  this->Generator = this->DefaultGenerator;
}

pqLineChartOptionsInternal::~pqLineChartOptionsInternal()
{
  delete this->DefaultGenerator;
}


//----------------------------------------------------------------------------
pqLineChartOptions::pqLineChartOptions(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqLineChartOptionsInternal();
}

pqLineChartOptions::~pqLineChartOptions()
{
  // Note: The plot option objects will get cleaned up by Qt.
  delete this->Internal;
}

pqChartSeriesOptionsGenerator *pqLineChartOptions::getGenerator()
{
  return this->Internal->Generator;
}

void pqLineChartOptions::setGenerator(pqChartSeriesOptionsGenerator *generator)
{
  this->Internal->Generator = generator;
  if(this->Internal->Generator == 0)
    {
    this->Internal->Generator = this->Internal->DefaultGenerator;
    }
}

int pqLineChartOptions::getNumberOfSeriesOptions() const
{
  return this->Internal->Options.size();
}

pqLineChartSeriesOptions *pqLineChartOptions::getSeriesOptions(int index)
{
  if(index >= 0 && index < this->Internal->Options.size())
    {
    return this->Internal->Options[index];
    }

  return 0;
}

void pqLineChartOptions::setSeriesOptions(int index,
    const pqLineChartSeriesOptions &options)
{
  if(index >= 0 && index < this->Internal->Options.size())
    {
    *(this->Internal->Options[index]) = options;
    }
}

void pqLineChartOptions::clearSeriesOptions()
{
  QList<pqLineChartSeriesOptions *>::Iterator iter =
      this->Internal->Options.begin();
  for( ; iter != this->Internal->Options.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Options.clear();
  this->Internal->Order.clear();
  this->Internal->EmptySpots.clear();
}

void pqLineChartOptions::insertSeriesOptions(int first, int last)
{
  if(first < 0 || last < 0)
    {
    return;
    }

  if(last < first)
    {
    int temp = first;
    first = last;
    last = temp;
    }

  QPen pen;
  int start = first;
  pqLineChartSeriesOptions *options = 0;
  QList<pqLineChartSeriesOptions *> newOptions;
  QList<int>::Iterator iter = this->Internal->EmptySpots.begin();
  while(start <= last)
    {
    options = new pqLineChartSeriesOptions(this);
    this->Internal->Options.insert(start, options);
    newOptions.append(options);

    // Fill the empty spots first. Then, use the count to generate
    // the new options.
    if(iter != this->Internal->EmptySpots.end())
      {
      this->Internal->Generator->getSeriesPen(*iter, pen);
      this->Internal->Order[*iter] = options;
      iter = this->Internal->EmptySpots.erase(iter);
      }
    else
      {
      this->Internal->Generator->getSeriesPen(
          this->Internal->Order.size(), pen);
      this->Internal->Order.append(options);
      }

    options->setPen(0, pen);
    options->setBrush(0, QBrush(Qt::white));
    this->connect(options, SIGNAL(optionsChanged()),
        this, SIGNAL(optionsChanged()));

    start++;
    }

  // Signal observers to finish the plot option creation.
  QList<pqLineChartSeriesOptions *>::Iterator jter = newOptions.begin();
  for( ; jter != newOptions.end(); ++jter, ++first)
    {
    emit this->optionsInserted(first, *jter);
    }
}

void pqLineChartOptions::removeSeriesOptions(int first, int last)
{
  if(first < 0 || first >= this->Internal->Options.size() ||
      last < 0 || last >= this->Internal->Options.size())
    {
    return;
    }

  if(last < first)
    {
    int temp = first;
    first = last;
    last = temp;
    }

  // Remove the options from last to first.
  int index = 0;
  pqLineChartSeriesOptions *options = 0;
  for( ; last >= first; last--)
    {
    options = this->Internal->Options.takeAt(last);
    index = this->Internal->Order.indexOf(options);
    this->Internal->Order[index] = 0;
    this->Internal->EmptySpots.append(index);
    delete options;
    }

  // Clean up the end of the order list so the count can be reset.
  QMutableListIterator<pqLineChartSeriesOptions *> cleaner(
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

  int count = this->Internal->Order.size() - 1;
  QMutableListIterator<int> iter(this->Internal->EmptySpots);
  iter.toFront();
  while(iter.hasNext())
    {
    if(iter.next() > count)
      {
      iter.remove();
      }
    }
}

void pqLineChartOptions::moveSeriesOptions(int current, int index)
{
  if(current < 0 || current >= this->Internal->Options.size() ||
      index < 0 || index >= this->Internal->Options.size())
    {
    return;
    }

  // Remove the plot options from the list.
  pqLineChartSeriesOptions *options = this->Internal->Options.takeAt(current);

  // Adjust the index if it is after the current one.
  if(index > current)
    {
    index--;
    }

  if(index < this->Internal->Options.size())
    {
    this->Internal->Options.insert(index, options);
    }
  else
    {
    this->Internal->Options.append(options);
    }
}


