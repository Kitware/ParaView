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
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMStringVectorProperty.h"

// pqCore
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"
#include "pqRenderViewModule.h"
#include "pqPipelineSource.h"

// pqComponents
#include "pqLinksModel.h"
#include "ui_pqLinksEditor.h"

static QString propertyType(vtkSMProperty* p)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(p);
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(p);
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(p);
  vtkSMIdTypeVectorProperty* idvp = vtkSMIdTypeVectorProperty::SafeDownCast(p);
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(p);

  if(ivp)
    {
    return QString("Integer %1").arg(ivp->GetNumberOfElements());
    }
  else if(dvp)
    {
    return QString("Real %1").arg(dvp->GetNumberOfElements());
    }
  else if(svp)
    {
    return QString("String %i").arg(svp->GetNumberOfElements());
    }
  else if(idvp)
    {
    return QString("Id %i").arg(idvp->GetNumberOfElements());
    }
  else if(pp)
    {
    return QString("Proxy %i").arg(pp->GetNumberOfProxies());
    }
  return "Unknown";
}

class pqLinksEditorProxyModel : public QAbstractItemModel
{
public:
  pqLinksEditorProxyModel(QObject* p) : QAbstractItemModel(p)
    {
    Q_ASSERT(sizeof(RowIndex) == sizeof(void*));
    }
  ~pqLinksEditorProxyModel()
    {
    }

  struct RowIndex
    {
    RowIndex(int t, bool hi, int i) : type(t), hasIndex(hi), index(i) {}
    size_t type : 7;
    size_t hasIndex : 1;
    size_t index : 8 * (sizeof(size_t) - sizeof(char));
    };
    
  void* encodeIndex(const RowIndex& row) const
    {
    RowIndex ri = row;
    ri.type++;
    return *reinterpret_cast<void**>(&ri);
    }
  
  RowIndex decodeIndex(void* p) const
    {
    RowIndex ri = *reinterpret_cast<RowIndex*>(&p);
    ri.type--;
    return ri;
    }

  QModelIndex index(int row, int column, const QModelIndex& pidx) const
    {
    if(this->rowCount(pidx) <= row)
      {
      return QModelIndex();
      }
    
    if(!pidx.isValid())
      {
      return this->createIndex(row, column);
      }
    RowIndex ri(pidx.row(), false, 0);

    if(pidx.internalPointer() != NULL)
      {
      ri = this->decodeIndex(pidx.internalPointer());
      ri.hasIndex = true;
      ri.index = pidx.row();
      }
    return this->createIndex(row, column, this->encodeIndex(ri));
    }
  
  QModelIndex parent(const QModelIndex& idx) const
    {
    if(!idx.isValid() || idx.internalPointer() == NULL)
      {
      return QModelIndex();
      }
    RowIndex ri = this->decodeIndex(idx.internalPointer());
    int row = ri.type;
    void* p = NULL;
    if(ri.hasIndex)
      {
      row = ri.index;
      RowIndex ri2(ri.type, false, 0);
      p = this->encodeIndex(ri2);
      }

    return this->createIndex(row, idx.column(), p);
    }

  vtkSMProxyListDomain* proxyListDomain(const QModelIndex& idx) const
    {
    vtkSMProxy* pxy = this->getProxy(idx);
    return pqLinksModel::proxyListDomain(pxy);
    }
  
  int rowCount(const QModelIndex& idx) const
    {
    if(!idx.isValid())
      {
      return 2;
      }
    QModelIndex pidx = this->parent(idx);
    pqServerManagerModel* smModel;
    smModel = pqApplicationCore::instance()->getServerManagerModel();

    if(!pidx.isValid())
      {
      if(idx.row() == 0)
        {
        return smModel->getNumberOfRenderModules();
        }
      else if(idx.row() == 1)
        {
        return smModel->getNumberOfSources();
        }
      }
    if(pidx.isValid() && pidx.row() == 1)
      {
      vtkSMProxyListDomain* pxyDomain = this->proxyListDomain(idx);
      if(pxyDomain)
        {
        return pxyDomain->GetNumberOfProxies();
        }
      }
    return 0;
    }
  
