/*=========================================================================

   Program: ParaView
   Module:    pqBarChartDisplayPanel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqBarChartDisplayPanel.h"
#include "ui_pqBarChartDisplayPanel.h"

#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"

#include <QPointer>
#include <QDebug>
#include <QColorDialog>

#include "pqChartSeriesEditorModel.h"
#include "pqComboBoxDomain.h"
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqSignalAdaptors.h"

//-----------------------------------------------------------------------------
class pqBarChartDisplayPanel::pqInternal : public Ui::pqBarChartDisplayPanel
{
public:
  pqInternal()
    {
    this->Model = 0;
    }

  ~pqInternal()
    {
    delete this->Model;
    this->Model = 0;
    }

  pqPropertyLinks Links;
  vtkWeakPointer<vtkSMChartRepresentationProxy> ChartRepresentation;
  pqChartSeriesEditorModel *Model;
};

//-----------------------------------------------------------------------------
pqBarChartDisplayPanel::pqBarChartDisplayPanel(pqRepresentation* repr,
  QWidget* parentObject): Superclass(repr, parentObject), Internal(0)
{
  vtkSMChartRepresentationProxy* proxy =
    vtkSMChartRepresentationProxy::SafeDownCast(repr->getProxy());
  if (!proxy)
    {
    this->setEnabled(false);
    qCritical() << "pqBarChartDisplayPanel "
      "can only work with vtkSMChartRepresentationProxy";
    return;
    }

  // this is essential to ensure that when you undo-redo, the representation is
  // indeed update-to-date, thus ensuring correct domains etc.
  proxy->Update();

  this->Internal = new pqBarChartDisplayPanel::pqInternal();
  this->Internal->setupUi(this);

  // Create the model for showing the list of series available.
  this->Internal->Model = new pqChartSeriesEditorModel(this);
  this->Internal->SeriesList->setModel(this->Internal->Model);
  // Give the representation to our series editor model
  this->Internal->Model->setRepresentation(
    qobject_cast<pqDataRepresentation*>(repr));

  QObject::connect(this->Internal->UseArrayIndex, SIGNAL(toggled(bool)), 
    this, SLOT(useArrayIndexToggled(bool)));
  QObject::connect(this->Internal->UseDataArray, SIGNAL(toggled(bool)), 
    this, SLOT(useDataArrayToggled(bool)));

  /// Setup property links.

  // Connect ViewData checkbox to the proxy's Visibility property
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  // Connect the X axis array
  this->Internal->Links.addPropertyLink(
    this->Internal->UseArrayIndex, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseIndexForXAxis"));
  pqSignalAdaptorComboBox* xAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XAxisArray);
  pqComboBoxDomain* xAxisArrayDomain = new pqComboBoxDomain(
    this->Internal->XAxisArray, proxy->GetProperty("XArrayName"));
  xAxisArrayDomain->forceDomainChanged(); // init list
  this->Internal->Links.addPropertyLink(
    xAxisArrayAdaptor, "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("XArrayName")); 

  // Connect the AttributeType
  pqSignalAdaptorComboBox* attributeModeAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->AttributeMode);
  this->Internal->Links.addPropertyLink(
    attributeModeAdaptor, "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("AttributeType"));

  // Set up the CompositeIndexAdaptor 
  pqSignalAdaptorCompositeTreeWidget* compositeIndexAdaptor =
    new pqSignalAdaptorCompositeTreeWidget(
    this->Internal->CompositeIndex, 
    vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("CompositeDataSetIndex")),
    /*autoUpdateVisibility=*/true);
  this->Internal->Links.addPropertyLink(
    compositeIndexAdaptor, "values", SIGNAL(valuesChanged()),
    proxy, proxy->GetProperty("CompositeDataSetIndex"));

  // Request a render when any GUI widget is changed by the user.
  QObject::connect(&this->Internal->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  // Now connect to the signals fired by widgets that are not directly connected
  // to the server manager properties using pqPropertyLinks.
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  QObject::connect(model,
    SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
    this, SLOT(updateSeriesOptions()));
  QObject::connect(model,
    SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    this, SLOT(updateSeriesOptions()));
  QObject::connect(this->Internal->Model, SIGNAL(modelReset()),
    this, SLOT(updateSeriesOptions()));

  QObject::connect(this->Internal->SeriesEnabled, SIGNAL(stateChanged(int)),
    this, SLOT(setCurrentSeriesEnabled(int)));
  QObject::connect(
    this->Internal->ColorButton, SIGNAL(chosenColorChanged(const QColor &)),
    this, SLOT(setCurrentSeriesColor(const QColor &)));

  QObject::connect(
    this->Internal->SeriesList, SIGNAL(activated(const QModelIndex &)),
    this, SLOT(activateItem(const QModelIndex &)));

  this->Internal->Model->reload();
  this->updateSeriesOptions();
}

