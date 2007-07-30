/*=========================================================================

   Program: ParaView
   Module:    pqSourceComboBox.cxx

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

========================================================================*/
#include "pqSourceComboBox.h"

// Server Manager Includes.
#include "vtkSMProxy.h"

// Qt Includes.

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"

//-----------------------------------------------------------------------------
pqSourceComboBox::pqSourceComboBox(QWidget* _parent) : Superclass(_parent)
{
  this->UpdateCurrentWithSelection = false;
  this->UpdateSelectionWithCurrent = false;

  QObject::connect(pqApplicationCore::instance()->getSelectionModel(),
    SIGNAL(currentChanged(pqServerManagerModelItem*)),
    this, SLOT(onCurrentChanged(pqServerManagerModelItem*)));
  QObject::connect(this, SIGNAL(currentIndexChanged(int)),
    this, SLOT(onCurrentIndexChanged(int)));
}

//-----------------------------------------------------------------------------
pqSourceComboBox::~pqSourceComboBox()
{
}

//-----------------------------------------------------------------------------
void pqSourceComboBox::onCurrentIndexChanged(int vtkNotUsed(changed))
{
  pqPipelineSource* source = this->currentSource();
  vtkSMProxy* sourceProxy = source? source->getProxy() : 0;
  emit this->currentIndexChanged(source);
  emit this->currentIndexChanged(sourceProxy);
}

//-----------------------------------------------------------------------------
void pqSourceComboBox::addSource(pqPipelineSource* source)
{
  if (source)
    {
    vtkSMProxy* proxy = source->getProxy();
    QVariant data = QVariant(proxy->GetSelfID().ID);
    if (this->findData(data) == -1)
      {
      this->addItem(source->getSMName(), data);
      QObject::connect(source, SIGNAL(nameChanged(pqServerManagerModelItem*)),
        this, SLOT(nameChanged(pqServerManagerModelItem*)));
      emit this->sourceAdded(source);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSourceComboBox::removeSource(pqPipelineSource* source)
{
  int index = this->findData(QVariant(source->getProxy()->GetSelfID().ID));
  if (index != -1)
    {
    this->removeItem(index);
    QObject::disconnect(source, 0, this, 0);
    emit this->sourceRemoved(source);
    }
}

//-----------------------------------------------------------------------------
void pqSourceComboBox::nameChanged(pqServerManagerModelItem* item)
{
  pqPipelineSource* src = qobject_cast<pqPipelineSource*>(item);
  if (src)
    {
    QVariant data = QVariant(src->getProxy()->GetSelfID().ID);
    int index = this->findData(data);
    if ((index != -1) && (src->getSMName() != this->itemText(index)))
      {
      this->blockSignals(true);
      this->insertItem(index, src->getSMName(), data);
      this->removeItem(index+1);
      this->blockSignals(false);
      emit this->renamed(src);
      }
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSourceComboBox::currentSource() const
{
  int index = this->currentIndex();
  pqPipelineSource* source = 0;
  if (index != -1)
    {
    QVariant data = this->itemData(index);
    vtkClientServerID cid (data.value<vtkTypeUInt32>());
    pqServerManagerModel* smmodel = 
      pqApplicationCore::instance()->getServerManagerModel();
    source = smmodel->findItem<pqPipelineSource*>(cid);
    }
  return source;
}

//-----------------------------------------------------------------------------
void pqSourceComboBox::setCurrentSource(pqPipelineSource* source)
{
  if (source)
    {
    QVariant data = QVariant(source->getProxy()->GetSelfID().ID);
    int index = this->findData(data);
    this->setCurrentIndex(index);
    }
}

//-----------------------------------------------------------------------------
void pqSourceComboBox::onCurrentChanged(pqServerManagerModelItem* item)
{
  if (!this->UpdateCurrentWithSelection)
    {
    return;
    }

  pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
  pqProxy* src = opPort? opPort->getSource() : qobject_cast<pqProxy*>(item);
  if (!src)
    {
    // We don't change the selection if the request is to select nothing 
    // at all.
    return;
    }

  QVariant data = QVariant(src->getProxy()->GetSelfID().ID);
  int index = this->findData(data);
  if (index != -1)
    {
    this->setCurrentIndex(index);
    }
}
