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

#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QtDebug>
#include <QPointer>
#include <QPixmap>

#include "pqDisplay.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetItemObject.h"
#include "pqComboBoxDomain.h"
#include "ui_pqXYPlotDisplayEditor.h"

//-----------------------------------------------------------------------------
class pqXYPlotDisplayProxyEditor::pqInternal : public Ui::Form
{
public:
  pqPropertyLinks Links;
  pqInternal()
    {
    this->XAxisModeAdaptor = 0;
    this->XAxisArrayDomain = 0;
    this->XAxisArrayAdaptor = 0;
    }
  ~pqInternal()
    {
    delete this->XAxisModeAdaptor;
    delete this->XAxisArrayAdaptor;
    delete this->XAxisArrayDomain;
    }
  pqSignalAdaptorComboBox* XAxisModeAdaptor;
  pqSignalAdaptorComboBox* XAxisArrayAdaptor;
  pqComboBoxDomain* XAxisArrayDomain;

  QPointer<pqDisplay> Display;
};

//-----------------------------------------------------------------------------
pqXYPlotDisplayProxyEditor::pqXYPlotDisplayProxyEditor(QWidget* p)
  : QWidget(p)
{
  this->Internal = new pqXYPlotDisplayProxyEditor::pqInternal();
  this->Internal->setupUi(this);
  this->Internal->XAxisModeAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XAxisMode);
  this->Internal->XAxisArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Internal->XAxisArray);

  QObject::connect(this->Internal->XAxisModeAdaptor,
    SIGNAL(currentTextChanged(const QString&)), 
    this, SLOT(updateAllViews()),
    Qt::QueuedConnection);
  QObject::connect(this->Internal->XAxisArrayAdaptor,
    SIGNAL(currentTextChanged(const QString&)), 
    this, SLOT(updateAllViews()),
    Qt::QueuedConnection);

  QObject::connect(this->Internal->ViewData, SIGNAL(stateChanged(int)),
    this, SLOT(updateAllViews()), Qt::QueuedConnection);

}

//-----------------------------------------------------------------------------
pqXYPlotDisplayProxyEditor::~pqXYPlotDisplayProxyEditor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqDisplay* pqXYPlotDisplayProxyEditor::getDisplay()
{
  return this->Internal->Display;
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::setDisplay(pqDisplay* display)
{
  if (this->Internal->Display == display)
    {
    return;
    }
  this->setEnabled(false);
  // Clean up stuff setup during previous call to setDisplay.
  this->Internal->Links.removeAllPropertyLinks();
  QList<pqTreeWidgetItemObject*> oldItems = 
    this->Internal->YAxisArrays->findChildren<pqTreeWidgetItemObject*>();
  foreach (pqTreeWidgetItemObject* item, oldItems)
    {
    delete item;
    }
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

  // Setup XAxisMode links.
  this->Internal->XAxisMode->clear();
  QList<QVariant> modes = pqSMAdaptor::getEnumerationPropertyDomain(
    proxy->GetProperty("XAxisMode"));
  foreach(QVariant item, modes)
    {
    this->Internal->XAxisMode->addItem(item.toString());
    }
  this->Internal->Links.addPropertyLink(this->Internal->XAxisModeAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    proxy, proxy->GetProperty("XAxisMode"));

  proxy->GetProperty("Input")->UpdateDependentDomains();

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


  // Now, build check boxes for y axis array selections. 
  QList<QString> input_scalars = pqSMAdaptor::getFieldSelectionScalarDomain(
    proxy->GetProperty("YArrayNames"));

  QList<QVariant> currentValues = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty("YArrayNames"));

  QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");
  foreach(QString array, input_scalars)
    {
    QStringList strs;
    strs.append(array);
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(
      this->Internal->YAxisArrays, strs);
    item->setData(0, Qt::ToolTipRole, array);
    item->setChecked(currentValues.contains(QVariant(array)));
    item->setData(0, Qt::DecorationRole, pointPixmap);
    QObject::connect(item,  SIGNAL(checkedStateChanged(bool)),
      this, SLOT(yArraySelectionChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::yArraySelectionChanged()
{
  pqTreeWidgetItemObject* item = qobject_cast<pqTreeWidgetItemObject*>(
    this->sender());
  if (!item)
    {
    return;
    }
  QString arrayName = item->data(0, Qt::ToolTipRole).toString();
  bool checked = item->isChecked();

  vtkSMProperty* prop = 
    this->Internal->Display->getProxy()->GetProperty("YArrayNames");
  QList<QVariant> selectedArrays = 
    pqSMAdaptor::getMultipleElementProperty(prop);
  if (!checked && selectedArrays.contains(arrayName))
    {
    selectedArrays.removeAll(arrayName);
    }
  else if (checked && !selectedArrays.contains(arrayName))
    {
    selectedArrays.push_back(arrayName);
    }
  pqSMAdaptor::setMultipleElementProperty(prop, selectedArrays);
  vtkSMStringVectorProperty::SafeDownCast(prop)->SetNumberOfElements(
    selectedArrays.size());
  this->Internal->Display->getProxy()->UpdateVTKObjects();
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void pqXYPlotDisplayProxyEditor::updateAllViews()
{
  if (this->Internal->Display)
    {
    this->Internal->Display->renderAllViews();
    }
}