//-----------------------------------------------------------------------------
pqBarChartDisplayPanel::~pqBarChartDisplayPanel()
{
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayPanel::updateSeriesOptions()
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  QModelIndex current = model->currentIndex();
  QModelIndexList indexes = model->selectedIndexes();
  if((!current.isValid() || !model->isSelected(current)) &&
    indexes.size() > 0)
    {
    current = indexes.last();
    }

  // Use the selection list to determine the tri-state of the
  // enabled and legend check boxes.
  this->Internal->SeriesEnabled->blockSignals(true);
  this->Internal->SeriesEnabled->setCheckState(this->getEnabledState());
  this->Internal->SeriesEnabled->blockSignals(false);

  this->Internal->ColorButton->blockSignals(true);
  if (current.isValid())
    {
    int seriesIndex = current.row();
    QColor color = this->Internal->Model->getSeriesColor(seriesIndex);
    this->Internal->ColorButton->setChosenColor(color);
    }
  else
    {
    this->Internal->ColorButton->setChosenColor(Qt::white);
    }
  this->Internal->ColorButton->blockSignals(false);

  // Disable the widgets if nothing is selected or current.
  bool hasItems = indexes.size() > 0;
  this->Internal->SeriesEnabled->setEnabled(hasItems);
  this->Internal->ColorButton->setEnabled(hasItems);
}

//-----------------------------------------------------------------------------
Qt::CheckState pqBarChartDisplayPanel::getEnabledState() const
{
  Qt::CheckState enabledState = Qt::Unchecked;
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  // Use the selection list to determine the tri-state of the
  // enabled check box.
  bool enabled = false;
  QModelIndexList indexes = model->selectedIndexes();
  bool initialized = false;
  foreach (QModelIndex index, indexes)
    {
    enabled = this->Internal->Model->getSeriesEnabled(index.row()); 
    if (!initialized)
      {
      enabledState = enabled ? Qt::Checked : Qt::Unchecked;
      initialized = true;
      }
    else if((enabled && enabledState == Qt::Unchecked) ||
      (!enabled && enabledState == Qt::Checked))
      {
      enabledState = Qt::PartiallyChecked;
      break;
      }
    }

  return enabledState;
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayPanel::activateItem(const QModelIndex &index)
{
  if (!index.isValid() || index.column() != 1)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  // Get current color
  QColor color = this->Internal->Model->getSeriesColor(index.row());

  // Show color selector dialog to get a new color
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
    // Set the new color
    this->Internal->Model->setSeriesColor(index.row(), color);
    this->Internal->ColorButton->blockSignals(true);
    this->Internal->ColorButton->setChosenColor(color);
    this->Internal->ColorButton->blockSignals(false);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayPanel::setCurrentSeriesEnabled(int state)
{
  if (state == Qt::PartiallyChecked)
    {
    // Ignore changes to partially checked state.
    return;
    }

  bool enabled = (state == Qt::Checked);
  this->Internal->SeriesEnabled->setTristate(false);

  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  QModelIndexList indexes = model->selectedIndexes();
  foreach (QModelIndex idx, indexes)
    {
    this->Internal->Model->setSeriesEnabled(idx.row(), enabled);
    }

  if (indexes.size() > 0)
    {
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayPanel::setCurrentSeriesColor(const QColor &color)
{
  QItemSelectionModel *model = this->Internal->SeriesList->selectionModel();
  QModelIndexList indexes = model->selectedIndexes();
  foreach (QModelIndex index, indexes)
    {
    this->Internal->Model->setSeriesColor(index.row(), color);
    }

  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayPanel::useArrayIndexToggled(bool toggle)
{
  this->Internal->UseDataArray->setChecked(!toggle);
}

//-----------------------------------------------------------------------------
void pqBarChartDisplayPanel::useDataArrayToggled(bool toggle)
{
  this->Internal->UseArrayIndex->setChecked(!toggle);
}
