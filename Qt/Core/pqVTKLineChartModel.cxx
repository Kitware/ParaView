/*=========================================================================

   Program: ParaView
   Module:    pqVTKLineChartModel.cxx

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
#include "pqVTKLineChartModel.h"

#include "vtkRectilinearGrid.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkPointData.h"

#include <QtDebug>
#include <QList>
#include <QMap>

#include "pqVTKLineChartPlot.h"
#include "pqSMAdaptor.h"
#include "pqDisplay.h"

//-----------------------------------------------------------------------------
class pqVTKLineChartModelInternal
{
public:
  typedef QMap<pqDisplay*, pqVTKLineChartPlot*> MapOfDataSetToPlot;
  MapOfDataSetToPlot PlotMap;
};

//-----------------------------------------------------------------------------
pqVTKLineChartModel::pqVTKLineChartModel(QObject* p/*=0*/)
  : pqLineChartModel(p)
{
  this->Internal = new pqVTKLineChartModelInternal();
}

//-----------------------------------------------------------------------------
pqVTKLineChartModel::~pqVTKLineChartModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::clearPlots()
{
  int max = this->getNumberOfPlots();
  QList<pqLineChartPlot*> plots;
  for(int cc=0; cc < max; cc++)
    {
    plots.push_back(const_cast<pqLineChartPlot*>(this->getPlot(cc)));
    }
  this->Superclass::clearPlots();
  foreach(pqLineChartPlot* plot, plots)
    {
    delete plot;
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::update(QList<pqDisplay*>& visibleDisplays)
{
  // Remove all old plots.
  this->clearPlots();

  pqVTKLineChartModelInternal::MapOfDataSetToPlot newMap;
  foreach (pqDisplay* display, visibleDisplays)
    {
    this->createPlotsForDisplay(display);
    }
  this->update();
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::createPlotsForDisplay(pqDisplay* display)
{
  // We create  a pqVTKLineChartPlot for every array in the display
  // to be plotted.
  vtkSMGenericViewDisplayProxy* proxy = 
    vtkSMGenericViewDisplayProxy::SafeDownCast(display->getProxy());
  if (!proxy)
    {
    qDebug() << "Display is not a vtkSMGenericViewDisplayProxy.";
    return;
    }

  vtkRectilinearGrid* dataset = vtkRectilinearGrid::SafeDownCast(proxy->GetOutput());
  if (!dataset)
    {
    qDebug() << "No client side data available.";
    return;
    }

  QString xaxisarray = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("XArrayName")).toString();
  int xaxismode = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("XAxisMode")).toInt();

  QList<QVariant> arraynames  = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty("YArrayNames"));

  foreach(QVariant vname, arraynames)
    {
    if (vname.toString() != "")
      {
      pqVTKLineChartPlot* plot = new pqVTKLineChartPlot(dataset, this);
      plot->setYArray(vname.toString());
      plot->setXArray(xaxisarray);
      plot->setXAxisMode(xaxismode);
      this->appendPlot(plot);
      }
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::update()
{
  foreach (pqVTKLineChartPlot* plot, this->Internal->PlotMap)
    {
    plot->update();
    }
  // This may be an overkill if none of the plots actually changed.
  emit this->plotsReset();
}

//-----------------------------------------------------------------------------
