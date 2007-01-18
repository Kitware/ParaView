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
#include <QLineEdit>
#include <QComboBox>

// SM
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"

// pqCore
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"
#include "pqRenderViewModule.h"
#include "pqPipelineSource.h"

// pqComponents
#include "pqLinksModel.h"
#include "ui_pqLinksEditor.h"
#include "ui_pqProxyLink.h"
#include "ui_pqPropertyLink.h"



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

pqLinksEditor::pqLinksEditor(QWidget* p)
  : QDialog(p)
{
  this->setupUi(this);
  this->Model = new pqLinksModel(this);
  this->tableView->setModel(this->Model);
  QObject::connect(this->tableView, SIGNAL(clicked(const QModelIndex&)),
                   this, SLOT(selectionChanged(const QModelIndex&)));
  QObject::connect(this->addButton, SIGNAL(clicked(bool)),
                   this, SLOT(addLink()));
  QObject::connect(this->editButton, SIGNAL(clicked(bool)),
                   this, SLOT(editLink()));
  QObject::connect(this->removeButton, SIGNAL(clicked(bool)),
                   this, SLOT(removeLink()));
  this->editButton->setEnabled(false);
  this->removeButton->setEnabled(false);
}

pqLinksEditor::~pqLinksEditor()
{
}

void pqLinksEditor::addLink()
{
  QDialog dialog;
  QVBoxLayout* dlayout = new QVBoxLayout(&dialog);
  QHBoxLayout* hlayout = new QHBoxLayout;
  QLineEdit* nameEdit = new QLineEdit(&dialog);
  hlayout->addWidget(nameEdit);
  QComboBox* mode = new QComboBox(&dialog);
  hlayout->addWidget(mode);
  dlayout->addLayout(hlayout);
  QWidget* cont = new QWidget(&dialog);
  dlayout->addWidget(cont);
  QDialogButtonBox* dbuttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                    QDialogButtonBox::Cancel,
                                                    Qt::Horizontal,
                                                    &dialog);
  dlayout->addWidget(dbuttons);

  Ui::pqProxyLink proxyui;
  proxyui.setupUi(cont);
  pqLinksEditorProxyModel* m1 = new pqLinksEditorProxyModel(&dialog);
  pqLinksEditorProxyModel* m2 = new pqLinksEditorProxyModel(&dialog);
  proxyui.Proxy1Tree->setModel(m1);
  proxyui.Proxy2Tree->setModel(m2);
  QObject::connect(dbuttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
  QObject::connect(dbuttons, SIGNAL(rejected()), &dialog, SLOT(reject()));

  if(dialog.exec() == QDialog::Accepted)
    {
    QModelIndex idx1 = proxyui.Proxy1Tree->selectionModel()->currentIndex();
    QModelIndex idx2 = proxyui.Proxy2Tree->selectionModel()->currentIndex();
    pqProxy* p1 = m1->getProxy(idx1);
    pqProxy* p2 = m2->getProxy(idx2);

    this->Model->addProxyLink("camLink", p1, p2);
    }
}

void pqLinksEditor::editLink()
{
}

void pqLinksEditor::removeLink()
{
  QModelIndex idx = this->tableView->selectionModel()->currentIndex();
  if(idx.isValid())
    {
    this->Model->removeLink(idx);
    }
}

void pqLinksEditor::selectionChanged(const QModelIndex& idx)
{
  if(!idx.isValid())
    {
    this->editButton->setEnabled(false);
    this->removeButton->setEnabled(false);
    }
  else
    {
    this->editButton->setEnabled(true);
    this->removeButton->setEnabled(true);
    }
}


