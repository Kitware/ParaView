/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineBrowser.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineBrowser.cxx
/// \date 4/20/2006

#include "pqPipelineBrowser.h"
#include "pqPipelineBrowserContextMenu.h"
#include "pqFlatTreeView.h"
#include "pqPipelineData.h"
#include "pqPipelineModel.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqPipelineSource.h"
#include "pqServer.h"

#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QVBoxLayout>

#include "vtkSMProxy.h"


pqPipelineBrowser::pqPipelineBrowser(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  this->ListModel = 0;
  this->TreeView = 0;

  // Get the pipeline model from the pipeline data.
  this->ListModel = pqPipelineData::instance()->getModel();

  // Create a flat tree view to display the pipeline.
  this->TreeView = new pqFlatTreeView(this);
  if(this->TreeView)
    {
    this->TreeView->setObjectName("PipelineView");
    this->TreeView->header()->hide();
    this->TreeView->setModel(this->ListModel);
    this->TreeView->installEventFilter(this);

    // Listen to the selection change signals.
    QItemSelectionModel *selection = this->TreeView->selectionModel();
    if(selection)
      {
      connect(selection,
          SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
          this, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
      }

    // Make sure the tree items get expanded when new descendents
    // are added.
    if(this->ListModel)
      {
      connect(this->ListModel, SIGNAL(firstChildAdded(const QModelIndex &)),
          this->TreeView, SLOT(expand(const QModelIndex &)));
      }

    // The tree view should have a context menu based on the selected
    // items. The context menu policy should be set to custom for this
    // behavior.
    this->TreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    new pqPipelineBrowserContextMenu(this);
    }

  // Add the tree view to the layout.
  QVBoxLayout *boxLayout = new QVBoxLayout(this);
  if(boxLayout)
    {
    boxLayout->setMargin(0);
    boxLayout->addWidget(this->TreeView);
    }
}

pqPipelineBrowser::~pqPipelineBrowser()
{
}

bool pqPipelineBrowser::eventFilter(QObject *object, QEvent *e)
{
  if(object == this->TreeView && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    if(keyEvent->key() == Qt::Key_Delete)
      {
      //this->deleteSelected();
      }
    }

  return QWidget::eventFilter(object, e);
}

QItemSelectionModel *pqPipelineBrowser::getSelectionModel() const
{
  if(this->TreeView)
    {
    return this->TreeView->selectionModel();
    }

  return 0;
}

pqPipelineServer *pqPipelineBrowser::getCurrentServer() const
{
  QItemSelectionModel *selectionModel = this->getSelectionModel();
  if(selectionModel && this->ListModel)
    {
    QModelIndex current = selectionModel->currentIndex();
    pqPipelineServer *server = this->ListModel->getServerFor(current);
    if(!server)
      {
      pqPipelineObject *object = this->ListModel->getObjectFor(current);
      if(object)
        {
        server = object->GetServer();
        }
      }

    return server;
    }

  return 0;
}

vtkSMProxy *pqPipelineBrowser::getSelectedProxy() const
{
  QItemSelectionModel *selectionModel = this->getSelectionModel();
  if(selectionModel && this->ListModel)
    {
    return this->ListModel->getProxyFor(selectionModel->currentIndex());
    }

  return 0;
}

vtkSMProxy *pqPipelineBrowser::getNextProxy() const
{
  QItemSelectionModel *selectionModel = this->getSelectionModel();
  if(selectionModel && this->ListModel)
    {
    pqPipelineSource *source = this->ListModel->getSourceFor(
        selectionModel->currentIndex());
    if(source && source->GetOutputCount() == 1)
      {
      source = dynamic_cast<pqPipelineSource *>(source->GetOutput(0));
      if(source)
        {
        return source->GetProxy();
        }
      }
    }

  return 0;
}

void pqPipelineBrowser::selectProxy(vtkSMProxy *proxy)
{
  if(this->ListModel && this->TreeView)
    {
    QModelIndex index = this->ListModel->getIndexFor(proxy);
    this->TreeView->selectionModel()->setCurrentIndex(index,
        QItemSelectionModel::Select | QItemSelectionModel::Current |
        QItemSelectionModel::Clear);
    }
}

void pqPipelineBrowser::selectServer(pqServer *server)
{
  if(this->ListModel && this->TreeView)
    {
    QModelIndex index = this->ListModel->getIndexFor(
        this->ListModel->getServerFor(server));
    this->TreeView->selectionModel()->setCurrentIndex(index,
        QItemSelectionModel::Select | QItemSelectionModel::Current |
        QItemSelectionModel::Clear);
    }
}

void pqPipelineBrowser::changeCurrent(const QModelIndex &current,
    const QModelIndex &)
{
  if(this->ListModel)
    {
    // Get the current item from the model.
    vtkSMProxy *proxy = this->ListModel->getProxyFor(current);
    emit this->proxySelected(proxy);
    }
}


