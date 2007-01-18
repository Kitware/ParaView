/*=========================================================================

   Program: ParaView
   Module:    pqLinksManager.cxx

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
#include "pqLinksManager.h"

// Qt
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

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
#include "pqLinksEditor.h"

pqLinksManager::pqLinksManager(QWidget* p)
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

pqLinksManager::~pqLinksManager()
{
}

void pqLinksManager::addLink()
{
  pqLinksEditor editor(NULL, this);
  if(editor.exec() == QDialog::Accepted)
    {
    if(editor.linkMode() == pqLinksModel::Proxy)
      {
      this->Model->addProxyLink(editor.linkName(), 
                                editor.selectedInputProxy(),
                                editor.selectedOutputProxy());
      }
    else if(editor.linkMode() == pqLinksModel::Property)
      {
      this->Model->addPropertyLink(editor.linkName(),
                                editor.selectedInputProxy(),
                                editor.selectedInputProperty(),
                                editor.selectedOutputProxy(),
                                editor.selectedOutputProperty());
      }
    }
}

void pqLinksManager::editLink()
{
  QModelIndex idx = this->tableView->selectionModel()->currentIndex();
  vtkSMLink* link = this->Model->getLink(idx);
  pqLinksEditor editor(link, this);
  if(editor.exec() == QDialog::Accepted)
    {
    this->Model->removeLink(idx);
    
    if(editor.linkMode() == pqLinksModel::Proxy)
      {
      this->Model->addProxyLink(editor.linkName(), 
                                editor.selectedInputProxy(),
                                editor.selectedOutputProxy());
      }
    else if(editor.linkMode() == pqLinksModel::Property)
      {
      this->Model->addPropertyLink(editor.linkName(),
                                editor.selectedInputProxy(),
                                editor.selectedInputProperty(),
                                editor.selectedOutputProxy(),
                                editor.selectedOutputProperty());
      }
    }
}

void pqLinksManager::removeLink()
{
  QModelIndex idx = this->tableView->selectionModel()->currentIndex();
  if(idx.isValid())
    {
    this->Model->removeLink(idx);
    }
}

void pqLinksManager::selectionChanged(const QModelIndex& idx)
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


