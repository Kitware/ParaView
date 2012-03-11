/*=========================================================================

   Program: ParaView
   Module:    pqPlotMatrixDisplayPanel.h

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

#include "pqPlotMatrixDisplayPanel.h"
#include "pqChartSeriesSettingsModel.h"
#include "pqDataRepresentation.h"
#include "pqSignalAdaptors.h"
#include "vtkSMProxy.h"

#include "ui_pqPlotMatrixDisplayPanel.h"

pqPlotMatrixDisplayPanel::pqPlotMatrixDisplayPanel(pqRepresentation *representation, QWidget *pWidget)
  : pqDisplayPanel(representation, pWidget)
{
  Ui::pqPlotMatrixDisplayPanel ui;
  ui.setupUi(this);

  this->SettingsModel = new pqChartSeriesSettingsModel(this);
  pqDataRepresentation* dispRep = qobject_cast<pqDataRepresentation*>(representation);
  this->SettingsModel->setRepresentation(dispRep);
  ui.Series->setModel(this->SettingsModel);
  ui.Series->setAcceptDrops(true);
  ui.Series->setDragEnabled(true);
  ui.Series->setDropIndicatorShown(true);
  ui.Series->setDragDropOverwriteMode(false);
  ui.Series->setDragDropMode(QAbstractItemView::InternalMove);

  vtkSMProxy *proxy = representation->getProxy();

  // add color buttons
  ui.ActivePlotColor->setChosenColor(Qt::black);
  ui.ScatterPlotsColor->setChosenColor(Qt::black);
  ui.HistogramColor->setChosenColor(Qt::black);

  this->ActivePlotColorAdaptor = new pqSignalAdaptorColor(ui.ActivePlotColor,
                                                          "chosenColor",
                                                          SIGNAL(chosenColorChanged(const QColor&)),
                                                          false);
  this->ScatterPlotsColorAdaptor = new pqSignalAdaptorColor(ui.ScatterPlotsColor,
                                                            "chosenColor",
                                                            SIGNAL(chosenColorChanged(const QColor&)),
                                                            false);
  this->HistogramColorAdaptor = new pqSignalAdaptorColor(ui.HistogramColor,
                                                         "chosenColor",
                                                         SIGNAL(chosenColorChanged(const QColor&)),
                                                         false);

  this->Links.addPropertyLink(this->ActivePlotColorAdaptor,
                              "color",
                              SIGNAL(colorChanged(QVariant)),
                              proxy,
                              proxy->GetProperty("ActivePlotColor"));
  this->Links.addPropertyLink(this->ScatterPlotsColorAdaptor,
                              "color",
                              SIGNAL(colorChanged(QVariant)),
                              proxy,
                              proxy->GetProperty("Color"));
  this->Links.addPropertyLink(this->HistogramColorAdaptor,
                              "color",
                              SIGNAL(colorChanged(QVariant)),
                              proxy,
                              proxy->GetProperty("HistogramColor"));

  this->Links.addPropertyLink(ui.ActivePlotMarkerSize,
                              "value",
                              SIGNAL(valueChanged(double)),
                              proxy,
                              proxy->GetProperty("ActivePlotMarkerSize"));
  this->Links.addPropertyLink(ui.ScatterPlotMarkerSize,
                              "value",
                              SIGNAL(valueChanged(double)),
                              proxy,
                              proxy->GetProperty("ScatterPlotMarkerSize"));

  this->ActivePlotMarkerStyleAdaptor = new pqSignalAdaptorComboBox(ui.ActivePlotMarkerStyle);
  this->ScatterPlotsMarkerStyleAdaptor = new pqSignalAdaptorComboBox(ui.ScatterPlotMarkerStyle);

  this->Links.addPropertyLink(this->ActivePlotMarkerStyleAdaptor,
                              "currentIndex",
                              SIGNAL(currentIndexChanged(int)),
                              proxy,
                              proxy->GetProperty("ActivePlotMarkerStyle"));
  this->Links.addPropertyLink(this->ScatterPlotsMarkerStyleAdaptor,
                              "currentIndex",
                              SIGNAL(currentIndexChanged(int)),
                              proxy,
                              proxy->GetProperty("ScatterPlotMarkerStyle"));

  QObject::connect(this->SettingsModel,
                   SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this,
                   SLOT(dataChanged(QModelIndex, QModelIndex)));
  QObject::connect(this->SettingsModel, SIGNAL(redrawChart()),
    this, SLOT(updateAllViews()));

  QObject::connect(dispRep, SIGNAL(dataUpdated()), this, SLOT(reloadSeries()));
  this->reloadSeries();
}

pqPlotMatrixDisplayPanel::~pqPlotMatrixDisplayPanel()
{
}

void pqPlotMatrixDisplayPanel::dataChanged(QModelIndex topLeft, QModelIndex bottomRight)
{
  Q_UNUSED(topLeft);
  Q_UNUSED(bottomRight);
  this->Representation->renderViewEventually();
}
//-----------------------------------------------------------------------------
void pqPlotMatrixDisplayPanel::reloadSeries()
{
  this->updateAllViews();
  this->SettingsModel->reload();
}
