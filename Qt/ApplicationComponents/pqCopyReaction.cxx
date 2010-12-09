/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqCopyReaction.h"
#include "ui_pqCopyReactionDialog.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOutputPort.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqUndoStack.h"
#include "pqActiveObjects.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqCopyReaction::pqCopyReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(sourceChanged(pqPipelineSource*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqCopyReaction::~pqCopyReaction()
{
}

//-----------------------------------------------------------------------------
void pqCopyReaction::updateEnableState()
{
  this->parentAction()->setEnabled(
    pqActiveObjects::instance().activeSource() != NULL);
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy()
{
  pqServerManagerSelectionModel* selModel=
    pqApplicationCore::instance()->getSelectionModel();
  pqPipelineSource* activeSource =
    qobject_cast<pqPipelineSource*>(selModel->currentItem());
  pqOutputPort* activePort = qobject_cast<pqOutputPort*>(
    selModel->currentItem());
  if (activePort)
    {
    activeSource = activePort->getSource();
    }
  if (!activeSource)
    {
    qDebug("Could not find an active source to copy to.");
    return;
    }

  QDialog dialog(pqCoreUtilities::mainWidget());
  dialog.setObjectName("CopyProperties");
  Ui::pqCopyReactionDialog ui;
  ui.setupUi(&dialog);
  pqServerManagerModel *smModel =
    pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineModel model(*smModel);
  model.setEditable(false);
  ui.pipelineView->setModel(&model);
  ui.pipelineView->setSelectionMode(pqFlatTreeView::SingleSelection);
  ui.pipelineView->getHeader()->hide();
  // don't show the visibility icons.
  ui.pipelineView->getHeader()->hideSection(1);
  ui.pipelineView->setRootIndex(
    model.getIndexFor(activeSource->getServer()));
  if (dialog.exec() != QDialog::Accepted)
    {
    return;
    }
  QModelIndexList indexes =
    ui.pipelineView->getSelectionModel()->selectedIndexes();
  if (indexes.size() == 1)
    {
    pqServerManagerModelItem* item = model.getItemFor(indexes[0]);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
    if (port)
      {
      source = port->getSource();
      }
    pqCopyReaction::copy(activeSource->getProxy(), source->getProxy(),
      ui.copyInputs->isChecked());
    }
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy(vtkSMProxy* dest, vtkSMProxy* source, bool skip_inputs)
{
  if (dest && source)
    {
    BEGIN_UNDO_SET("Copy Properties");
    if (skip_inputs)
      {
      dest->Copy(source, "vtkSMInputProperty");
      }
    else
      {
      dest->Copy(source);
      }
    dest->UpdateVTKObjects();
    END_UNDO_SET();
    }
}
