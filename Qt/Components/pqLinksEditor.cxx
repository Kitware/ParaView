/*=========================================================================

   Program: ParaView
   Module:    pqLinksEditor.cxx

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

=========================================================================*/

// self
#include "pqLinksEditor.h"

// Qt
#include <QPushButton>

// SM
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMOrderedPropertyIterator.h"

// pqCore
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"
#include "pqRenderViewModule.h"
#include "pqPipelineSource.h"

// pqComponents
#include "pqLinksModel.h"
#include "ui_pqLinksEditor.h"


class pqLinksEditorProxyModel : public QAbstractItemModel
{
public:
  pqLinksEditorProxyModel(QObject* p) : QAbstractItemModel(p)
    {
    }
  ~pqLinksEditorProxyModel()
    {
    }

  QModelIndex index(int row, int column, const QModelIndex& pidx) const
    {
    pqServerManagerModel* m;
    m = pqApplicationCore::instance()->getServerManagerModel();
    if(!pidx.isValid())
      {
      return this->createIndex(row, column);
      }
    return this->createIndex(row, column, reinterpret_cast<void*>(pidx.row()+1));
    }
  
  QModelIndex parent(const QModelIndex& pidx) const
    {
    if(!pidx.isValid() || pidx.internalPointer() == NULL)
      {
      return QModelIndex();
      }
    int row = reinterpret_cast<size_t>(pidx.internalPointer()) - 1;
    return this->createIndex(row, pidx.column());
    }
  
  int rowCount(const QModelIndex& idx) const
    {
    if(!idx.isValid())
      {
      return 2;
      }
    QModelIndex pidx = this->parent(idx);
    if(!pidx.isValid())
      {
      pqServerManagerModel* m;
      m = pqApplicationCore::instance()->getServerManagerModel();
      if(idx.row() == 0)
        {
        return m->getNumberOfRenderModules();
        }
      else if(idx.row() == 1)
        {
        return m->getNumberOfSources();
        }
      }
    return 0;
    }
  
  int columnCount(const QModelIndex& /*idx*/) const
    {
    return 1;
    }
  
  QVariant data(const QModelIndex& idx, int role) const
    {
    if(!idx.isValid())
      {
      return QVariant();
      }
    QModelIndex pidx = this->parent(idx);
    if(!pidx.isValid() && role == Qt::DisplayRole)
      {
      if(idx.row() == 0)
        {
        return "Views";
        }
      else if(idx.row() == 1)
        {
        return "Objects";
        }
      }
    else if(role == Qt::DisplayRole)
      {
      pqProxy* pxy = this->getProxy(idx);
      if(pxy)
        {
        return pxy->getProxyName();
        }
      }
    return QVariant();
    }

  QModelIndex findProxy(pqProxy* pxy)
    {
    QModelIndex parentIdx;
    if(qobject_cast<pqRenderViewModule*>(pxy))
      {
      parentIdx = this->index(0, 0, QModelIndex());
      }
    else if(qobject_cast<pqPipelineSource*>(pxy))
      {
      parentIdx = this->index(1, 0, QModelIndex());
      }
    
    if(parentIdx.isValid())
      {
      int numRows = this->rowCount(parentIdx);
      for(int i=0; i<numRows; i++)
        {
        QModelIndex idx = this->index(i, 0, parentIdx);
        if(this->getProxy(idx) == pxy)
          {
          return idx;
          }
        }
      }

    return QModelIndex();
    }

  pqProxy* getProxy(const QModelIndex& idx) const
    {
    if(!idx.isValid())
      {
      return NULL;
      }
    
    QModelIndex pidx = this->parent(idx);
    if(pidx.isValid())
      {
      pqServerManagerModel* m;
      m = pqApplicationCore::instance()->getServerManagerModel();
      if(pidx.row() == 0)
        {
        return m->getRenderModule(idx.row());
        }
      else if(pidx.row() == 1)
        {
        return m->getPQSource(idx.row());
        }
      }
    return NULL;

    }
};

