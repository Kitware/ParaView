/*=========================================================================

   Program: ParaView
   Module:    pqXYPlotDisplayProxyEditor.cxx

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
#include "pqXYPlotDisplayProxyEditor.h"
#include "ui_pqXYPlotDisplayEditor.h"

#include "vtkSMArraySelectionDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkDataObject.h"

#include <QtDebug>
#include <QPointer>
#include <QPixmap>
#include <QColorDialog>
#include <QList>

#include "pqComboBoxDomain.h"
#include "pqLineChartRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidgetItemObject.h"

//-----------------------------------------------------------------------------
class pqXYPlotDisplayProxyEditor::pqInternal : public Ui::Form
{
public:
  pqInternal()
    {
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    this->AttributeModeAdaptor = 0;
    this->InChange = false;
    }

  ~pqInternal()
    {
    delete this->XAxisArrayAdaptor;
    delete this->XAxisArrayDomain;
    delete this->AttributeModeAdaptor;
    }

  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqSignalAdaptorComboBox* AttributeModeAdaptor;
  pqComboBoxDomain* XAxisArrayDomain;

  QPointer<pqLineChartRepresentation> Display;
  QList<QPointer<pqTreeWidgetItemObject> > TreeItems;

  bool InChange;
};

//-----------------------------------------------------------------------------
pqXYPlotDisplayProxyEditor::pqXYPlotDisplayProxyEditor(pqRepresentation* display, QWidget* p)
  : pqDisplayPanel(display, p)
{
  this->Internal = new pqXYPlotDisplayProxyEditor::pqInternal();
  this->Internal->setupUi(this);

  pqTreeWidgetCheckHelper* helper = new pqTreeWidgetCheckHelper(
    this->Internal->YAxisArrays, 0, this);
  helper->setCheckMode(pqTreeWidgetCheckHelper::CLICK_IN_COLUMN);

  QObject::connect(this->Internal->YAxisArrays, 
    SIGNAL(itemActivated(QTreeWidgetItem *, int)),
    this, SLOT(activateItem(QTreeWidgetItem *, int)));

  this->Internal->XAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XAxisArray);

  this->Internal->AttributeModeAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->AttributeMode);

  QObject::connect(this->Internal->UseArrayIndex, SIGNAL(toggled(bool)), 
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  QObject::connect(this->Internal->XAxisArrayAdaptor,
    SIGNAL(currentTextChanged(const QString&)), 
    this, SLOT(updateAllViews()),
    Qt::QueuedConnection);

  QObject::connect(this->Internal->AttributeModeAdaptor,
    SIGNAL(currentTextChanged(const QString&)), 
    this, SLOT(onAttributeModeChanged()),
    Qt::QueuedConnection);

  QObject::connect(this->Internal->ViewData, SIGNAL(stateChanged(int)),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

  QObject::connect(this->Internal->YAxisArrays, SIGNAL(itemSelectionChanged()),
    this, SLOT(updateOptionsWidgets()));
  QObject::connect(this->Internal->YAxisArrays,
    SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
    this, SLOT(updateOptionsWidgets()));

  QObject::connect(this->Internal->SeriesEnabled, SIGNAL(stateChanged(int)),
    this, SLOT(setCurrentSeriesEnabled(int)));
  QObject::connect(this->Internal->ShowInLegend, SIGNAL(stateChanged(int)),
    this, SLOT(setCurrentSeriesInLegend(int)));
  QObject::connect(
    this->Internal->ColorButton, SIGNAL(chosenColorChanged(const QColor &)),
    this, SLOT(setCurrentSeriesColor(const QColor &)));
  QObject::connect(this->Internal->Thickness, SIGNAL(valueChanged(int)),
    this, SLOT(setCurrentSeriesThickness(int)));
  QObject::connect(this->Internal->StyleList, SIGNAL(currentIndexChanged(int)),
    this, SLOT(setCurrentSeriesStyle(int)));

  this->setDisplay(display);
}

//-----------------------------------------------------------------------------
pqXYPlotDisplayProxyEditor::~pqXYPlotDisplayProxyEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::reloadSeries()
{
  this->Internal->YAxisArrays->clear();
  this->Internal->TreeItems.clear();
  if(!this->Internal->Display)
    {
    return;
    }

  int total = this->Internal->Display->getNumberOfSeries();
  for(int i = 0; i < total; i++)
    {
    QStringList columnData;
    QString seriesName, seriesLabel;
    this->Internal->Display->getSeriesName(i, seriesName);
    this->Internal->Display->getSeriesLabel(i, seriesLabel);
    columnData << seriesName << seriesLabel;
    pqTreeWidgetItemObject *item = new pqTreeWidgetItemObject(
        this->Internal->YAxisArrays, columnData);
    item->setData(0, Qt::ToolTipRole, seriesName);
    item->setChecked(this->Internal->Display->isSeriesEnabled(i));

    // Add a pixmap for the color.
    QColor seriesColor;
    this->Internal->Display->getSeriesColor(i, seriesColor);
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(seriesColor);
    item->setData(1, Qt::DecorationRole, colorPixmap);
    QObject::connect(item,  SIGNAL(checkedStateChanged(bool)),
      this, SLOT(setSeriesEnabled(bool)));
    }

  this->updateOptionsWidgets();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::setSeriesEnabled(bool enabled)
{
  // Get the tree widget item from the sender.
  pqTreeWidgetItemObject *item =
      qobject_cast<pqTreeWidgetItemObject *>(this->sender());
  if(item)
    {
    int series = this->Internal->Display->getSeriesIndex(
        item->data(0, Qt::DisplayRole).toString());
    this->Internal->Display->setSeriesEnabled(series, enabled);
    this->updateAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::setDisplay(pqRepresentation* disp)
{
  pqLineChartRepresentation* display = qobject_cast<pqLineChartRepresentation*>(disp);
  if (this->Internal->Display == display)
    {
    return;
    }

  this->setEnabled(false);
  // Clean up stuff setup during previous call to setDisplay.
  this->Internal->Links.removeAllPropertyLinks();
  this->Internal->YAxisArrays->clear();
  this->Internal->TreeItems.clear();
  delete this->Internal->XAxisArrayDomain;
  this->Internal->XAxisArrayDomain = 0;
  if(this->Internal->Display)
    {
    this->disconnect(this->Internal->Display, 0, this, 0);
    }

  this->Internal->Display = display;
  if (!this->Internal->Display)
    {
    // Display is null, nothing to do.
    return;
    }
  vtkSMProxy* proxy = display->getProxy();

  if (!proxy || proxy->GetXMLName() != QString("XYPlotRepresentation"))
    {
    qDebug() << "Proxy must be a XYPlotRepresentation display to be editable in "
      "pqXYPlotDisplayProxyEditor.";
    return;
    }
  this->setEnabled(true);

  // Setup links for visibility.
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  // Setup UseYArrayIndex links.
  this->Internal->Links.addPropertyLink(this->Internal->UseArrayIndex,
    "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseYArrayIndex"));

  // Attribute mode.
  this->Internal->Links.addPropertyLink(this->Internal->AttributeModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("AttributeType"));

  // pqComboBoxDomain will ensure that when ever the domain changes the
  // widget is updated as well.
  this->Internal->XAxisArrayDomain = new pqComboBoxDomain(
    this->Internal->XAxisArray, proxy->GetProperty("XArrayName"));
  // This is useful to initially populate the combobox.
  this->Internal->XAxisArrayDomain->forceDomainChanged();

  // This link will ensure that when ever the widget selection changes,
  // the property XArrayName will be updated as well.
  this->Internal->Links.addPropertyLink(this->Internal->XAxisArrayAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("XArrayName"));

  this->connect(this->Internal->Display, SIGNAL(seriesListChanged()),
      this, SLOT(reloadSeries()));
  this->connect(
      this->Internal->Display, SIGNAL(enabledStateChanged(int, bool)),
      this, SLOT(updateItemEnabled(int)));
  this->connect(this->Internal->Display, SIGNAL(legendStateChanged(int, bool)),
      this, SLOT(updateItemLegend(int)));
  this->connect(
      this->Internal->Display, SIGNAL(colorChanged(int, const QColor &)),
      this, SLOT(updateItemColor(int, const QColor &)));
  this->connect(
      this->Internal->Display, SIGNAL(styleChanged(int, Qt::PenStyle)),
      this, SLOT(updateItemStyle(int, Qt::PenStyle)));

  this->reloadSeries();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::onAttributeModeChanged()
{
  vtkSMProxy* proxy = this->Internal->Display->getProxy();
  vtkSMIntVectorProperty* at = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty("AttributeType"));
  // FIXME HACK: The domain uses unchecked elements to update the values,
  // hence  we update the unchecked element.
  at->SetUncheckedElement(0, at->GetElement(0));
  proxy->GetProperty("AttributeType")->UpdateDependentDomains();

  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::activateItem(QTreeWidgetItem *item, int col)
{
  if(col != 1 || !this->Internal->Display)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  int series = this->Internal->Display->getSeriesIndex(
      item->data(0, Qt::DisplayRole).toString());
  QColor color;
  this->Internal->Display->getSeriesColor(series, color);
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
    this->Internal->Display->setSeriesColor(series, color);
    this->updateAllViews();
    }
}

void pqXYPlotDisplayProxyEditor::updateOptionsWidgets()
{
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    // Use the selection list to determine the tri-state of the
    // enabled and legend check boxes.
    Qt::CheckState enabledState = this->getEnabledState();
    this->Internal->SeriesEnabled->blockSignals(true);
    this->Internal->ShowInLegend->blockSignals(true);
    this->Internal->SeriesEnabled->setCheckState(enabledState);
    this->Internal->ShowInLegend->setCheckState(this->getInLegendState());
    this->Internal->SeriesEnabled->blockSignals(false);
    this->Internal->ShowInLegend->blockSignals(false);

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
    if(current.isValid())
      {
      QColor color;
      this->Internal->Display->getSeriesColor(current.row(), color);
      this->Internal->ColorButton->setChosenColor(color);
      this->Internal->Thickness->setValue(
          this->Internal->Display->getSeriesThickness(current.row()));
      this->Internal->StyleList->setCurrentIndex(
          (int)this->Internal->Display->getSeriesStyle(current.row()) - 1);
      }
    else
      {
      this->Internal->ColorButton->setChosenColor(Qt::white);
      this->Internal->Thickness->setValue(0);
      this->Internal->StyleList->setCurrentIndex(0);
      }

    this->Internal->ColorButton->blockSignals(false);
    this->Internal->Thickness->blockSignals(false);
    this->Internal->StyleList->blockSignals(false);

    // Disable the widgets if nothing is selected or current.
    bool hasItems = indexes.size() > 0;
    this->Internal->SeriesEnabled->setEnabled(hasItems);
    this->Internal->ShowInLegend->setEnabled(hasItems &&
        enabledState == Qt::Checked);
    this->Internal->ColorButton->setEnabled(hasItems);
    this->Internal->Thickness->setEnabled(hasItems);
    this->Internal->StyleList->setEnabled(hasItems);
    }
}

void pqXYPlotDisplayProxyEditor::setCurrentSeriesEnabled(int state)
{
  if(state == Qt::PartiallyChecked)
    {
    // Ignore changes to partially checked state.
    return;
    }

  bool enabled = state == Qt::Checked;
  this->Internal->SeriesEnabled->setTristate(false);
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    this->Internal->Display->beginSeriesChanges();
    this->Internal->InChange = true;
    pqTreeWidgetItemObject *item = 0;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Display->setSeriesEnabled(iter->row(), enabled);
      item = dynamic_cast<pqTreeWidgetItemObject *>(
          this->Internal->YAxisArrays->topLevelItem(iter->row()));
      if(item)
        {
        item->blockSignals(true);
        item->setChecked(enabled);
        item->blockSignals(false);
        }
      }

    // Update the legend check box.
    this->Internal->ShowInLegend->blockSignals(true);
    this->Internal->ShowInLegend->setCheckState(this->getInLegendState());
    this->Internal->ShowInLegend->blockSignals(false);
    this->Internal->ShowInLegend->setEnabled(enabled &&
        this->Internal->SeriesEnabled->isEnabled());
    if(this->Internal->ShowInLegend->checkState() != Qt::PartiallyChecked)
      {
      this->Internal->ShowInLegend->setTristate(false);
      }

    this->Internal->InChange = false;
    this->Internal->Display->endSeriesChanges();
    this->updateAllViews();
    }
}

void pqXYPlotDisplayProxyEditor::setCurrentSeriesInLegend(int state)
{
  if(state == Qt::PartiallyChecked)
    {
    // Ignore changes to partially checked state.
    return;
    }

  bool inLegend = state == Qt::Checked;
  this->Internal->ShowInLegend->setTristate(false);
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    this->Internal->Display->beginSeriesChanges();
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Display->setSeriesInLegend(iter->row(), inLegend);
      }

    this->Internal->InChange = false;
    this->Internal->Display->endSeriesChanges();
    this->updateAllViews();
    }
}

void pqXYPlotDisplayProxyEditor::setCurrentSeriesColor(const QColor &color)
{
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    this->Internal->Display->beginSeriesChanges();
    this->Internal->InChange = true;
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(color);
    pqTreeWidgetItemObject *item = 0;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Display->setSeriesColor(iter->row(), color);
      item = dynamic_cast<pqTreeWidgetItemObject *>(
          this->Internal->YAxisArrays->topLevelItem(iter->row()));
      if(item)
        {
        item->setData(1, Qt::DecorationRole, colorPixmap);
        }
      }

    this->Internal->InChange = false;
    this->Internal->Display->endSeriesChanges();
    this->updateAllViews();
    }
}

void pqXYPlotDisplayProxyEditor::setCurrentSeriesThickness(int thickness)
{
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    this->Internal->Display->beginSeriesChanges();
    this->Internal->InChange = true;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Display->setSeriesThickness(iter->row(), thickness);
      }

    this->Internal->InChange = false;
    this->Internal->Display->endSeriesChanges();
    this->updateAllViews();
    }
}

void pqXYPlotDisplayProxyEditor::setCurrentSeriesStyle(int listIndex)
{
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    this->Internal->Display->beginSeriesChanges();
    this->Internal->InChange = true;
    Qt::PenStyle style = (Qt::PenStyle)(listIndex + 1);
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for( ; iter != indexes.end(); ++iter)
      {
      this->Internal->Display->setSeriesStyle(iter->row(), style);
      }

    this->Internal->InChange = false;
    this->Internal->Display->endSeriesChanges();
    this->updateAllViews();
    }
}

void pqXYPlotDisplayProxyEditor::updateItemEnabled(int index)
{
  if(this->Internal->InChange)
    {
    return;
    }

  // If the index is part of the selection, update the enabled check box.
  QModelIndex changed = this->Internal->YAxisArrays->model()->index(index, 0);
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model && model->isSelected(changed))
    {
    Qt::CheckState enabledState = this->getEnabledState();
    this->Internal->SeriesEnabled->blockSignals(true);
    this->Internal->SeriesEnabled->setCheckState(enabledState);
    this->Internal->SeriesEnabled->blockSignals(false);

    // Update the enabled state of the legend chack box.
    this->Internal->ShowInLegend->setEnabled(enabledState == Qt::Checked &&
        this->Internal->SeriesEnabled->isEnabled());
    }
}

void pqXYPlotDisplayProxyEditor::updateItemLegend(int index)
{
  if(this->Internal->InChange)
    {
    return;
    }

  // If the index is part of the selection, update the legend check box.
  QModelIndex changed = this->Internal->YAxisArrays->model()->index(index, 0);
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model && model->isSelected(changed))
    {
    this->Internal->ShowInLegend->blockSignals(true);
    this->Internal->ShowInLegend->setCheckState(this->getInLegendState());
    this->Internal->ShowInLegend->blockSignals(false);
    }
}

void pqXYPlotDisplayProxyEditor::updateItemColor(int index,
    const QColor &color)
{
  if(this->Internal->InChange)
    {
    return;
    }

  // Update the pixmap for the item.
  QTreeWidgetItem *item = this->Internal->YAxisArrays->topLevelItem(index);
  if(item)
    {
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(color);
    item->setData(1, Qt::DecorationRole, colorPixmap);
    }

  // If the index is the 'current', update the color button.
  QModelIndex changed = this->Internal->YAxisArrays->model()->index(index, 0);
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model && model->isSelected(changed))
    {
    QModelIndex current = model->currentIndex();
    if(!current.isValid() || !model->isSelected(current))
      {
      current = model->selectedIndexes().last();
      }

    if(changed == current)
      {
      this->Internal->ColorButton->blockSignals(true);
      this->Internal->ColorButton->setChosenColor(color);
      this->Internal->ColorButton->blockSignals(false);
      }
    }
}

void pqXYPlotDisplayProxyEditor::updateItemStyle(int index, Qt::PenStyle style)
{
  if(this->Internal->InChange)
    {
    return;
    }

  // If the index is the 'current', update the style combo box.
  QModelIndex changed = this->Internal->YAxisArrays->model()->index(index, 0);
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model && model->isSelected(changed))
    {
    QModelIndex current = model->currentIndex();
    if(!current.isValid() || !model->isSelected(current))
      {
      current = model->selectedIndexes().last();
      }

    if(changed == current)
      {
      this->Internal->StyleList->blockSignals(true);
      this->Internal->StyleList->setCurrentIndex((int)style - 1);
      this->Internal->StyleList->blockSignals(false);
      }
    }
}

Qt::CheckState pqXYPlotDisplayProxyEditor::getEnabledState() const
{
  Qt::CheckState enabledState = Qt::Unchecked;
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    // Use the selection list to determine the tri-state of the
    // enabled check box.
    bool enabled = false;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for(int i = 0; iter != indexes.end(); ++iter, ++i)
      {
      enabled = this->Internal->Display->isSeriesEnabled(iter->row());
      if(i == 0)
        {
        enabledState = enabled ? Qt::Checked : Qt::Unchecked;
        }
      else if((enabled && enabledState == Qt::Unchecked) ||
          (!enabled && enabledState == Qt::Checked))
        {
        enabledState = Qt::PartiallyChecked;
        break;
        }
      }
    }

  return enabledState;
}

Qt::CheckState pqXYPlotDisplayProxyEditor::getInLegendState() const
{
  Qt::CheckState inLegendState = Qt::Unchecked;
  QItemSelectionModel *model = this->Internal->YAxisArrays->selectionModel();
  if(model)
    {
    // Use the selection list to determine the tri-state of the
    // legend check box.
    bool inLegend = false;
    QModelIndexList indexes = model->selectedIndexes();
    QModelIndexList::Iterator iter = indexes.begin();
    for(int i = 0; iter != indexes.end(); ++iter, ++i)
      {
      inLegend = this->Internal->Display->isSeriesInLegend(iter->row());
      if(i == 0)
        {
        inLegendState = inLegend ? Qt::Checked : Qt::Unchecked;
        }
      else if((inLegend && inLegendState == Qt::Unchecked) ||
          (!inLegend && inLegendState == Qt::Checked))
        {
        inLegendState = Qt::PartiallyChecked;
        break;
        }
      }
    }

  return inLegendState;
}


