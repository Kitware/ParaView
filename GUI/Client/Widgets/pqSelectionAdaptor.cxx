/*=========================================================================

   Program:   ParaQ
   Module:    pqSelectionAdaptor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include <QItemSelectionModel>
#include <QPointer>
#include <QtDebug>

// ParaQ includes.
#include "pqServerManagerSelectionModel.h"

//-----------------------------------------------------------------------------
class pqSelectionAdaptorInternal
{
public:
  QPointer<QItemSelectionModel> QSelectionModel;
  QPointer<pqServerManagerSelectionModel> SMSelectionModel;
  bool IgnoreSignals;
};

//-----------------------------------------------------------------------------
pqSelectionAdaptor::pqSelectionAdaptor(
  QItemSelectionModel* qSelectionModel,
    pqServerManagerSelectionModel* smSelectionModel, QObject* _parent/*=0*/)
: QObject(_parent)
{
  this->Internal = new pqSelectionAdaptorInternal();
  this->Internal->QSelectionModel = qSelectionModel;
  this->Internal->SMSelectionModel = smSelectionModel;
  this->Internal->IgnoreSignals = false;

  QObject::connect(this->Internal->QSelectionModel,
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
    this, SLOT(currentChanged(const QModelIndex&, const QModelIndex& )));

  QObject::connect(this->Internal->QSelectionModel,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    this, SLOT(selectionChanged(const QItemSelection&,
        const QItemSelection&)));

  QObject::connect(this->Internal->SMSelectionModel,
    SIGNAL(currentChanged(pqServerManagerModelItem*)),
    this, SLOT(currentChanged(pqServerManagerModelItem*)));

  QObject::connect(this->Internal->SMSelectionModel,
    SIGNAL(selectionChanged(const pqServerManagerModelSelection&,
        const pqServerManagerModelSelection&)),
    this, SLOT(selectionChanged(const pqServerManagerModelSelection&,
        const pqServerManagerModelSelection&)));
}

//-----------------------------------------------------------------------------
pqSelectionAdaptor::~pqSelectionAdaptor()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QItemSelectionModel* pqSelectionAdaptor::getQSelectionModel() const
{
  return this->Internal->QSelectionModel;
}

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel* pqSelectionAdaptor::getSMSelectionModel() const
{
  return this->Internal->SMSelectionModel;
}
//-----------------------------------------------------------------------------
// Returns the QAbstractItemModel used by the QSelectionModel.
// If QSelectionModel uses a QAbstractProxyModel, this method skips
// over all such proxy models and returns the first non-proxy model 
// encountered.
const QAbstractItemModel* pqSelectionAdaptor::getQModel() const
{
  const QAbstractItemModel* model = this->getQSelectionModel()->model();

  // Pass thru proxy models. 
  const QAbstractProxyModel* proxyModel = 
    qobject_cast<const QAbstractProxyModel*>(model);
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
  const QAbstractProxyModel* proxyModel = 
    qobject_cast<const QAbstractProxyModel*>(model);
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
  const QAbstractProxyModel* proxyModel = 
    qobject_cast<const QAbstractProxyModel*>(model);
  if (!proxyModel)
    {
    return inIndex;
    }

  return proxyModel->mapFromSource(
    this->mapFromSource(inIndex, proxyModel->sourceModel()));
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::currentChanged(const QModelIndex& current,
    const QModelIndex& /*previous*/)
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }
  if (!this->Internal->SMSelectionModel)
    {
    qDebug() << "No SMSelectionModel set.!";
    return;
    }
  this->Internal->IgnoreSignals = true;
  pqServerManagerModelItem* smCurrent = this->mapToSMModel(
    this->mapToSource(current));
  
  pqServerManagerSelectionModel::SelectionFlags command = 
    pqServerManagerSelectionModel::NoUpdate;

  /* This doesn't seem to work well for pipeline browser.
  if (this->Internal->QSelectionModel->isSelected(current))
    {
    command |= pqServerManagerSelectionModel::Select;
    }
  else if (this->Internal->SMSelectionModel->isSelected(smCurrent))
    {
    command |= pqServerManagerSelectionModel::Deselect;
    }
    */
  command |= pqServerManagerSelectionModel::ClearAndSelect;
  this->Internal->SMSelectionModel->setCurrentItem(smCurrent, command);
  this->Internal->IgnoreSignals = false;
}


//-----------------------------------------------------------------------------
void pqSelectionAdaptor::selectionChanged(
  const QItemSelection& selected, const QItemSelection& deselected)
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }
  if (!this->Internal->SMSelectionModel)
    {
    qDebug() << "No SMSelectionModel set.!";
    return;
    }

  pqServerManagerModelSelection smSelected;
  pqServerManagerModelSelection smDeselected;
  const QModelIndexList &sIndexes = selected.indexes();

  foreach (const QModelIndex& index, sIndexes)
    {
    pqServerManagerModelItem* smItem = this->mapToSMModel(
      this->mapToSource(index));
    smSelected.push_back(smItem);
    }

  const QModelIndexList &dIndexes = deselected.indexes();
  foreach (const QModelIndex& index, dIndexes)
    {
    pqServerManagerModelItem* smItem = this->mapToSMModel(
      this->mapToSource(index));
    smDeselected.push_back(smItem);
    }
  this->Internal->IgnoreSignals = true;
  this->Internal->SMSelectionModel->select(smDeselected, 
   pqServerManagerSelectionModel::Deselect);
  this->Internal->SMSelectionModel->select(smSelected,
   pqServerManagerSelectionModel::Select);
  this->Internal->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::currentChanged(
  pqServerManagerModelItem* item)
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }
  if (!this->Internal->QSelectionModel)
    {
    qDebug() << "No QSelectionModel set.!";
    return;
    }

  const QModelIndex& index = this->mapFromSource(
    this->mapFromSMModel(item), this->getQSelectionModel()->model());
  this->Internal->IgnoreSignals = true;
  QItemSelectionModel::SelectionFlags command = 
    QItemSelectionModel::NoUpdate;
  if (this->Internal->SMSelectionModel->isSelected(item))
    {
    command |= QItemSelectionModel::Select;
    }
  else if (this->Internal->QSelectionModel->isSelected(index))
    {
    command |= QItemSelectionModel::Deselect;
    }
  this->Internal->QSelectionModel->setCurrentIndex(index, 
    command | QItemSelectionModel::Rows | QItemSelectionModel::Clear);
  this->Internal->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::selectionChanged(
  const pqServerManagerModelSelection& selected,
    const pqServerManagerModelSelection& deselected)
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }
  QItemSelection qSelected;
  QItemSelection qDeselected;

  foreach (pqServerManagerModelItem* item, selected)
    {
    const QModelIndex& index = this->mapFromSource(
      this->mapFromSMModel(item), this->getQSelectionModel()->model());
    qSelected.push_back(QItemSelectionRange(index));
    }

  foreach(pqServerManagerModelItem* item, deselected )
    {
    const QModelIndex& index = this->mapFromSource(
      this->mapFromSMModel(item), this->getQSelectionModel()->model());
    qDeselected.push_back(QItemSelectionRange(index));
    }
  this->Internal->IgnoreSignals = true;
  this->Internal->QSelectionModel->select(qDeselected,
    QItemSelectionModel::Deselect);

  this->Internal->QSelectionModel->select(qSelected,
    QItemSelectionModel::Select);
  this->Internal->IgnoreSignals = false;
}

