// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimatableProxyComboBox.h"

// Server Manager Includes.

// Qt Includes.

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqSMProxy.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
pqAnimatableProxyComboBox::pqAnimatableProxyComboBox(QWidget* _parent)
  : Superclass(_parent)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  QList<pqPipelineSource*> sources = smmodel->findItems<pqPipelineSource*>();
  Q_FOREACH (pqPipelineSource* src, sources)
  {
    QVariant p;
    p.setValue(pqSMProxy(src->getProxy()));
    this->addItem(src->getSMName(), p);
  }

  QObject::connect(smmodel, SIGNAL(preSourceRemoved(pqPipelineSource*)), this,
    SLOT(onSourceRemoved(pqPipelineSource*)));

  QObject::connect(
    smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), this, SLOT(onSourceAdded(pqPipelineSource*)));

  QObject::connect(smmodel, SIGNAL(nameChanged(pqServerManagerModelItem*)), this,
    SLOT(onNameChanged(pqServerManagerModelItem*)));

  QObject::connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentSourceChanged(int)));
}

//-----------------------------------------------------------------------------
pqAnimatableProxyComboBox::~pqAnimatableProxyComboBox() = default;

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimatableProxyComboBox::getCurrentProxy() const
{
  pqSMProxy pxy = this->itemData(this->currentIndex()).value<pqSMProxy>();
  return pxy;
}

//-----------------------------------------------------------------------------
void pqAnimatableProxyComboBox::onSourceAdded(pqPipelineSource* src)
{
  QVariant p;
  p.setValue(pqSMProxy(src->getProxy()));
  this->addItem(src->getSMName(), p);
}

//-----------------------------------------------------------------------------
void pqAnimatableProxyComboBox::onSourceRemoved(pqPipelineSource* source)
{
  int index = this->findProxy(source->getProxy());
  if (index != -1)
  {
    this->removeItem(index);
    if (this->count() == 0)
    {
      Q_EMIT this->currentProxyChanged(nullptr);
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimatableProxyComboBox::onNameChanged(pqServerManagerModelItem* item)
{
  pqPipelineSource* src = qobject_cast<pqPipelineSource*>(item);
  if (src)
  {
    int index = this->findProxy(src->getProxy());
    if (index != -1 && src->getSMName() != this->itemText(index))
    {
      QModelIndex midx = this->model()->index(index, 0);
      this->model()->setData(midx, src->getSMName(), Qt::DisplayRole);
    }
  }
}

void pqAnimatableProxyComboBox::onCurrentSourceChanged(int idx)
{
  pqSMProxy pxy = this->itemData(idx).value<pqSMProxy>();
  Q_EMIT this->currentProxyChanged(pxy);
}

void pqAnimatableProxyComboBox::addProxy(int index, const QString& label, vtkSMProxy* pxy)
{
  QVariant p;
  p.setValue(pqSMProxy(pxy));
  this->insertItem(index, label, p);
}

void pqAnimatableProxyComboBox::removeProxy(const QString& label)
{
  int index = this->findText(label);
  if (index != -1)
  {
    this->removeItem(index);
    if (this->count() == 0)
    {
      Q_EMIT this->currentProxyChanged(nullptr);
    }
  }
}

int pqAnimatableProxyComboBox::findProxy(vtkSMProxy* pxy)
{
  int c = this->count();
  for (int i = 0; i < c; i++)
  {
    if (pxy == this->itemData(i).value<pqSMProxy>())
    {
      return i;
    }
  }
  return -1;
}
