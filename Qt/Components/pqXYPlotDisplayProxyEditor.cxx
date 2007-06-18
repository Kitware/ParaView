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
#include "pqLineChartDisplay.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidgetItemObject.h"

//-----------------------------------------------------------------------------
class pqXYPlotDisplayProxyEditor::pqInternal : public Ui::Form
{
public:
  pqPropertyLinks Links;
  pqInternal()
    {
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    this->AttributeModeAdaptor = 0;
    }
  ~pqInternal()
    {
    delete this->XAxisArrayAdaptor;
    delete this->XAxisArrayDomain;
    delete this->AttributeModeAdaptor;
    }
  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqSignalAdaptorComboBox* AttributeModeAdaptor;
  pqComboBoxDomain* XAxisArrayDomain;

  QPointer<pqLineChartDisplay> Display;
  QList<QPointer<pqTreeWidgetItemObject> > TreeItems;
};

//-----------------------------------------------------------------------------
pqXYPlotDisplayProxyEditor::pqXYPlotDisplayProxyEditor(pqDisplay* display, QWidget* p)
  : pqDisplayPanel(display, p)
{
  this->Internal = new pqXYPlotDisplayProxyEditor::pqInternal();
  this->Internal->setupUi(this);

  pqTreeWidgetCheckHelper* helper = new pqTreeWidgetCheckHelper(
    this->Internal->YAxisArrays, 0, this);
  helper->setCheckMode(pqTreeWidgetCheckHelper::CLICK_IN_COLUMN);

  QObject::connect(this->Internal->YAxisArrays, 
    SIGNAL(itemClicked(QTreeWidgetItem*, int)),
    this, SLOT(onItemClicked(QTreeWidgetItem*, int)));

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
    QString seriesName;
    this->Internal->Display->getSeriesName(i, seriesName);
    pqTreeWidgetItemObject *item = new pqTreeWidgetItemObject(
        this->Internal->YAxisArrays, QStringList(seriesName));
    item->setData(0, Qt::ToolTipRole, seriesName);
    item->setChecked(this->Internal->Display->isSeriesEnabled(i));

    // Add a column for the color.
    QColor seriesColor;
    this->Internal->Display->getSeriesColor(i, seriesColor);
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(seriesColor);
    item->setData(1, Qt::DecorationRole, colorPixmap);
    item->setData(1, Qt::UserRole, QVariant(seriesColor));
    QObject::connect(item,  SIGNAL(checkedStateChanged(bool)),
      this, SLOT(setSeriesEnabled(bool)));
    }
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
void pqXYPlotDisplayProxyEditor::setDisplay(pqDisplay* disp)
{
  pqLineChartDisplay* display = qobject_cast<pqLineChartDisplay*>(disp);
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

  if (!proxy || proxy->GetXMLName() != QString("XYPlotDisplay2"))
    {
    qDebug() << "Proxy must be a XYPlotDisplay2 display to be editable in "
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
      this->Internal->Display, SIGNAL(colorChanged(int, const QColor &)),
      this, SLOT(updateItemColor(int, const QColor &)));

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
void pqXYPlotDisplayProxyEditor::onItemClicked(QTreeWidgetItem* item, int col)
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

void pqXYPlotDisplayProxyEditor::updateItemColor(int index,
    const QColor &color)
{
  // Get the item for the given index.
  QTreeWidgetItem *item = this->Internal->YAxisArrays->topLevelItem(index);
  if(item)
    {
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(color);
    item->setData(1, Qt::DecorationRole, colorPixmap);
    item->setData(1, Qt::UserRole, QVariant(color));
    }
}

