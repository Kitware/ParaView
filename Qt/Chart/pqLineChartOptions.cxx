/*=========================================================================

   Program: ParaView
   Module:    pqLineChartOptions.cxx

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

/// \file pqLineChartOptions.cxx
/// \date 4/26/2007

#include "pqLineChartOptions.h"

#include "pqChartSeriesColorManager.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqLineChartSeriesOptions.h"

#include <QBrush>
#include <QColor>
#include <QList>
#include <QPen>


class pqLineChartOptionsInternal
{
public:
  pqLineChartOptionsInternal();
  ~pqLineChartOptionsInternal();

  pqChartSeriesColorManager *Colors;
  pqChartSeriesColorManager *DefaultColors;
  QList<pqLineChartSeriesOptions *> Options;
};


//----------------------------------------------------------------------------
pqLineChartOptionsInternal::pqLineChartOptionsInternal()
  : Options()
{
  this->DefaultColors = new pqChartSeriesColorManager();
  this->Colors = this->DefaultColors;
}

pqLineChartOptionsInternal::~pqLineChartOptionsInternal()
{
  delete this->DefaultColors;
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

pqChartSeriesColorManager *pqLineChartOptions::getSeriesColorManager()
{
  return this->Internal->Colors;
}

void pqLineChartOptions::setSeriesColorManager(
    pqChartSeriesColorManager *manager)
{
  this->Internal->Colors = manager;
  if(this->Internal->Colors == 0)
    {
    this->Internal->Colors = this->Internal->DefaultColors;
    }
}

pqChartSeriesOptionsGenerator *pqLineChartOptions::getGenerator()
{
  return this->Internal->Colors->getGenerator();
}

void pqLineChartOptions::setGenerator(pqChartSeriesOptionsGenerator *generator)
{
  this->Internal->Colors->setGenerator(generator);
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
    this->Internal->Colors->removeSeriesOptions(*iter);
    delete *iter;
    }

  this->Internal->Options.clear();
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
  while(start <= last)
    {
    options = new pqLineChartSeriesOptions(this);
    this->Internal->Options.insert(start, options);
    newOptions.append(options);

    // Use the series color manager to set up the pen.
    int index = this->Internal->Colors->addSeriesOptions(options);
    this->getGenerator()->getSeriesPen(index, pen);

    // Set up the line series options.
    options->setPen(pen, 0);
    options->setBrush(QBrush(Qt::white), 0);
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
  pqLineChartSeriesOptions *options = 0;
  for( ; last >= first; last--)
    {
    options = this->Internal->Options.takeAt(last);
    this->Internal->Colors->removeSeriesOptions(options);
    delete options;
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


