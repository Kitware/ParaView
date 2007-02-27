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

#include "vtkEventQtSlotConnect.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMGenericViewDisplayProxy.h"

#include <QColor>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QtDebug>

#include "pqVTKLineChartPlot.h"
#include "pqSMAdaptor.h"
#include "pqLineChartDisplay.h"

//-----------------------------------------------------------------------------
class pqVTKLineChartModelInternal
{
public:
  QList<QPointer<pqLineChartDisplay> > Displays;
  QList<pqVTKLineChartPlot*> Plots;
  vtkEventQtSlotConnect* VTKConnect;
  bool DisplaysChangedSinceLastUpdate;
};

//-----------------------------------------------------------------------------
pqVTKLineChartModel::pqVTKLineChartModel(QObject* p/*=0*/)
  : pqLineChartModel(p)
{
  this->Internal = new pqVTKLineChartModelInternal();
  this->Internal->VTKConnect = vtkEventQtSlotConnect::New();
  this->Internal->DisplaysChangedSinceLastUpdate = true;
}


//-----------------------------------------------------------------------------
pqVTKLineChartModel::~pqVTKLineChartModel()
{
  this->Internal->VTKConnect->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::markModified()
{
  this->Internal->DisplaysChangedSinceLastUpdate = true;
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
    if (!this->Internal->Displays.contains(display))
      {
      this->Internal->VTKConnect->Connect(display->getProxy(),
        vtkCommand::PropertyModifiedEvent, this, SLOT(markModified()));
      this->Internal->Displays.push_back(display);
      this->Internal->DisplaysChangedSinceLastUpdate = true;
      }
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::removeDisplay(pqDisplay* disp)
{
  pqLineChartDisplay* display = qobject_cast<pqLineChartDisplay*>(disp);
  if (display)
    {
    this->Internal->VTKConnect->Disconnect(display->getProxy(),
      vtkCommand::PropertyModifiedEvent, this, SLOT(markModified()));
    this->Internal->Displays.removeAll(display);
    this->Internal->DisplaysChangedSinceLastUpdate = true;
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::removeAllDisplays()
{
  this->Internal->VTKConnect->Disconnect();
  this->Internal->Displays.clear();
  this->Internal->DisplaysChangedSinceLastUpdate = true;
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::update()
{
  // If any of the displays were modified, we need to recreate the
  // pqVTKLineChartPlot objects since the array selections 
  // made by the user may have changed. Otherwise, we can use old
  // pqVTKLineChartPlot objects and let then update themselves
  // based of data changes.

  bool some_plot_changed = false;
  if (true || this->Internal->DisplaysChangedSinceLastUpdate)
    {
    // Remove all old plots.
    this->clearPlots();

    foreach (pqLineChartDisplay* display, this->Internal->Displays)
      {
      if (display->isVisible())
        {
        this->createPlotsForDisplay(display);
        }
      }
    some_plot_changed = true;
    }

  foreach (pqVTKLineChartPlot* plot, this->Internal->Plots)
    {
    plot->update();
    if (true)
      {
      some_plot_changed = true;
      }
    }

  this->Internal->DisplaysChangedSinceLastUpdate = false;

  if (some_plot_changed)
    {
    // This may be an overkill if none of the plots actually changed.
    emit this->plotsReset();
    }
}

//-----------------------------------------------------------------------------
void pqVTKLineChartModel::createPlotsForDisplay(pqLineChartDisplay* display)
{
  // We create  a pqVTKLineChartPlot for every array in the display
  // to be plotted.
  if (!display->getClientSideData())
    {
    qDebug() << "No client side data available.";
    return;
    }

  int num_y_arrays = display->getNumberOfYArrays();
  for (int cc=0; cc < num_y_arrays; cc++)
    {
    if (!display->getYArrayEnabled(cc))
      {
      continue;
      }

    pqVTKLineChartPlot* plot = new pqVTKLineChartPlot(this);
    plot->setXArray(display->getXArray());
    plot->setYArray(display->getYArray(cc));
    plot->setColor(display->getYColor(cc));

    this->Internal->Plots.push_back(plot);
    this->appendPlot(plot);
    }
}

