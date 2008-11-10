/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewHistogram.cxx

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

/// \file pqPlotViewHistogram.cxx
/// \date 7/13/2007

#include "pqPlotViewHistogram.h"

#include "pqBarChartRepresentation.h"
#include "pqChartArea.h"
#include "pqHistogramChart.h"
#include "pqHistogramChartOptions.h"
#include "pqSMAdaptor.h"
#include "pqVTKHistogramColor.h"
#include "pqVTKHistogramModel.h"

#include <QList>
#include <QPointer>
#include <QtDebug>

#include "vtkSMProxy.h"
#include "vtkTimeStamp.h"


class pqPlotViewHistogramInternal
{
public:
  pqPlotViewHistogramInternal();
  ~pqPlotViewHistogramInternal() {}

  QPointer<pqHistogramChart> Layer;
  pqVTKHistogramModel *Model;
  pqVTKHistogramColor ColorScheme;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp MTime;
  QPointer<pqBarChartRepresentation> LastUsedRepresentation;
  QList<QPointer<pqBarChartRepresentation> > Representations;
};


//----------------------------------------------------------------------------
pqPlotViewHistogramInternal::pqPlotViewHistogramInternal()
  : Layer(0), ColorScheme(), LastUpdateTime(), MTime(),
    LastUsedRepresentation(0), Representations()
{
  this->Model = 0;
}


//----------------------------------------------------------------------------
pqPlotViewHistogram::pqPlotViewHistogram(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqPlotViewHistogramInternal();
}

//----------------------------------------------------------------------------
pqPlotViewHistogram::~pqPlotViewHistogram()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::initialize(pqChartArea *chartArea)
{
  if(this->Internal->Model)
    {
    return;
    }

  // Add a histogram layer to the chart.
  this->Internal->Layer = new pqHistogramChart(chartArea);
  chartArea->insertLayer(chartArea->getAxisLayerIndex(),
      this->Internal->Layer);

  // Set up the histogram model and options.
  this->Internal->Model = new pqVTKHistogramModel(this);
  this->Internal->ColorScheme.setModel(this->Internal->Model);
  this->Internal->Layer->getOptions()->setColorScheme(
      &this->Internal->ColorScheme);
  this->Internal->Layer->setModel(this->Internal->Model);
}

//----------------------------------------------------------------------------
pqHistogramChart *pqPlotViewHistogram::getChartLayer() const
{
  return this->Internal->Layer;
}

//----------------------------------------------------------------------------
pqBarChartRepresentation *pqPlotViewHistogram::getCurrentRepresentation() const
{
  QList<QPointer<pqBarChartRepresentation> >::ConstIterator display =
      this->Internal->Representations.begin();
  for( ; display != this->Internal->Representations.end(); ++display)
    {
    if(!display->isNull() && (*display)->isVisible())
      {
      return *display;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::setCurrentRepresentation(pqBarChartRepresentation *display)
{
  vtkSMProxy* lut = 0;
  if (display)
    {
    lut = pqSMAdaptor::getProxyProperty(
      display->getProxy()->GetProperty("LookupTable"));
    if (!lut)
      {
      // Update the lookup table.
      display->updateLookupTable();
      lut = pqSMAdaptor::getProxyProperty(
        display->getProxy()->GetProperty("LookupTable"));
      }
    }

  this->Internal->ColorScheme.setMapIndexToColor(true);
  this->Internal->ColorScheme.setScalarsToColors(lut);
  if (this->Internal->LastUsedRepresentation == display)
    {
    return;
    }

  this->Internal->LastUsedRepresentation = display;
  this->Internal->MTime.Modified();
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::update(bool force)
{
  this->setCurrentRepresentation(this->getCurrentRepresentation());

  if(this->Internal->Model && (force || this->isUpdateNeeded()))
    {
    vtkDataArray *xarray = 0;
    vtkDataArray *yarray = 0;
    int xcomp = -1;
    int ycomp = -1;
    if(!this->Internal->LastUsedRepresentation.isNull())
      {
      xarray = this->Internal->LastUsedRepresentation->getXArray();
      yarray = this->Internal->LastUsedRepresentation->getYArray();
      xcomp = this->Internal->LastUsedRepresentation->getXArrayComponent();
      ycomp = this->Internal->LastUsedRepresentation->getYArrayComponent();
      if(!xarray || !yarray)
        {
        qCritical() << "Failed to locate the data to plot on either axes.";
        }
      }

    this->Internal->LastUpdateTime.Modified();
    this->Internal->Model->setDataArrays(xarray, xcomp, yarray, ycomp);
    }
}

//----------------------------------------------------------------------------
bool pqPlotViewHistogram::isUpdateNeeded()
{
  bool force = true; //FIXME: until we fix thses conditions to include LUT.

  /*
  // We try to determine if we really need to update the GUI.

  // If the model has been modified since last update.
  force |= this->Internal->MTime > this->Internal->LastUpdateTime;

  // if the display has been modified since last update.
  force |= (this->Internal->LastUsedRepresentation) && 
    (this->Internal->LastUsedRepresentation->getProxy()->GetMTime() > 
     this->Internal->LastUpdateTime); 

  // if the data object obtained from the display has been modified 
  // since last update.
  vtkRectilinearGrid* data = this->Internal->LastUsedRepresentation ?
    this->Internal->LastUsedRepresentation->getClientSideData() : 0; 
  force |= (data) && (data->GetMTime() > this->Internal->LastUpdateTime);
  */

  return force;
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::addRepresentation(
    pqBarChartRepresentation *histogram)
{
  if(histogram && !this->Internal->Representations.contains(histogram))
    {
    this->Internal->Representations.append(histogram);
    }
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::removeRepresentation(
    pqBarChartRepresentation *histogram)
{
  if(histogram)
    {
    this->Internal->Representations.removeAll(histogram);
    if(histogram == this->Internal->LastUsedRepresentation)
      {
      this->setCurrentRepresentation(0);
      }
    }
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::removeAllRepresentations()
{
  this->Internal->LastUsedRepresentation = 0;
  this->Internal->Representations.clear();
  this->Internal->MTime.Modified();
}

//----------------------------------------------------------------------------
void pqPlotViewHistogram::updateVisibility(pqRepresentation* display)
{
  pqBarChartRepresentation *histogram =
      qobject_cast<pqBarChartRepresentation *>(display);
  if(histogram && histogram->isVisible())
    {
    // There can be only one visible data set for the histogram.
    QList<QPointer<pqBarChartRepresentation> >::Iterator iter =
        this->Internal->Representations.begin();
    for( ; iter != this->Internal->Representations.end(); ++iter)
      {
      if(*iter != histogram && (*iter)->isVisible())
        {
        (*iter)->setVisible(false);
        }
      }
    }
}