  int columnCount(const QModelIndex& /*idx*/) const
    {
    return 1;
    }
  
  QVariant headerData(int, Qt::Orientation, int) const
    {
    return QVariant();
    }
  
  QVariant data(const QModelIndex& idx, int role) const
    {
    if(!idx.isValid())
      {
      return QVariant();
      }
    if(role == Qt::DisplayRole)
      {
      if(idx.internalPointer() == NULL)
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
      
      RowIndex ri = this->decodeIndex(idx.internalPointer());
      if(!ri.hasIndex)
        {
        vtkSMProxy* pxy = this->getProxy(idx);
        pqServerManagerModel* m;
        m = pqApplicationCore::instance()->getServerManagerModel();
        if(pxy)
          {
          return m->getPQProxy(pxy)->getProxyName();
          }
        }
      else
        {
        QModelIndex pidx = this->parent(idx);
        vtkSMProxyListDomain* domain = this->proxyListDomain(pidx);
        if(domain && (int)domain->GetNumberOfProxies() > idx.row())
          {
          return domain->GetProxyName(idx.row());
          }
        }
      }
    return QVariant();
    }

  QModelIndex findProxy(vtkSMProxy* pxy)
    {
    QModelIndexList indexes;
    QModelIndex start = this->index(0, 0, QModelIndex());
    if(start.isValid())
      {
      indexes.append(start);
      }

    while(!indexes.isEmpty())
      {
      QModelIndex idx = indexes.back();
      if(pxy == this->getProxy(idx))
        {
        return idx;
        }

      if(this->rowCount(idx))
        {
        // go to first child
        indexes.append(this->index(0, 0, idx));
        }
      else
        {
        // can this be compacted ?

        QModelIndex sib = idx.sibling(idx.row() + 1, 0);
        // got to next sibling
        if(sib.isValid())
          {
          indexes.removeLast();
          indexes.append(sib);
          }
        // go to parents sibling
        else
          {
          while(!indexes.isEmpty() && !sib.isValid())
            {
            indexes.removeLast();
            if(!indexes.isEmpty())
              {
              QModelIndex pidx = indexes.back();
              sib = pidx.sibling(pidx.row() + 1, 0);
              if(sib.isValid())
                {
                indexes.removeLast();
                indexes.append(sib);
                }
              }
            }
          }
        }
      }
    
    // never found it
    return QModelIndex();
    }

