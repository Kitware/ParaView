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

#include "vtkSMProxy.h"

#include <QMap>
#include <QtDebug>

#include "pqVTKLineChartPlot.h"
#include "pqLineChartDisplay.h"

//-----------------------------------------------------------------------------
class pqVTKLineChartModelInternal
{
public:
  QMap<pqLineChartDisplay*, pqVTKLineChartPlot*> Plots;
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
  this->Superclass::clearPlots();

  foreach(pqVTKLineChartPlot* plot, this->Internal->Plots)
    {
    delete plot;
    }

  this->Internal->Plots.clear();
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::addDisplay(pqDisplay* disp)
{
  pqLineChartDisplay* display = qobject_cast<pqLineChartDisplay*>(disp);
  if (display && display->getProxy()->GetXMLName() == QString("XYPlotDisplay2"))
    {
    if (!this->Internal->Plots.contains(display))
      {
      pqVTKLineChartPlot* plot = new pqVTKLineChartPlot(display, this);
      this->Internal->Plots[display] = plot;
      }
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::removeDisplay(pqDisplay* disp)
{
  pqLineChartDisplay* display = qobject_cast<pqLineChartDisplay*>(disp);
  if (display)
    {
    if (this->Internal->Plots.contains(display))
      {
      pqVTKLineChartPlot* plot = this->Internal->Plots.take(display);
      this->removePlot(plot);
      delete plot;
      }
    }
}


//-----------------------------------------------------------------------------
void pqVTKLineChartModel::update()
{
  // Add visible plots not already added to view
  // while remove those that are no longer visible but were 
  // previously added to the view.

  QMap<pqLineChartDisplay*, pqVTKLineChartPlot*>::iterator iter;
  for (iter = this->Internal->Plots.begin();
    iter != this->Internal->Plots.end(); ++iter)
    {
    bool isVisible = (iter.key()->isVisible() && iter.value()->getNumberOfSeries() > 0);
    if (isVisible && this->getIndexOf(iter.value()) < 0)
      {
      this->appendPlot(iter.value());
      }

    if (!isVisible && this->getIndexOf(iter.value()) >= 0)
      {
      this->removePlot(iter.value());
      }

    if (isVisible)
      {
      // update all visible views.
      iter.value()->update();
      }
    }
}

