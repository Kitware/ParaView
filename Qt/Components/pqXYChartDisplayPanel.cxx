/*=========================================================================

   Program: ParaView
   Module:    pqXYChartDisplayPanel.cxx

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
#include "pqXYChartDisplayPanel.h"
#include "ui_pqXYChartDisplayPanel.h"

#include "vtkChart.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSmartPointer.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkChartRepresentation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkTable.h"
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
#include "pqPlotSettingsModel.h"

#include <assert.h>

//-----------------------------------------------------------------------------
class pqXYChartDisplayPanel::pqInternal : public Ui::pqXYChartDisplayPanel
{
public:
  pqInternal()
    {
    this->SettingsModel = 0;
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    this->CompositeIndexAdaptor = 0;
    this->AttributeModeAdaptor = 0;
    }

  ~pqInternal()
    {
    delete this->SettingsModel;
    delete this->XAxisArrayDomain;
    delete this->XAxisArrayAdaptor;
    delete this->CompositeIndexAdaptor;
    delete this->AttributeModeAdaptor;
    }

  vtkWeakPointer<vtkSMChartRepresentationProxy> ChartRepresentation;
  pqPlotSettingsModel* SettingsModel;
  pqComboBoxDomain* XAxisArrayDomain;
  pqSignalAdaptorComboBox* AttributeModeAdaptor;
  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqPropertyLinks Links;
  pqSignalAdaptorCompositeTreeWidget* CompositeIndexAdaptor;

  bool InChange;
};

//-----------------------------------------------------------------------------
pqXYChartDisplayPanel::pqXYChartDisplayPanel(
  pqRepresentation* display,QWidget* p)
: pqDisplayPanel(display, p)
{
  this->Internal = new pqXYChartDisplayPanel::pqInternal();
  this->Internal->setupUi(this);

  this->Internal->SettingsModel = new pqPlotSettingsModel(this);
  this->Internal->SeriesList->setModel(this->Internal->SettingsModel);
  QObject::connect(this->Internal->SeriesList->header(),
    SIGNAL(checkStateChanged()),
    this, SLOT(headerCheckStateChanged()));

  this->Internal->XAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XAxisArray);
  this->Internal->AttributeModeAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->AttributeMode);

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
  QObject::connect(this->Internal->XAxisArray, SIGNAL(currentIndexChanged(int)),
    this, SLOT(updateAllViews()));
  QObject::connect(this->Internal->SettingsModel, SIGNAL(rescaleChart()),
    this, SLOT(rescaleChart()));

  QObject::connect(this->Internal->UseArrayIndex, SIGNAL(toggled(bool)),
    this, SLOT(useArrayIndexToggled(bool)));
  QObject::connect(this->Internal->UseDataArray, SIGNAL(toggled(bool)),
    this, SLOT(useDataArrayToggled(bool)));

  QObject::connect(
    this->Internal->ColorButton, SIGNAL(chosenColorChanged(const QColor &)),
    this, SLOT(setCurrentSeriesColor(const QColor &)));
  QObject::connect(this->Internal->Thickness, SIGNAL(valueChanged(int)),
    this, SLOT(setCurrentSeriesThickness(int)));
  QObject::connect(this->Internal->StyleList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesStyle(int)));
  QObject::connect(this->Internal->AxisList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesAxes(int)));
  QObject::connect(this->Internal->MarkerStyleList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesMarkerStyle(int)));

  this->setDisplay(display);

  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
                   this, SLOT(reloadSeries()), Qt::QueuedConnection);
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
                   this->Internal->SettingsModel, SLOT(reload()));
}

//-----------------------------------------------------------------------------
pqXYChartDisplayPanel::~pqXYChartDisplayPanel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::reloadSeries()
{
  vtkSMChartRepresentationProxy* proxy = this->Internal->ChartRepresentation;
  proxy->UpdatePropertyInformation();

  this->updateAllViews();
  this->updateOptionsWidgets();
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::rescaleChart()
{
  vtkSMChartRepresentationProxy* proxy = this->Internal->ChartRepresentation;
  if (proxy)
    {
    proxy->GetRepresentation()->RescaleChart();
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::setDisplay(pqRepresentation* disp)
{
  this->setEnabled(false);

  vtkSMChartRepresentationProxy* proxy =
    vtkSMChartRepresentationProxy::SafeDownCast(disp->getProxy());
  this->Internal->ChartRepresentation = proxy;
  if (!this->Internal->ChartRepresentation)
    {
    qWarning() << "pqXYChartDisplayPanel given a representation proxy "
                  "that is not an XYChartRepresentation. Cannot edit.";
    return;
    }

  // this is essential to ensure that when you undo-redo, the representation is
  // indeed update-to-date, thus ensuring correct domains etc.
  // proxy->UpdatePipeline();

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

  // Connect to the new properties.pqComboBoxDomain will ensure that
  // when ever the domain changes the widget is updated as well.
  this->Internal->XAxisArrayDomain = new pqComboBoxDomain(
      this->Internal->XAxisArray, proxy->GetProperty("XArrayName"));
  this->Internal->Links.addPropertyLink(this->Internal->XAxisArrayAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      proxy, proxy->GetProperty("XArrayName"));

  // Link to set whether the index is used for the x axis
  this->Internal->Links.addPropertyLink(
    this->Internal->UseArrayIndex, "checked",
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseIndexForXAxis"));

  this->Internal->Links.addPropertyLink(this->Internal->AttributeModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("AttributeType"));

  this->changeDialog(disp);

  this->setEnabled(true);

  QObject::connect(disp, SIGNAL(dataUpdated()),
    this, SLOT(reloadSeries()));

  this->reloadSeries();
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::changeDialog(pqRepresentation* disp)
{
  vtkSMChartRepresentationProxy* proxy =
    vtkSMChartRepresentationProxy::SafeDownCast(disp->getProxy());
  bool visible = true;
  if (QString("Bar") == vtkSMPropertyHelper(proxy,"ChartType").GetAsString())
    {
    visible = false;
    }
  this->Internal->Thickness->setVisible(visible);
  this->Internal->ThicknessLabel->setVisible(visible);
  this->Internal->StyleList->setVisible(visible);
  this->Internal->StyleListLabel->setVisible(visible);
  this->Internal->MarkerStyleList->setVisible(visible);
  this->Internal->MarkerStyleListLabel->setVisible(visible);
  this->Internal->AxisList->setVisible(visible);
  this->Internal->AxisListLabel->setVisible(visible);
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::activateItem(const QModelIndex &index)
{
  if(!this->Internal->ChartRepresentation
      || !index.isValid() || index.column() != 1)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  // Get current color
  QColor color = this->Internal->SettingsModel->getSeriesColor(index.row());

  // Show color selector dialog to get a new color
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
    // Set the new color
    this->Internal->SettingsModel->setSeriesColor(index.row(), color);
    this->Internal->ColorButton->blockSignals(true);
    this->Internal->ColorButton->setChosenColor(color);
    this->Internal->ColorButton->blockSignals(false);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::updateOptionsWidgets()
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    // Show the options for the current item.
    QModelIndex current = model->currentIndex();
    QModelIndexList indexes = model->selectedIndexes();
    if((!current.isValid() || !model->isSelected(current)) &&
        indexes.size() > 0)
      {
      current = indexes.last();
      }

    this->Internal->ColorButton->blockSignals(true);
    this->Internal->Thickness->blockSignals(true);
    this->Internal->StyleList->blockSignals(true);
    this->Internal->MarkerStyleList->blockSignals(true);
    this->Internal->AxisList->blockSignals(true);
    if (current.isValid())
      {
      int seriesIndex = current.row();
      QColor color = this->Internal->SettingsModel->getSeriesColor(seriesIndex);
      this->Internal->ColorButton->setChosenColor(color);
      this->Internal->Thickness->setValue(
        this->Internal->SettingsModel->getSeriesThickness(seriesIndex));
      this->Internal->StyleList->setCurrentIndex(
        this->Internal->SettingsModel->getSeriesStyle(seriesIndex));
      this->Internal->MarkerStyleList->setCurrentIndex(
        this->Internal->SettingsModel->getSeriesMarkerStyle(seriesIndex));
      this->Internal->AxisList->setCurrentIndex(
        this->Internal->SettingsModel->getSeriesAxisCorner(seriesIndex));
      }
    else
      {
      this->Internal->ColorButton->setChosenColor(Qt::white);
      this->Internal->Thickness->setValue(1);
      this->Internal->StyleList->setCurrentIndex(0);
      this->Internal->MarkerStyleList->setCurrentIndex(0);
      this->Internal->AxisList->setCurrentIndex(0);
      }

    this->Internal->ColorButton->blockSignals(false);
    this->Internal->Thickness->blockSignals(false);
    this->Internal->StyleList->blockSignals(false);
    this->Internal->MarkerStyleList->blockSignals(false);
    this->Internal->AxisList->blockSignals(false);

    // Disable the widgets if nothing is selected or current.
    bool hasItems = indexes.size() > 0;
    this->Internal->ColorButton->setEnabled(hasItems);
    this->Internal->Thickness->setEnabled(hasItems);
    this->Internal->StyleList->setEnabled(hasItems);
    this->Internal->MarkerStyleList->setEnabled(hasItems);
    this->Internal->AxisList->setEnabled(hasItems);
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::setCurrentSeriesColor(const QColor &color)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if(model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->SettingsModel->setSeriesColor(iter->row(), color);
      }
    this->Internal->InChange = false;
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::setCurrentSeriesThickness(int thickness)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if (model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->SettingsModel->setSeriesThickness(iter->row(), thickness);
      }
    this->Internal->InChange = false;
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::setCurrentSeriesStyle(int style)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if (model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->SettingsModel->setSeriesStyle(iter->row(), style);
      }
    this->Internal->InChange = false;
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::setCurrentSeriesMarkerStyle(int style)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if (model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->SettingsModel->setSeriesMarkerStyle(iter->row(), style);
      }
    this->Internal->InChange = false;
    }
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::setCurrentSeriesAxes(int value)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  if (model)
    {
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->SettingsModel->setSeriesAxisCorner(iter->row(), value);
      }
    this->Internal->InChange = false;
    }
}

//-----------------------------------------------------------------------------
Qt::CheckState pqXYChartDisplayPanel::getEnabledState() const
{
  Qt::CheckState enabledState = Qt::Unchecked;

  return enabledState;
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::useArrayIndexToggled(bool toggle)
{
  this->Internal->UseDataArray->setChecked(!toggle);
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::useDataArrayToggled(bool toggle)
{
  this->Internal->UseArrayIndex->setChecked(!toggle);
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqXYChartDisplayPanel::headerCheckStateChanged()
{
  // get current check state/
  QHeaderView* header = this->Internal->SeriesList->header();
  QAbstractItemModel* headerModel = header->model();
  bool checkable = false;
  int cs = headerModel->headerData(
    0, header->orientation(), Qt::CheckStateRole).toInt(&checkable);
  if (checkable)
    {
    if (cs ==  Qt::Checked)
      {
      cs = Qt::Unchecked;
      }
    else
      {
      cs = Qt::Checked;
      }
    headerModel->setHeaderData(0, header->orientation(), cs, Qt::CheckStateRole);
    }
}
