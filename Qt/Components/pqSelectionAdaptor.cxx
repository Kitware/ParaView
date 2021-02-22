/*=========================================================================

   Program: ParaView
   Module:    pqSelectionAdaptor.cxx

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
  foreach (const QModelIndex& index, indexes)
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
  foreach (pqServerManagerModelItem* item, selection)
  {
    const QModelIndex& index =
      this->mapFromSource(this->mapFromItem(item), this->getQSelectionModel()->model());
    qSelection.push_back(QItemSelectionRange(index));
  }

  this->QSelectionModel->select(
    qSelection, QItemSelectionModel::ClearAndSelect | this->qtSelectionFlags());

  this->IgnoreSignals = false;
}
