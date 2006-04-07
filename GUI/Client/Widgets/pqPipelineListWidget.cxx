/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

/// \file pqPipelineListWidget.cxx
/// \brief
///   The pqPipelineListWidget class is used to display the pipeline
///   in the form of a list.
///
/// \date 11/25/2005

#include "pqPipelineListWidget.h"

#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QTreeView>
#include <QVBoxLayout>
#include <QMenu>
#include <QDialog>

#include "pqPipelineData.h"
#include "pqPipelineListModel.h"
#include "pqPipelineObject.h"
#include "QVTKWidget.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMDisplayProxy.h"
#include "pqDisplayProxyEditor.h"


pqPipelineListWidget::pqPipelineListWidget(QWidget *p)
  : QWidget(p)
{
  this->ListModel = 0;
  this->TreeView = 0;

  // Create the pipeline list model.
  this->ListModel = new pqPipelineListModel(this);
  if(this->ListModel)
    this->ListModel->setObjectName("PipelineList");

  // Create a tree view to display the pipeline.
  this->TreeView = new QTreeView(this);
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

    // Make sure the tree items get expanded when a new
    // sub-item is added.
    if(this->ListModel)
      {
      connect(this->ListModel, SIGNAL(childAdded(const QModelIndex &)),
          this->TreeView, SLOT(expand(const QModelIndex &)));
      }

    this->TreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(this->TreeView, SIGNAL(customContextMenuRequested(const QPoint&)),
                     this, SLOT(doViewContextMenu(const QPoint&)));
    
    }

  // Add the tree view to the layout.
  QVBoxLayout *boxLayout = new QVBoxLayout(this);
  if(boxLayout)
    {
    boxLayout->setMargin(0);
    boxLayout->addWidget(this->TreeView);
    }
}

pqPipelineListWidget::~pqPipelineListWidget()
{
}

bool pqPipelineListWidget::eventFilter(QObject *object, QEvent *e)
{
  if(object == this->TreeView && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    if(keyEvent->key() == Qt::Key_Delete)
      {
      this->deleteSelected();
      }
    }

  return QWidget::eventFilter(object, e);
}

vtkSMProxy *pqPipelineListWidget::getSelectedProxy() const
{
  vtkSMProxy *proxy = 0;
  if(this->ListModel && this->TreeView)
    {
    // Get the current item from the model.
    QModelIndex current = this->TreeView->selectionModel()->currentIndex();
    proxy = this->ListModel->getProxyFor(current);
    }

  return proxy;
}

vtkSMProxy *pqPipelineListWidget::getNextProxy() const
{
  vtkSMProxy *proxy = 0;
  if(this->ListModel && this->TreeView)
    {
    // Get the current item from the model. Make sure the current item
    // is a proxy object.
    QModelIndex current = this->TreeView->selectionModel()->currentIndex();
    if(this->ListModel->getProxyFor(current))
      {
      current = this->ListModel->sibling(current.row() + 1, 0, current);
      proxy = this->ListModel->getProxyFor(current);
      }
    }

  return proxy;
}

QVTKWidget *pqPipelineListWidget::getCurrentWindow() const
{
  QVTKWidget *qvtk = 0;
  if(this->ListModel && this->TreeView)
    {
    // First, check to see if there is a selection.
    QModelIndex current = this->TreeView->selectionModel()->currentIndex();
    if(current.isValid())
      {
      // See if the selected item is a window.
      QWidget *widget = this->ListModel->getWidgetFor(current);
      if(widget)
        qvtk = qobject_cast<QVTKWidget *>(widget);
      else
        {
        // If the selected item is not a window, get it from the
        // proxy's parent window.
        vtkSMProxy *proxy = this->ListModel->getProxyFor(current);
        pqPipelineData *pipeline = pqPipelineData::instance();
        if(proxy && pipeline)
          qvtk = pipeline->getWindowFor(proxy);
        }
      }
    }

  return qvtk;
}

void pqPipelineListWidget::selectProxy(vtkSMProxy *proxy)
{
  if(this->ListModel && this->TreeView)
    {
    QModelIndex index = this->ListModel->getIndexFor(proxy);
    this->TreeView->selectionModel()->setCurrentIndex(index,
        QItemSelectionModel::Select | QItemSelectionModel::Current |
        QItemSelectionModel::Clear);
    }
}

void pqPipelineListWidget::selectWindow(QVTKWidget *win)
{
  if(this->ListModel && this->TreeView)
    {
    QModelIndex index = this->ListModel->getIndexFor(win);
    this->TreeView->selectionModel()->setCurrentIndex(index,
        QItemSelectionModel::Select | QItemSelectionModel::Current |
        QItemSelectionModel::Clear);
    }
}

