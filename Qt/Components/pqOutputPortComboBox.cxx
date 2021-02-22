/*=========================================================================

   Program: ParaView
   Module:    pqOutputPortComboBox.cxx

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
  foreach (pqPipelineSource* source, sources)
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
