// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqOutputPortComboBox.h"

// ParaView Includes.
#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
pqOutputPortComboBox::pqOutputPortComboBox(QWidget* _parent)
  : Superclass(_parent)
{
  pqApplicationCore* core = pqApplicationCore::instance();

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(portChanged(pqOutputPort*)), this,
    SLOT(portChanged(pqOutputPort*)));

  QObject::connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));

  QObject::connect(core->getServerManagerModel(), SIGNAL(sourceAdded(pqPipelineSource*)), this,
    SLOT(addSource(pqPipelineSource*)));

  QObject::connect(core->getServerManagerModel(), SIGNAL(sourceRemoved(pqPipelineSource*)), this,
    SLOT(removeSource(pqPipelineSource*)));
  this->AutoUpdateIndex = true;
}

//-----------------------------------------------------------------------------
pqOutputPortComboBox::~pqOutputPortComboBox() = default;

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::fillExistingPorts()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  QList<pqPipelineSource*> sources = smmodel->findItems<pqPipelineSource*>(nullptr);
  Q_FOREACH (pqPipelineSource* source, sources)
  {
    this->addSource(source);
  }
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::addCustomEntry(const QString& name, pqOutputPort* port)
{
  this->addItem(name, QVariant::fromValue((void*)port));
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::addSource(pqPipelineSource* source)
{
  if (source)
  {
    int numPorts = source->getNumberOfOutputPorts();
    if (numPorts > 1)
    {
      for (int cc = 0; cc < numPorts; cc++)
      {
        pqOutputPort* port = source->getOutputPort(cc);
        this->addItem(QString("%1 (%2)").arg(source->getSMName()).arg(port->getPortName()),
          QVariant::fromValue((void*)port));
      }
    }
    else
    {
      this->addItem(source->getSMName(), QVariant::fromValue((void*)(source->getOutputPort(0))));
    }

    QObject::connect(source, SIGNAL(nameChanged(pqServerManagerModelItem*)), this,
      SLOT(nameChanged(pqServerManagerModelItem*)));
  }
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::removeSource(pqPipelineSource* source)
{
  int numPorts = source->getNumberOfOutputPorts();
  for (int cc = 0; cc < numPorts; cc++)
  {
    int index = this->findData(QVariant::fromValue((void*)(source->getOutputPort(cc))));
    if (index != -1)
    {
      this->removeItem(index);
    }
  }
  QObject::disconnect(source, nullptr, this, nullptr);
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::nameChanged(pqServerManagerModelItem* item)
{
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
  if (!source)
  {
    return;
  }

  int numPorts = source->getNumberOfOutputPorts();
  for (int cc = 0; cc < numPorts; cc++)
  {
    pqOutputPort* port = source->getOutputPort(cc);
    int index = this->findData(QVariant::fromValue((void*)port));
    if (index != -1)
    {
      QString text = source->getSMName();
      if (numPorts > 1)
      {
        text = QString("%1 (%2)").arg(source->getSMName()).arg(port->getPortName());
      }
      bool prev = this->blockSignals(true);
      this->insertItem(index, text, QVariant::fromValue((void*)port));
      this->removeItem(index + 1);
      this->blockSignals(prev);
    }
  }
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::setCurrentPort(pqOutputPort* port)
{
  QVariant _data = QVariant::fromValue((void*)port);
  int index = this->findData(_data);
  if (index != -1)
  {
    this->setCurrentIndex(index);
  }
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::portChanged(pqOutputPort* item)
{
  if (!this->AutoUpdateIndex)
  {
    return;
  }

  this->setCurrentPort(item);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqOutputPortComboBox::currentPort() const
{
  int index = this->currentIndex();
  if (index != -1)
  {
    QVariant _data = this->itemData(index);
    return reinterpret_cast<pqOutputPort*>(_data.value<void*>());
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqOutputPortComboBox::onCurrentIndexChanged(int /*changed*/)
{
  pqOutputPort* port = this->currentPort();
  Q_EMIT this->currentIndexChanged(port);
}