pqLinksEditor::pqLinksEditor(vtkSMLink* link, QWidget* p)
  : QDialog(p)
{
  this->setupUi(this);

  this->SelectedInputProxy = NULL;
  this->SelectedOutputProxy = NULL;
  
  this->InputProxyModel = new pqLinksEditorProxyModel(this);
  this->OutputProxyModel = new pqLinksEditorProxyModel(this);
  this->ObjectTreeProxy1->setModel(this->InputProxyModel);
  this->ObjectTreeProxy2->setModel(this->OutputProxyModel);
  this->ObjectTreeProperty1->setModel(this->InputProxyModel);
  this->ObjectTreeProperty2->setModel(this->OutputProxyModel);

  QObject::connect(this->ObjectTreeProxy1->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentInputProxyChanged(const QModelIndex&, const QModelIndex&)));
  
  QObject::connect(this->ObjectTreeProperty1->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentInputProxyChanged(const QModelIndex&, const QModelIndex&)));

  QObject::connect(this->ObjectTreeProxy2->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentOutputProxyChanged(const QModelIndex&, const QModelIndex&)));
  
  QObject::connect(this->ObjectTreeProperty2->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentOutputProxyChanged(const QModelIndex&, const QModelIndex&)));
  
  QObject::connect(this->Property1List,
     SIGNAL(currentTextChanged(const QString&)),
     this,
     SLOT(currentInputPropertyChanged(const QString&)));
  
  QObject::connect(this->Property2List,
     SIGNAL(currentTextChanged(const QString&)),
     this,
     SLOT(currentOutputPropertyChanged(const QString&)));
  
  QObject::connect(this->lineEdit,
     SIGNAL(textChanged(const QString&)),
     this,
     SLOT(updateEnabledState()), Qt::QueuedConnection);
  
  QObject::connect(this->comboBox,
     SIGNAL(currentIndexChanged(const QString&)),
     this,
     SLOT(updateEnabledState()), Qt::QueuedConnection);

  pqLinksModel model;
  
  if(link)
    {
    QModelIndex idx = model.findLink(link);
    QItemSelectionModel::SelectionFlags selFlags =
      QItemSelectionModel::ClearAndSelect;
    
    // set the input/output proxies
    if(idx.isValid())
      {
      this->lineEdit->setText(model.getLinkName(idx));

      if(model.getLinkType(idx) == pqLinksModel::Property)
        {
        this->comboBox->setCurrentIndex(1);
        }
      else
        {
        this->comboBox->setCurrentIndex(0);
        }
      
      pqProxy* inputProxy = model.getInputProxy(idx);
      QModelIndex viewIdx = this->InputProxyModel->findProxy(inputProxy);
      if(viewIdx.isValid())
        {
        this->ObjectTreeProxy1->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        this->ObjectTreeProperty1->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        }
      
      pqProxy* outputProxy = model.getOutputProxy(idx);
      viewIdx = this->OutputProxyModel->findProxy(outputProxy);
      if(viewIdx.isValid())
        {
        this->ObjectTreeProxy2->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        this->ObjectTreeProperty2->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        }

      }
    }
  else
    {
    // make a name
    QString newLinkName;
    int index = 0;
    while(newLinkName.isEmpty())
      {
      QString tryName = QString("Link%1").arg(index++);
      if(!model.getLink(tryName))
        {
        newLinkName = tryName;
        }
      }
    this->lineEdit->setText(newLinkName);
    }

  this->updateEnabledState();

}

pqLinksEditor::~pqLinksEditor()
{
}

QString pqLinksEditor::linkName()
{
  return this->lineEdit->text();
}

pqLinksModel::ItemType pqLinksEditor::linkMode()
{
  return this->comboBox->currentIndex() == 0 ? 
    pqLinksModel::Proxy : pqLinksModel::Property ;
}

pqProxy* pqLinksEditor::selectedInputProxy()
{
  return this->SelectedInputProxy;
}

pqProxy* pqLinksEditor::selectedOutputProxy()
{
  return this->SelectedOutputProxy;
}


QString pqLinksEditor::selectedInputProperty()
{
  return this->SelectedInputProperty;
}

QString pqLinksEditor::selectedOutputProperty()
{
  return this->SelectedOutputProperty;
}


void pqLinksEditor::currentInputProxyChanged(const QModelIndex& cur,
                                         const QModelIndex&)
{
  this->SelectedInputProxy = this->InputProxyModel->getProxy(cur);
  if(this->linkMode() == pqLinksModel::Property)
    {
    this->updatePropertyList(this->Property1List, this->SelectedInputProxy);
    }
  this->updateEnabledState();
}

void pqLinksEditor::currentOutputProxyChanged(const QModelIndex& cur,
                                         const QModelIndex&)
{
  this->SelectedOutputProxy = this->OutputProxyModel->getProxy(cur);
  if(this->linkMode() == pqLinksModel::Property)
    {
    this->updatePropertyList(this->Property2List, this->SelectedOutputProxy);
    }
  this->updateEnabledState();
}

void pqLinksEditor::updatePropertyList(QListWidget* tw, pqProxy* proxy)
{
  tw->clear();
  if(!proxy)
    {
    return;
    }
  vtkSMOrderedPropertyIterator *iter = vtkSMOrderedPropertyIterator::New();
  iter->SetProxy(proxy->getProxy());
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    tw->addItem(iter->GetKey());
    }
  iter->Delete();
}

void pqLinksEditor::currentInputPropertyChanged(const QString& item)
{
  this->SelectedInputProperty = item;
  this->updateEnabledState();
}

void pqLinksEditor::currentOutputPropertyChanged(const QString& item)
{
  this->SelectedOutputProperty = item;
  this->updateEnabledState();
}

void pqLinksEditor::updateEnabledState()
{
  bool enabled = true;
  if(!this->SelectedInputProxy || !this->SelectedInputProxy ||
     this->linkName().isEmpty())
    {
    enabled = false;
    }
  if(this->linkMode() == pqLinksModel::Property)
    {
    if(this->SelectedInputProperty.isEmpty() ||
       this->SelectedOutputProperty.isEmpty())
      {
      enabled = false;
      }
    }
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

