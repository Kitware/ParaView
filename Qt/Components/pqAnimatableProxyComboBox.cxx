/*=========================================================================

   Program: ParaView
   Module:    pqAnimatableProxyComboBox.cxx

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

========================================================================*/
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
  foreach (pqPipelineSource* src, sources)
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
pqAnimatableProxyComboBox::~pqAnimatableProxyComboBox()
{
}

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
      Q_EMIT this->currentProxyChanged(NULL);
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
      Q_EMIT this->currentProxyChanged(NULL);
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