  vtkSMProxy* getProxy(const QModelIndex& idx) const
    {
    if(!idx.isValid())
      {
      return NULL;
      }
    
    QModelIndex pidx = this->parent(idx);
    if(pidx.isValid())
      {
      RowIndex ri = this->decodeIndex(idx.internalPointer());
      pqServerManagerModel* m;
      m = pqApplicationCore::instance()->getServerManagerModel();
      if(ri.type == 0)
        {
        return m->getRenderModule(idx.row())->getProxy();
        }
      else if(ri.type == 1)
        {
        if(!ri.hasIndex)
          {
          return m->getPQSource(idx.row())->getProxy();
          }
        else
          {
          vtkSMProxyListDomain* domain = this->proxyListDomain(pidx);
          if(domain && idx.row() < (int)domain->GetNumberOfProxies())
            {
            return domain->GetProxy(idx.row());
            }
          }
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
     SIGNAL(itemPressed(QListWidgetItem* )),
     this,
     SLOT(currentInputPropertyChanged(QListWidgetItem* )));
  
  QObject::connect(this->Property2List,
     SIGNAL(itemPressed(QListWidgetItem* )),
     this,
     SLOT(currentOutputPropertyChanged(QListWidgetItem* )));
  
  QObject::connect(this->lineEdit,
     SIGNAL(textChanged(const QString&)),
     this,
     SLOT(updateEnabledState()), Qt::QueuedConnection);
  
  QObject::connect(this->comboBox,
     SIGNAL(currentIndexChanged(const QString&)),
     this,
     SLOT(updateEnabledState()), Qt::QueuedConnection);

  pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
  
  if(link)
    {
    QModelIndex idx = model->findLink(link);
    QItemSelectionModel::SelectionFlags selFlags =
      QItemSelectionModel::ClearAndSelect;
    
    // set the input/output proxies
    if(idx.isValid())
      {
      this->lineEdit->setText(model->getLinkName(idx));

      if(model->getLinkType(idx) == pqLinksModel::Property)
        {
        this->comboBox->setCurrentIndex(1);
        }
      else
        {
        this->comboBox->setCurrentIndex(0);
        }
      
      vtkSMProxy* inputProxy = model->getInputProxy(idx);
      QModelIndex viewIdx = this->InputProxyModel->findProxy(inputProxy);
      if(viewIdx.isValid())
        {
        this->ObjectTreeProxy1->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        this->ObjectTreeProperty1->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        }
      
      vtkSMProxy* outputProxy = model->getOutputProxy(idx);
      viewIdx = this->OutputProxyModel->findProxy(outputProxy);
      if(viewIdx.isValid())
        {
        this->ObjectTreeProxy2->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        this->ObjectTreeProperty2->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        }
      
      // if this is a property link, make the properties current
      if(model->getLinkType(idx) == pqLinksModel::Property)
        {
        QString inputProperty = model->getInputProperty(idx);
        QList<QListWidgetItem*> items =
          this->Property1List->findItems(inputProperty, Qt::MatchExactly);
        if(!items.isEmpty())
          {
          this->Property1List->setCurrentItem(items[0]);
          }

        QString outputProperty = model->getOutputProperty(idx);
        items = this->Property2List->findItems(outputProperty, 
          Qt::MatchExactly);
        if(!items.isEmpty())
          {
          this->Property2List->setCurrentItem(items[0]);
          }
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
      if(!model->getLink(tryName))
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

vtkSMProxy* pqLinksEditor::selectedInputProxy()
{
  return this->SelectedInputProxy;
}

vtkSMProxy* pqLinksEditor::selectedOutputProxy()
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

void pqLinksEditor::updatePropertyList(QListWidget* tw, vtkSMProxy* proxy)
{
  tw->clear();
  if(!proxy)
    {
    return;
    }
  vtkSMOrderedPropertyIterator *iter = vtkSMOrderedPropertyIterator::New();
  iter->SetProxy(proxy);
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    QString name = iter->GetKey();
    QString type = propertyType(iter->GetProperty());
    QString propertyLabel = QString("%1 (%2)").arg(name).arg(type);
    QListWidgetItem* item = new QListWidgetItem(propertyLabel, tw);
    item->setData(Qt::UserRole, name);
    }
  iter->Delete();
}

void pqLinksEditor::currentInputPropertyChanged(QListWidgetItem*  item)
{
  this->SelectedInputProperty = item->data(Qt::UserRole).toString();
  this->updateEnabledState();
}

void pqLinksEditor::currentOutputPropertyChanged(QListWidgetItem*  item)
{
  this->SelectedOutputProperty = item->data(Qt::UserRole).toString();
  this->updateEnabledState();
}

void pqLinksEditor::updateEnabledState()
{
  bool enabled = true;
  if(!this->SelectedInputProxy || !this->SelectedOutputProxy ||
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
    // check property types compatible
    if(this->SelectedInputProxy && this->SelectedOutputProxy)
      {
      vtkSMProperty* p1 =
        this->SelectedInputProxy->GetProperty(
          this->SelectedInputProperty.toAscii().data());
      vtkSMProperty* p2 =
        this->SelectedOutputProxy->GetProperty(
          this->SelectedOutputProperty.toAscii().data());
      if(!p1 || !p2 || propertyType(p1) != propertyType(p2))
        {
        enabled = false;
        }
      }
    }
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

