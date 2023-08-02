// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSelectionAdaptor.h"

// Qt includes.
#include <QAbstractProxyModel>
#include <QPointer>
#include <QSet>
#include <QtDebug>

// ParaView includes.
#include "pqActiveObjects.h"
#include "vtkSMProxy.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
pqSelectionAdaptor::pqSelectionAdaptor(QItemSelectionModel* _parent)
  : QObject(_parent)
  , QSelectionModel(_parent)
  , IgnoreSignals(false)
{
  QObject::connect(this->QSelectionModel,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(selectionChanged()));

  QObject::connect(this->QSelectionModel,
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectionChanged()));

  pqActiveObjects* ao = &pqActiveObjects::instance();
  QObject::connect(ao, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(currentProxyChanged()));
  QObject::connect(
    ao, SIGNAL(selectionChanged(const pqProxySelection&)), this, SLOT(proxySelectionChanged()));
}

//-----------------------------------------------------------------------------
pqSelectionAdaptor::~pqSelectionAdaptor() = default;

//-----------------------------------------------------------------------------
// Returns the QAbstractItemModel used by the QSelectionModel.
// If QSelectionModel uses a QAbstractProxyModel, this method skips
// over all such proxy models and returns the first non-proxy model
// encountered.
const QAbstractItemModel* pqSelectionAdaptor::getQModel() const
{
  const QAbstractItemModel* model = this->getQSelectionModel()->model();

  // Pass thru proxy models.
  const QAbstractProxyModel* proxyModel = qobject_cast<const QAbstractProxyModel*>(model);
  while (proxyModel)
  {
    model = proxyModel->sourceModel();
    proxyModel = qobject_cast<const QAbstractProxyModel*>(model);
  }

  return model;
}

//-----------------------------------------------------------------------------
QModelIndex pqSelectionAdaptor::mapToSource(const QModelIndex& inIndex) const
{
  QModelIndex outIndex = inIndex;
  const QAbstractItemModel* model = this->getQSelectionModel()->model();

  // Pass thru proxy models.
  const QAbstractProxyModel* proxyModel = qobject_cast<const QAbstractProxyModel*>(model);
  while (proxyModel)
  {
    outIndex = proxyModel->mapToSource(outIndex);
    model = proxyModel->sourceModel();
    proxyModel = qobject_cast<const QAbstractProxyModel*>(model);
  }

  return outIndex;
}

//-----------------------------------------------------------------------------
QModelIndex pqSelectionAdaptor::mapFromSource(
  const QModelIndex& inIndex, const QAbstractItemModel* model) const
{
  const QAbstractProxyModel* proxyModel = qobject_cast<const QAbstractProxyModel*>(model);
  if (!proxyModel)
  {
    return inIndex;
  }

  return proxyModel->mapFromSource(this->mapFromSource(inIndex, proxyModel->sourceModel()));
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::selectionChanged()
{
  if (this->IgnoreSignals)
  {
    return;
  }

  this->IgnoreSignals = true;

  QItemSelectionModel* qModel = this->QSelectionModel;

  pqProxySelection selection;
  const QModelIndexList& indexes = qModel->selection().indexes();
  Q_FOREACH (const QModelIndex& index, indexes)
  {
    pqServerManagerModelItem* item = this->mapToItem(this->mapToSource(index));
    if (item)
    {
      selection.push_back(item);
    }
  }
  pqActiveObjects::instance().setSelection(
    selection, this->mapToItem(this->mapToSource(qModel->currentIndex())));
  this->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::currentProxyChanged()
{
  if (this->IgnoreSignals)
  {
    return;
  }
  this->IgnoreSignals = true;

  const QModelIndex& index =
    this->mapFromSource(this->mapFromItem(pqActiveObjects::instance().activePort()),
      this->getQSelectionModel()->model());

  QItemSelectionModel::SelectionFlags command = QItemSelectionModel::NoUpdate;
  command |= QItemSelectionModel::Select;
  this->QSelectionModel->setCurrentIndex(index, command | this->qtSelectionFlags());

  this->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::proxySelectionChanged()
{
  if (this->IgnoreSignals)
  {
    return;
  }

  this->IgnoreSignals = true;

  QItemSelection qSelection;
  const pqProxySelection& selection = pqActiveObjects::instance().selection();
  Q_FOREACH (pqServerManagerModelItem* item, selection)
  {
    const QModelIndex& index =
      this->mapFromSource(this->mapFromItem(item), this->getQSelectionModel()->model());
    qSelection.push_back(QItemSelectionRange(index));
  }

  this->QSelectionModel->select(
    qSelection, QItemSelectionModel::ClearAndSelect | this->qtSelectionFlags());

  this->IgnoreSignals = false;
}
