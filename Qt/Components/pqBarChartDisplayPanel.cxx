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

#include "vtkSMChartTableRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"

#include <QPointer>
#include <QDebug>

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
  vtkWeakPointer<vtkSMChartTableRepresentationProxy> ChartRepresentation;
  pqChartSeriesEditorModel *Model;
};

//-----------------------------------------------------------------------------
pqBarChartDisplayPanel::pqBarChartDisplayPanel(pqRepresentation* repr,
  QWidget* parentObject): Superclass(repr, parentObject), Internal(0)
{
  vtkSMChartTableRepresentationProxy* proxy =
    vtkSMChartTableRepresentationProxy::SafeDownCast(repr->getProxy());
  if (!proxy)
    {
    this->setEnabled(false);
    qCritical() << "pqBarChartDisplayPanel "
      "can only work with vtkSMChartTableRepresentationProxy";
    return;
    }

  this->Internal = new pqBarChartDisplayPanel::pqInternal();
  this->Internal->setupUi(this);

  // Create the model for showing the list of series available.
  this->Internal->Model = new pqChartSeriesEditorModel(this);
  this->Internal->SeriesList->setModel(this->Internal->Model);
  // Give the representation to our series editor model
  this->Internal->Model->setRepresentation(
    qobject_cast<pqDataRepresentation*>(repr));

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
