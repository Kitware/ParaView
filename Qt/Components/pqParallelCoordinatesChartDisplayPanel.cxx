/*=========================================================================

   Program: ParaView
   Module:    pqParallelCoordinatesChartDisplayPanel.cxx

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
#include "pqParallelCoordinatesChartDisplayPanel.h"
#include "ui_pqParallelCoordinatesChartDisplayPanel.h"

#include "vtkSMParallelCoordinatesRepresentationProxy.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkChart.h"
#include "vtkWeakPointer.h"

#include <QColorDialog>
#include <QHeaderView>
#include <QList>
#include <QPointer>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QDebug>

#include "pqDataInformationModel.h"
#include "pqComboBoxDomain.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqView.h"
#include "pqDataRepresentation.h"
#include "pqParallelCoordinatesSettingsModel.h"

#include <assert.h>

//-----------------------------------------------------------------------------
class pqParallelCoordinatesChartDisplayPanel::pqInternal
  : public Ui::pqParallelCoordinatesChartDisplayPanel
{
public:
  pqInternal()
    {
    this->SettingsModel = 0;
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    this->CompositeIndexAdaptor = 0;
    }

  ~pqInternal()
    {
    delete this->SettingsModel;
    delete this->XAxisArrayDomain;
    delete this->XAxisArrayAdaptor;
    delete this->CompositeIndexAdaptor;
    }

  vtkWeakPointer<vtkSMParallelCoordinatesRepresentationProxy> ChartRepresentation;
  pqParallelCoordinatesSettingsModel* SettingsModel;
  pqComboBoxDomain* XAxisArrayDomain;
  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqPropertyLinks Links;
  pqSignalAdaptorCompositeTreeWidget* CompositeIndexAdaptor;

  bool InChange;
};

//-----------------------------------------------------------------------------
pqParallelCoordinatesChartDisplayPanel::pqParallelCoordinatesChartDisplayPanel(
  pqRepresentation* display,QWidget* p)
: pqDisplayPanel(display, p)
{
  this->Internal = new pqParallelCoordinatesChartDisplayPanel::pqInternal();
  this->Internal->setupUi(this);

  this->Internal->SettingsModel = new pqParallelCoordinatesSettingsModel(this);
  this->Internal->SeriesList->setModel(this->Internal->SettingsModel);

  QObject::connect(
    this->Internal->SeriesList, SIGNAL(activated(const QModelIndex &)),
    this, SLOT(activateItem(const QModelIndex &)));
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  QObject::connect(model,
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    this, SLOT(updateOptionsWidgets()));
  QObject::connect(model,
    SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    this, SLOT(updateOptionsWidgets()));
  QObject::connect(this->Internal->SettingsModel, SIGNAL(modelReset()),
    this, SLOT(updateOptionsWidgets()));
  QObject::connect(this->Internal->SettingsModel, SIGNAL(redrawChart()),
    this, SLOT(updateAllViews()));

  QObject::connect(
    this->Internal->ColorButton, SIGNAL(chosenColorChanged(const QColor &)),
    this, SLOT(setSeriesColor(const QColor &)));
  QObject::connect(this->Internal->Opacity, SIGNAL(valueChanged(double)),
    this, SLOT(setSeriesOpacity(double)));
  QObject::connect(this->Internal->Thickness, SIGNAL(valueChanged(int)),
    this, SLOT(setSeriesThickness(int)));
  QObject::connect(this->Internal->StyleList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setSeriesStyle(int)));

  this->setDisplay(display);

  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
                   this, SLOT(reloadSeries()), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqParallelCoordinatesChartDisplayPanel::~pqParallelCoordinatesChartDisplayPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::reloadSeries()
{
  this->updateAllViews();
  this->updateOptionsWidgets();
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::setDisplay(pqRepresentation* disp)
{
  this->setEnabled(false);

  vtkSMParallelCoordinatesRepresentationProxy* proxy =
    vtkSMParallelCoordinatesRepresentationProxy::SafeDownCast(disp->getProxy());
  this->Internal->ChartRepresentation = proxy;
  if (!this->Internal->ChartRepresentation)
    {
    qWarning() << "pqParallelCoordinatesChartDisplayPanel given a representation proxy "
                  "that is not an XYChartRepresentation. Cannot edit.";
    return;
    }

  // this is essential to ensure that when you undo-redo, the representation is
  // indeed update-to-date, thus ensuring correct domains etc.
  proxy->UpdatePipeline();

  // The model for the plot settings
  this->Internal->SettingsModel->setRepresentation(
      qobject_cast<pqDataRepresentation*>(disp));

  // Set up the CompositeIndexAdaptor
  this->Internal->CompositeIndexAdaptor = new pqSignalAdaptorCompositeTreeWidget(
    this->Internal->CompositeIndex,
    vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("CompositeDataSetIndex")),
    /*autoUpdateVisibility=*/true);

  this->Internal->Links.addPropertyLink(this->Internal->CompositeIndexAdaptor,
    "values", SIGNAL(valuesChanged()),
    proxy, proxy->GetProperty("CompositeDataSetIndex"));

  this->setEnabled(true);

  this->reloadSeries();
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::activateItem(const QModelIndex&)
{
  if(!this->Internal->ChartRepresentation)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  // Get current color
  //QColor color = this->Internal->SettingsModel->getSeriesColor(index.row());

  // Show color selector dialog to get a new color
  QColor color = QColorDialog::getColor(QColor(Qt::black), this);
  if (color.isValid())
    {
    // Set the new color
    QList<QVariant> values;
    values.append(QVariant(static_cast<double>(color.redF())));
    values.append(QVariant(static_cast<double>(color.greenF())));
    values.append(QVariant(static_cast<double>(color.blueF())));
    pqSMAdaptor::setMultipleElementProperty(
        this->Internal->ChartRepresentation->GetProperty("Color"), values);
    this->Internal->ChartRepresentation->UpdateVTKObjects();

    this->Internal->ColorButton->blockSignals(true);
    this->Internal->ColorButton->setChosenColor(color);
    this->Internal->ColorButton->blockSignals(false);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::updateOptionsWidgets()
{

}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::setSeriesColor(const QColor &color)
{
  if (color.isValid())
    {
    // Set the new color
    QList<QVariant> values;
    values.append(QVariant(static_cast<double>(color.redF())));
    values.append(QVariant(static_cast<double>(color.greenF())));
    values.append(QVariant(static_cast<double>(color.blueF())));
    pqSMAdaptor::setMultipleElementProperty(
        this->Internal->ChartRepresentation->GetProperty("Color"), values);
    this->Internal->ChartRepresentation->UpdateVTKObjects();

    this->Internal->ColorButton->blockSignals(true);
    this->Internal->ColorButton->setChosenColor(color);
    this->Internal->ColorButton->blockSignals(false);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::setSeriesOpacity(double opacity)
{
  pqSMAdaptor::setElementProperty(
      this->Internal->ChartRepresentation->GetProperty("Opacity"), opacity);
  this->Internal->ChartRepresentation->UpdateVTKObjects();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::setSeriesThickness(int thickness)
{
  pqSMAdaptor::setElementProperty(
      this->Internal->ChartRepresentation->GetProperty("LineThickness"), thickness);
  this->Internal->ChartRepresentation->UpdateVTKObjects();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesChartDisplayPanel::setSeriesStyle(int lineStyle)
{
  pqSMAdaptor::setElementProperty(
      this->Internal->ChartRepresentation->GetProperty("LineStyle"), lineStyle);
  this->Internal->ChartRepresentation->UpdateVTKObjects();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
Qt::CheckState pqParallelCoordinatesChartDisplayPanel::getEnabledState() const
{
  Qt::CheckState enabledState = Qt::Unchecked;

  return enabledState;
}