void pqPipelineListWidget::deleteSelected()
{
  if(!this->ListModel || !this->TreeView)
    {
    return;
    }

  // Get the current index from the tree view.
  QModelIndex current = this->TreeView->selectionModel()->currentIndex();
  pqPipelineListModel::ItemType type = this->ListModel->getTypeFor(current);
  if(type == pqPipelineListModel::Source ||
      type == pqPipelineListModel::Filter ||
      type == pqPipelineListModel::Bundle)
    {
    deleteProxy(this->ListModel->getProxyFor(current));
    }
}

void pqPipelineListWidget::deleteProxy(vtkSMProxy *proxy)
{
  pqPipelineData *pipeline = pqPipelineData::instance();
  if(!proxy || !this->ListModel || !pipeline)
    {
    return;
    }

  pqPipelineObject *object = pipeline->getObjectFor(proxy);
  if(!object)
    {
    return;
    }

  // Make sure there is a valid index for the proxy as well.
  if(!this->ListModel->getIndexFor(proxy).isValid())
    {
    return;
    }

  // Check the connections on the object. If the connections are
  // simple enough, the item can be removed without any re-arranging.
  // Otherwise, send the delete through the regular channel.
  bool canReconnect = false;
  if(object->GetInputCount() == 0)
    {
    canReconnect = object->GetOutputCount() == 1;
    }
  else if(object->GetInputCount() == 1)
    {
    canReconnect = object->GetOutputCount() >= 1;
    }
  else if(object->GetInputCount() > 1)
    {
    canReconnect = object->GetOutputCount() == 1;
    }

  if(canReconnect)
    {
    // Save the connections.
    int i = 0;
    QList<pqPipelineObject *> input;
    QList<pqPipelineObject *> output;
    QList<pqPipelineObject *>::Iterator iter;
    for( ; i < object->GetInputCount(); i++)
      {
      input.append(object->GetInput(i));
      }

    for(i = 0; i < object->GetOutputCount(); i++)
      {
      output.append(object->GetOutput(i));
      }

    // Delete the proxy and reconnect the surrounding proxies.
    this->ListModel->beginDeleteAndConnect();
    pipeline->deleteProxy(proxy);
    if(input.size() == 1)
      {
      vtkSMProxy *inputProxy = input[0]->GetProxy();
      for(iter = output.begin(); iter != output.end(); ++iter)
        {
        pipeline->addInput((*iter)->GetProxy(), inputProxy);
        }
      }
    else if(input.size() > 1)
      {
      vtkSMProxy *outputProxy = output[0]->GetProxy();
      for(iter = input.begin(); iter != input.end(); ++iter)
        {
        pipeline->addInput(outputProxy, (*iter)->GetProxy());
        }
      }

    this->ListModel->finishDeleteAndConnect();
    }
  else
    {
    pipeline->deleteProxy(proxy);
    }
}

void pqPipelineListWidget::changeCurrent(const QModelIndex &current,
                                         const QModelIndex &/*previous*/)
{
  if(this->ListModel)
    {
    // Get the current item from the model.
    vtkSMProxy *proxy = this->ListModel->getProxyFor(current);
    emit this->proxySelected(proxy);
    }
}

void pqPipelineListWidget::doViewContextMenu(const QPoint& position)
{
  // get the current selection, and do a QMenu based on that
  QModelIndex selection = this->TreeView->currentIndex();
  if(selection.isValid())
    {
    pqPipelineListModel::ItemType type = this->ListModel->getTypeFor(selection);
    if(type == pqPipelineListModel::Source ||
       type == pqPipelineListModel::Filter ||
       type == pqPipelineListModel::Bundle)
      {
      pqPipelineObject* po = this->ListModel->getObjectFor(selection);
      vtkSMDisplayProxy* display = po ? po->GetDisplayProxy() : NULL;
      if(display)
        {
        static const char* DisplaySettingString = "Display Settings...";
        QMenu menu;
        menu.addAction(DisplaySettingString);
        QAction* action = menu.exec(this->TreeView->mapToGlobal(position));
        if(action && action->text() == DisplaySettingString)
          {
          QDialog* dialog = new QDialog;
          dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
          dialog->setWindowTitle("Display Settings");
          QHBoxLayout* l = new QHBoxLayout(dialog);
          pqDisplayProxyEditor* editor = new pqDisplayProxyEditor(dialog);
          editor->setDisplayProxy(display, po->GetProxy());
          l->addWidget(editor);
          dialog->show();
          }
        }
      }
    }
}

