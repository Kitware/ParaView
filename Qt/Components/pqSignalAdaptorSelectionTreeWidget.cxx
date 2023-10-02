// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSignalAdaptorSelectionTreeWidget.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMTimeStepsDomain.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QTreeWidget>

#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqSignalAdaptorSelectionTreeWidget::pqInternal
{
public:
  QPointer<QTreeWidget> TreeWidget;
  vtkSmartPointer<vtkSMProperty> Property;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqSignalAdaptorSelectionTreeWidget::pqSignalAdaptorSelectionTreeWidget(
  QTreeWidget* treeWidget, vtkSMProperty* prop)
  : QObject(treeWidget)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;
  this->Internal->TreeWidget = treeWidget;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->ItemCreatorFunctionPtr = nullptr;

  // get domain
  vtkSMDomainIterator* iter = prop->NewDomainIterator();
  iter->Begin();
  while (!iter->IsAtEnd() && !this->Internal->Domain)
  {
    vtkSMDomain* d = iter->GetDomain();
    if (vtkSMEnumerationDomain::SafeDownCast(d) || vtkSMStringListDomain::SafeDownCast(d) ||
      vtkSMTimeStepsDomain::SafeDownCast(d))
    {
      this->Internal->Domain = d;
    }
    iter->Next();
  }
  iter->Delete();

  if (this->Internal->Domain)
  {
    this->Internal->VTKConnect->Connect(this->Internal->Domain, vtkCommand::DomainModifiedEvent,
      this, SLOT(domainChanged()), nullptr, 0);

    this->domainChanged();
  }

  QObject::connect(this->Internal->TreeWidget->model(),
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SIGNAL(valuesChanged()));
  QObject::connect(
    this->Internal->TreeWidget->model(), SIGNAL(modelReset()), this, SIGNAL(valuesChanged()));
  QObject::connect(this->Internal->TreeWidget->model(),
    SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SIGNAL(valuesChanged()));
  QObject::connect(this->Internal->TreeWidget->model(),
    SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SIGNAL(valuesChanged()));
}

//-----------------------------------------------------------------------------
pqSignalAdaptorSelectionTreeWidget::~pqSignalAdaptorSelectionTreeWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QList<QList<QVariant>> pqSignalAdaptorSelectionTreeWidget::values() const
{
  QList<QList<QVariant>> reply;

  int max = this->Internal->TreeWidget->topLevelItemCount();
  for (int cc = 0; cc < max; cc++)
  {
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    QList<QVariant> itemValue;
    itemValue.append(item->text(0));
    itemValue.append(item->checkState(0) == Qt::Checked ? 1 : 0);
    reply.append(itemValue);
  }

  return reply;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorSelectionTreeWidget::setValues(const QList<QList<QVariant>>& new_values)
{
  if (new_values.size() != this->Internal->TreeWidget->topLevelItemCount())
  {
    qDebug("inconsistent count in selection list\n");
  }

  bool old_bs = this->blockSignals(true);
  int max = this->Internal->TreeWidget->topLevelItemCount();
  if (new_values.size() < max)
  {
    max = new_values.size();
  }

  bool changed = false;
  for (int cc = 0; cc < max; cc++)
  {
    QList<QVariant> nval = new_values[cc];
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    if (item->text(0) != nval[0])
    {
      item->setText(0, nval[0].toString());
      changed = true;
    }
    Qt::CheckState newState = nval[1].toInt() == 0 ? Qt::Unchecked : Qt::Checked;
    if (item->checkState(0) != newState)
    {
      item->setCheckState(0, newState);
      changed = true;
    }
  }
  this->blockSignals(old_bs);

  if (changed)
  {
    Q_EMIT this->valuesChanged();
  }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorSelectionTreeWidget::domainChanged()
{
  // when domain changes, we need to update the property with
  // new default values
  QList<QVariant> newDomain = pqSMAdaptor::getSelectionPropertyDomain(this->Internal->Property);
  QList<QList<QVariant>> oldValues = this->values();

  bool equal = true;
  if (oldValues.size() == newDomain.size())
  {
    for (int i = 0; equal && i < oldValues.size(); i++)
    {
      if (oldValues[i][0] != newDomain[i])
      {
        equal = false;
      }
    }
  }
  else
  {
    equal = false;
  }

  if (equal)
  {
    return;
  }

  // Domain changes should not change the property values. This is overriding
  // the value loaded from state files etc.
  // this->Internal->Property->ResetToDefault();

  QList<QList<QVariant>> newValues = pqSMAdaptor::getSelectionProperty(this->Internal->Property);

  // Now update the tree widget. We hide any elements no longer in the domain.
  this->Internal->TreeWidget->clear();

  Q_FOREACH (QList<QVariant> newValue, newValues)
  {
    QTreeWidgetItem* item = nullptr;
    if (this->ItemCreatorFunctionPtr)
    {
      item = (*this->ItemCreatorFunctionPtr)(
        this->Internal->TreeWidget, QStringList(newValue[0].toString()));
    }
    if (!item)
    {
      item = new QTreeWidgetItem(this->Internal->TreeWidget, QStringList(newValue[0].toString()));
    }
    item->setCheckState(0, newValue[1].toInt() ? Qt::Checked : Qt::Unchecked);
  }
}
