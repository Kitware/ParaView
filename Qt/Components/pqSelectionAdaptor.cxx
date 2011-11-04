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
#include <QtDebug>
#include <QSet>

// ParaView includes.
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProxy.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
class pqSelectionAdaptor::pqSelectionAdaptorInternal
{
public:
  QPointer<QItemSelectionModel> QSelectionModel;
  vtkSmartPointer<vtkSMProxySelectionModel> SMSelectionModel;
  vtkEventQtSlotConnect* VTKConnect;
  bool IgnoreSignals;

  pqSelectionAdaptorInternal():
    VTKConnect(vtkEventQtSlotConnect::New()),
    IgnoreSignals(false)
  {
  }

  ~pqSelectionAdaptorInternal()
    {
    this->VTKConnect->Delete();
    this->VTKConnect = NULL;
    }
};

//-----------------------------------------------------------------------------
pqSelectionAdaptor::pqSelectionAdaptor(
  QItemSelectionModel* qSelectionModel,
    vtkSMProxySelectionModel* smSelectionModel, QObject* _parent/*=0*/)
: QObject(_parent)
{
  this->Internal = new pqSelectionAdaptorInternal();
  this->Internal->QSelectionModel = qSelectionModel;
  this->Internal->SMSelectionModel = smSelectionModel;

  QObject::connect(this->Internal->QSelectionModel,
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
    this, SLOT(currentChanged(const QModelIndex&)));

  QObject::connect(this->Internal->QSelectionModel,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    this, SLOT(selectionChanged()));

  this->Internal->VTKConnect->Connect(
    this->Internal->SMSelectionModel, vtkCommand::CurrentChangedEvent,
    this, SLOT(currentProxyChanged()));

  this->Internal->VTKConnect->Connect(
    this->Internal->SMSelectionModel, vtkCommand::SelectionChangedEvent,
    this, SLOT(proxySelectionChanged()));
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
vtkSMProxySelectionModel* pqSelectionAdaptor::getProxySelectionModel() const
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
void pqSelectionAdaptor::currentChanged(const QModelIndex& current)
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }

  this->Internal->IgnoreSignals = true;
  vtkSMProxy* smCurrent = this->mapToProxy(this->mapToSource(current));
  
  int command = vtkSMProxySelectionModel::NO_UPDATE;

  // This doesn't seem to work well for pipeline browser.
  if (this->Internal->QSelectionModel->isSelected(current))
    {
    command |= vtkSMProxySelectionModel::SELECT;
    }
  else if (this->Internal->SMSelectionModel->IsSelected(smCurrent))
    {
    command |= vtkSMProxySelectionModel::DESELECT;
    }
  this->Internal->SMSelectionModel->SetCurrentProxy(smCurrent, command);
  this->Internal->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::selectionChanged()
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }

  this->Internal->IgnoreSignals = true;

  QItemSelectionModel* qModel = this->Internal->QSelectionModel;

  QSet<vtkSMProxy*> proxy_set;
  const QModelIndexList &indexes = qModel->selection().indexes();
  foreach (const QModelIndex& index, indexes)
    {
    vtkSMProxy* proxy = this->mapToProxy(this->mapToSource(index));
    if (proxy)
      {
      proxy_set.insert(proxy);
      }
    }

  vtkCollection* collection = vtkCollection::New();
  foreach (vtkSMProxy* proxy, proxy_set)
    {
    collection->AddItem(proxy);
    }

  this->Internal->SMSelectionModel->Select(collection,
    vtkSMProxySelectionModel::CLEAR_AND_SELECT);
  collection->Delete();

  this->Internal->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::currentProxyChanged()
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }

  vtkSMProxy* currentProxy =
    this->Internal->SMSelectionModel->GetCurrentProxy();

  const QModelIndex& index = this->mapFromSource(
    this->mapFromProxy(currentProxy),
    this->getQSelectionModel()->model());

  this->Internal->IgnoreSignals = true;
  QItemSelectionModel::SelectionFlags command = 
    QItemSelectionModel::NoUpdate;
  if (this->Internal->SMSelectionModel->IsSelected(currentProxy))
    {
    command |= QItemSelectionModel::Select;
    }
  else if (this->Internal->QSelectionModel->isSelected(index))
    {
    command |= QItemSelectionModel::Deselect;
    }
  this->Internal->QSelectionModel->setCurrentIndex(index, 
    command | this->qtSelectionFlags());
  this->Internal->IgnoreSignals = false;
}

//-----------------------------------------------------------------------------
void pqSelectionAdaptor::proxySelectionChanged()
{
  if (this->Internal->IgnoreSignals)
    {
    return;
    }
  this->Internal->IgnoreSignals = true;
  QItemSelection qSelected;
  QItemSelection qDeselected;

  vtkCollection* selected =
    this->Internal->SMSelectionModel->GetNewlySelected();
  vtkCollection* deselected =
    this->Internal->SMSelectionModel->GetNewlyDeselected();
  for (int cc=0; cc < selected->GetNumberOfItems(); cc++)
    {
    const QModelIndex& index = this->mapFromSource(
      this->mapFromProxy(vtkSMProxy::SafeDownCast(selected->GetItemAsObject(cc))),
      this->getQSelectionModel()->model());
    qSelected.push_back(QItemSelectionRange(index));
    }

  for (int cc=0; cc < deselected->GetNumberOfItems(); cc++)
    {
    const QModelIndex& index = this->mapFromSource(
      this->mapFromProxy(vtkSMProxy::SafeDownCast(deselected->GetItemAsObject(cc))),
      this->getQSelectionModel()->model());
    qDeselected.push_back(QItemSelectionRange(index));
    }

  this->Internal->QSelectionModel->select(qDeselected,
    QItemSelectionModel::Deselect | this->qtSelectionFlags());

  this->Internal->QSelectionModel->select(qSelected,
    QItemSelectionModel::Select | this->qtSelectionFlags());
  this->Internal->IgnoreSignals = false;
}

