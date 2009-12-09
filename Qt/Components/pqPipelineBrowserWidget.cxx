/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowserWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqPipelineBrowserWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqOutputPort.h"
#include "pqPipelineModel.h"
#include "pqPipelineModelSelectionAdaptor.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqObjectBuilder.h"

#include <QHeaderView>
#include <QKeyEvent>

//-----------------------------------------------------------------------------
pqPipelineBrowserWidget::pqPipelineBrowserWidget(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->PipelineModel = new pqPipelineModel(this);

  // Initialize pqFlatTreeView.
  this->setModel(this->PipelineModel); 
  this->getHeader()->hide();
  this->getHeader()->moveSection(1, 0);
  this->installEventFilter(this);
  this->setSelectionMode(pqFlatTreeView::ExtendedSelection);

  // Connect the model to the ServerManager model.
  pqServerManagerModel *smModel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqObjectBuilder* builder = 
    pqApplicationCore::instance()->getObjectBuilder();
  QObject::connect(builder, SIGNAL(finishedAddingServer(pqServer*)),
    this->PipelineModel, SLOT(addServer(pqServer*)));
  QObject::connect(smModel, SIGNAL(serverRemoved(pqServer*)),
    this->PipelineModel, SLOT(removeServer(pqServer*)));
  QObject::connect(smModel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this->PipelineModel, SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smModel, SIGNAL(sourceRemoved(pqPipelineSource*)),
    this->PipelineModel, SLOT(removeSource(pqPipelineSource*)));
  QObject::connect(smModel,
    SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
    this->PipelineModel,
    SLOT(addConnection(pqPipelineSource*, pqPipelineSource*, int)));
  QObject::connect(smModel,
    SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
    this->PipelineModel,
    SLOT(removeConnection(pqPipelineSource*, pqPipelineSource*, int)));

  QObject::connect(this, SIGNAL(clicked(const QModelIndex &)),
    this, SLOT(handleIndexClicked(const QModelIndex &)));

  // Use the tree view's font as the base for the model's modified
  // font.
  QFont modifiedFont = this->font();
  modifiedFont.setBold(true);
  this->PipelineModel->setModifiedFont(modifiedFont);

  // Create the selection adaptor.
  new pqPipelineModelSelectionAdaptor(this->getSelectionModel(),
    pqApplicationCore::instance()->getSelectionModel(), this);
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this, SLOT(setActiveView(pqView*)));

  // Make sure the tree items get expanded when new descendents
  // are added.
  QObject::connect(this->PipelineModel, SIGNAL(firstChildAdded(const QModelIndex &)),
      this, SLOT(expand(const QModelIndex &)));
}

//-----------------------------------------------------------------------------
pqPipelineBrowserWidget::~pqPipelineBrowserWidget()
{
}

//-----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setActiveView(pqView* view)
{
  this->PipelineModel->setView(view);
}

//-----------------------------------------------------------------------------
bool pqPipelineBrowserWidget::eventFilter(QObject *object, QEvent *eventArg)
{
  if (object == this && eventArg->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(eventArg);
    if (keyEvent->key() == Qt::Key_Delete ||
        keyEvent->key() == Qt::Key_Backspace)
      {
      emit this->deleteKey();
      }
    }

  return this->Superclass::eventFilter(object, eventArg);
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::handleIndexClicked(const QModelIndex &index)
{
  // we make sure we are only clicking on an eye
  if (index.column() == 1)
    {
    pqDisplayPolicy* display_policy = pqApplicationCore::instance()->getDisplayPolicy();

    // We need to obtain the source to give the undo element some sensible name.
    pqServerManagerModelItem* smModelItem = this->PipelineModel->getItemFor(index);
    pqPipelineSource *source = qobject_cast<pqPipelineSource*>(smModelItem);
    pqOutputPort* port = source? source->getOutputPort(0) :
      qobject_cast<pqOutputPort*>(smModelItem);
    if (port)
      {
      bool new_visibility_state = ! (display_policy->getVisibility(
          pqActiveObjects::instance().activeView(), port) == pqDisplayPolicy::Visible);

      bool is_selected = false;
      QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
      foreach (QModelIndex selIndex, indexes)
        {
        if (selIndex.row() == index.row() && selIndex.parent() == index.parent())
          {
          is_selected = true;
          break;
          }
        }
      if (is_selected)
        {
        this->setVisibility(new_visibility_state, indexes);
        }
      else
        {
        // although there's a selected group of objects, the user clicked on the
        // eye for some other item. In that case, we only affect the clicked
        // item.
        QModelIndexList indexes2;
        indexes2 << index;
        this->setVisibility(new_visibility_state, indexes2);
        // change the selection to the item, if we just made it visible.
        if (new_visibility_state)
          {
          QModelIndex itemIndex = this->PipelineModel->index(index.row(), 0,
            index.parent());
          this->getSelectionModel()->select(itemIndex,
            QItemSelectionModel::ClearAndSelect);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setSelectionVisibility(bool visible)
{
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  this->setVisibility(visible, indexes); 
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setVisibility(bool visible,
  const QModelIndexList& indexes)
{
  pqDisplayPolicy* display_policy = pqApplicationCore::instance()->getDisplayPolicy();

  bool begun_undo_set = false;
  foreach (QModelIndex index, indexes)
    {
    pqServerManagerModelItem* smModelItem = this->PipelineModel->getItemFor(index);
    pqPipelineSource *source = qobject_cast<pqPipelineSource*>(smModelItem);
    pqOutputPort* port = source? source->getOutputPort(0) :
      qobject_cast<pqOutputPort*>(smModelItem);

    if (port)
      {
      if (!begun_undo_set)
        {
        begun_undo_set = true;
        if (indexes.size() == 1)
          {
          source = port->getSource();
          BEGIN_UNDO_SET(QString("%1 %2").arg(visible? "Show" : "Hide").
            arg(source->getSMName()));
          }
        else
          {
          BEGIN_UNDO_SET(QString("%1 Selected").arg(visible? "Show" : "Hide"));
          }
        }
      display_policy->setRepresentationVisibility(
        port, pqActiveObjects::instance().activeView(), visible);
      }
    }
  if (begun_undo_set)
    {
    END_UNDO_SET();
    }
  if (pqActiveObjects::instance().activeView())
    {
    pqActiveObjects::instance().activeView()->render();
    }
}

