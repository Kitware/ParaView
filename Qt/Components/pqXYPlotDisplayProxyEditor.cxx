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

  QObject::connect(this->Internal->UseYArrayIndex, SIGNAL(toggled(bool)), 
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
  foreach (pqTreeWidgetItemObject* item, this->Internal->TreeItems)
    {
    delete item;
    }
  this->Internal->TreeItems.clear();
  delete this->Internal->XAxisArrayDomain;
  this->Internal->XAxisArrayDomain = 0;

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
  this->Internal->Links.addPropertyLink(this->Internal->UseYArrayIndex,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("UseYArrayIndex"));

  // Attribute mode.
  this->Internal->Links.addPropertyLink(this->Internal->AttributeModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("AttributeType"));

  // pqComboBoxDomain will ensure that when ever the domain changes the
  // widget is updated as well.
  this->Internal->XAxisArrayDomain = new pqComboBoxDomain(
    this->Internal->XAxisArray, proxy->GetProperty("XArrayName"), 1);
  // This is useful to initially populate the combobox.
  this->Internal->XAxisArrayDomain->forceDomainChanged();

  // This link will ensure that when ever the widget selection changes,
  // the property XArrayName will be updated as well.
  this->Internal->Links.addPropertyLink(this->Internal->XAxisArrayAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("XArrayName"));

  this->reloadGUI();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::reloadGUI()
{
  if (!this->Internal->Display)
    {
    return;
    }
  vtkSMProxy* proxy = this->Internal->Display->getProxy();
  if (!proxy)
    {
    return;
    }
  this->Internal->YAxisArrays->clear();

  proxy->GetProperty("Input")->UpdateDependentDomains();
  proxy->UpdatePropertyInformation();
  proxy->GetProperty("CellArrayInfo")->UpdateDependentDomains();
  proxy->GetProperty("PointArrayInfo")->UpdateDependentDomains();

  // Now, build check boxes for y axis array selections. 
  int attribute_mode = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  QPixmap pixmap(attribute_mode == vtkDataObject::FIELD_ASSOCIATION_POINTS?
    ":/pqWidgets/Icons/pqPointData16.png":":/pqWidgets/Icons/pqCellData16.png");

  vtkSMProperty* smproperty  = proxy->GetProperty(
    attribute_mode == vtkDataObject::FIELD_ASSOCIATION_POINTS?
    "YPointArrayStatus" : "YCellArrayStatus");
  vtkSMArraySelectionDomain* asd = vtkSMArraySelectionDomain::SafeDownCast(
    smproperty->GetDomain("array_list"));

  // Get the array names available from the domain, however, 
  // their selection state and color is obtained from the property.
  
  int num_y_values = asd->GetNumberOfStrings();
  for (int cc=0; cc < num_y_values; cc++)
    {
    QString arrayname = asd->GetString(cc);
    QStringList strs;
    strs.append(arrayname);
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(
      this->Internal->YAxisArrays, strs);
    this->Internal->TreeItems.push_back(item);
    item->setData(0, Qt::ToolTipRole, arrayname);
    item->setChecked(this->Internal->Display->getYArrayEnabled(arrayname));
    item->setData(0, Qt::DecorationRole, pixmap);
    QColor color = this->Internal->Display->getYColor(arrayname);
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(color);
    item->setData(1, Qt::DecorationRole, colorPixmap);
    item->setData(1, Qt::UserRole, QVariant(color));
    QObject::connect(item,  SIGNAL(checkedStateChanged(bool)),
      this, SLOT(yArraySelectionChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::yArraySelectionChanged()
{
  this->updateSMState();
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

  this->reloadGUI();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::updateSMState()
{
  vtkSMProxy* proxy = this->Internal->Display->getProxy();
  int attribute_mode = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  vtkSMStringVectorProperty* smproperty  = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty(
      attribute_mode == vtkDataObject::FIELD_ASSOCIATION_POINTS?
      "YPointArrayStatus" : "YCellArrayStatus"));

  QList<QVariant> values;
  foreach(pqTreeWidgetItemObject* item, this->Internal->TreeItems)
    {
    QString arrayName = item->data(0, Qt::ToolTipRole).toString();
    bool checked = item->isChecked();
    QColor color = item->data(1, Qt::UserRole).value<QColor>();

    values.push_back(color.redF());
    values.push_back(color.greenF());
    values.push_back(color.blueF());
    values.push_back(checked);
    values.push_back(arrayName);
    }

  smproperty->SetNumberOfElements(values.size());
  pqSMAdaptor::setMultipleElementProperty(smproperty, values);
  proxy->UpdateVTKObjects();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::onItemClicked(QTreeWidgetItem* item, int col)
{
  if (col != 1)
    {
    // We are interested in clicks on the color swab alone.
    return;
    }

  QColor color = item->data(1, Qt::UserRole).value<QColor>();
  color = QColorDialog::getColor(color, this);
  if (color.isValid())
    {
    QPixmap colorPixmap(16, 16);
    colorPixmap.fill(color);
    item->setData(1, Qt::DecorationRole, colorPixmap);
    item->setData(1, Qt::UserRole, QVariant(color));
    this->updateSMState();
    }
}

