/*=========================================================================

   Program: ParaView
   Module:    pqLinksEditor.cxx

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

// self
#include "pqLinksEditor.h"

// Qt
#include <QPushButton>

// SM
#include "vtkSMPropertyLink.h"
#include "vtkSMProxy.h"
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
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

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
    return QString("String %1").arg(svp->GetNumberOfElements());
    }
  else if(idvp)
    {
    return QString("Id %1").arg(idvp->GetNumberOfElements());
    }
  else if(pp)
    {
    return QString("Proxy %1").arg(pp->GetNumberOfProxies());
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
        return smModel->getNumberOfItems<pqRenderView*>();
        }
      else if(idx.row() == 1)
        {
        return smModel->getNumberOfItems<pqPipelineSource*>();
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
          return m->findItem<pqProxy*>(pxy)->getSMName();
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
        return m->getItemAtIndex<pqRenderView*>(idx.row())->getProxy();
        }
      else if(ri.type == 1)
        {
        if(!ri.hasIndex)
          {
          return m->getItemAtIndex<pqPipelineSource*>(idx.row())->getProxy();
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

  this->SelectedProxy1 = NULL;
  this->SelectedProxy2 = NULL;
  
  this->Proxy1Model = new pqLinksEditorProxyModel(this);
  this->Proxy2Model = new pqLinksEditorProxyModel(this);
  this->ObjectTreeProxy1->setModel(this->Proxy1Model);
  this->ObjectTreeProxy2->setModel(this->Proxy2Model);
  this->ObjectTreeProperty1->setModel(this->Proxy1Model);
  this->ObjectTreeProperty2->setModel(this->Proxy2Model);

  QObject::connect(this->ObjectTreeProxy1->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentProxy1Changed(const QModelIndex&, const QModelIndex&)));
  
  QObject::connect(this->ObjectTreeProperty1->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentProxy1Changed(const QModelIndex&, const QModelIndex&)));

  QObject::connect(this->ObjectTreeProxy2->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentProxy2Changed(const QModelIndex&, const QModelIndex&)));
  
  QObject::connect(this->ObjectTreeProperty2->selectionModel(),
     SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
     this,
     SLOT(currentProxy2Changed(const QModelIndex&, const QModelIndex&)));
  
  QObject::connect(this->Property1List,
     SIGNAL(itemPressed(QListWidgetItem* )),
     this,
     SLOT(currentProperty1Changed(QListWidgetItem* )));
  
  QObject::connect(this->Property2List,
     SIGNAL(itemPressed(QListWidgetItem* )),
     this,
     SLOT(currentProperty2Changed(QListWidgetItem* )));
  
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
      
      vtkSMProxy* inputProxy = model->getProxy1(idx);
      QModelIndex viewIdx = this->Proxy1Model->findProxy(inputProxy);
      if(viewIdx.isValid())
        {
        this->ObjectTreeProxy1->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        this->ObjectTreeProperty1->selectionModel()->
          setCurrentIndex(viewIdx, selFlags);
        }
      
      vtkSMProxy* outputProxy = model->getProxy2(idx);
      viewIdx = this->Proxy2Model->findProxy(outputProxy);
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
        QString prop1 = model->getProperty1(idx);
        int count = this->Property1List->count();
        int i;
        for(i=0; i<count; i++)
          {
          QListWidgetItem* item = this->Property1List->item(i);
          QString d = item->data(Qt::UserRole).toString();
          if(d == prop1)
            {
            this->Property1List->setCurrentItem(item);
            break;
            }
          }

        QString prop2 = model->getProperty2(idx);
        count = this->Property2List->count();
        for(i=0; i<count; i++)
          {
          QListWidgetItem* item = this->Property2List->item(i);
          QString d = item->data(Qt::UserRole).toString();
          if(d == prop2)
            {
            this->Property2List->setCurrentItem(item);
            break;
            }
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

pqLinksModel::ItemType pqLinksEditor::linkType()
{
  return this->comboBox->currentIndex() == 0 ? 
    pqLinksModel::Proxy : pqLinksModel::Property ;
}

vtkSMProxy* pqLinksEditor::selectedProxy1()
{
  return this->SelectedProxy1;
}

vtkSMProxy* pqLinksEditor::selectedProxy2()
{
  return this->SelectedProxy2;
}


QString pqLinksEditor::selectedProperty1()
{
  return this->SelectedProperty1;
}

QString pqLinksEditor::selectedProperty2()
{
  return this->SelectedProperty2;
}


void pqLinksEditor::currentProxy1Changed(const QModelIndex& cur,
                                         const QModelIndex&)
{
  this->SelectedProxy1 = this->Proxy1Model->getProxy(cur);
  if(this->linkType() == pqLinksModel::Property)
    {
    this->updatePropertyList(this->Property1List, this->SelectedProxy1);
    }
  this->updateEnabledState();
}

void pqLinksEditor::currentProxy2Changed(const QModelIndex& cur,
                                         const QModelIndex&)
{
  this->SelectedProxy2 = this->Proxy2Model->getProxy(cur);
  if(this->linkType() == pqLinksModel::Property)
    {
    this->updatePropertyList(this->Property2List, this->SelectedProxy2);
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

void pqLinksEditor::currentProperty1Changed(QListWidgetItem*  item)
{
  this->SelectedProperty1 = item->data(Qt::UserRole).toString();
  this->updateEnabledState();
}

void pqLinksEditor::currentProperty2Changed(QListWidgetItem*  item)
{
  this->SelectedProperty2 = item->data(Qt::UserRole).toString();
  this->updateEnabledState();
}

void pqLinksEditor::updateEnabledState()
{
  bool enabled = true;
  if(!this->SelectedProxy1 || !this->SelectedProxy2 ||
     this->linkName().isEmpty())
    {
    enabled = false;
    }
  if(this->linkType() == pqLinksModel::Property)
    {
    if(this->SelectedProperty1.isEmpty() ||
       this->SelectedProperty2.isEmpty())
      {
      enabled = false;
      }
    // check property types compatible
    if(this->SelectedProxy1 && this->SelectedProxy2)
      {
      vtkSMProperty* p1 =
        this->SelectedProxy1->GetProperty(
          this->SelectedProperty1.toAscii().data());
      vtkSMProperty* p2 =
        this->SelectedProxy2->GetProperty(
          this->SelectedProperty2.toAscii().data());
      if(!p1 || !p2 || propertyType(p1) != propertyType(p2))
        {
        enabled = false;
        }
      }
    }
  this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

