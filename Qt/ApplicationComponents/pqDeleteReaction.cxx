/*=========================================================================

   Program: ParaView
   Module:    pqDeleteReaction.cxx

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
#include "pqDeleteReaction.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqUndoStack.h"
#include "pqObjectBuilder.h"

#include <QDebug>
#include <QSet>

//-----------------------------------------------------------------------------
pqDeleteReaction::pqDeleteReaction(QAction* parentObject, bool delete_all)
  : Superclass(parentObject)
{
  this->DeleteAll = delete_all;
  if (!this->DeleteAll)
    {
    pqApplicationCore* core = pqApplicationCore::instance();
    QObject::connect(core->getSelectionModel(),
      SIGNAL(selectionChanged(const pqServerManagerSelection&,
          const pqServerManagerSelection&)),
      this, SLOT(updateEnableState()));

    //needed for when you delete the only consumer of an item. The selection
    //signal is emitted before the consumer is removed
    QObject::connect( core->getServerManagerObserver(),
      SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*) ),
      this, SLOT(updateEnableState()) );
    }

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::updateEnableState()
{
  if (this->DeleteAll)
    {
    this->parentAction()->setEnabled(true);
    return;
    }

  this->parentAction()->setEnabled(this->canDeleteSelected());
}

//-----------------------------------------------------------------------------
static void pqDeleteReactionGetSelectedSet(
  const pqServerManagerSelection& selection,
  QSet<pqPipelineSource*>& selectedSources)
{
  foreach (pqServerManagerModelItem* item, selection)
    {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port? port->getSource():
      qobject_cast<pqPipelineSource*>(item);
    if (source)
      {
      selectedSources.insert(source);
      }
    }
}

//-----------------------------------------------------------------------------
bool pqDeleteReaction::canDeleteSelected()
{
  pqServerManagerSelectionModel* selModel=
    pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection& selection = *(selModel->selectedItems());
  if (selection.size() == 0)
    {
    return false;
    }

  QSet<pqPipelineSource*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selection, selectedSources);

  if (selectedSources.size() == 0)
    {
    return false;
    }

  // Now ensure that all consumers for the current sources don't have consumers
  // outside the selectedSources, then alone can we delete the selected items.
  foreach (pqPipelineSource* source, selectedSources)
    {
    QList<pqPipelineSource*> consumers = source->getAllConsumers();
    for (int cc=0; cc < consumers.size(); cc++)
      {
      pqPipelineSource* consumer = consumers[cc];
      if (consumer && !selectedSources.contains(consumer))
        {
        return false;
        }
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteAll()
{
  BEGIN_UNDO_SET("Delete All");
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroyPipelineProxies();
  END_UNDO_SET();
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqDeleteReaction::deleteSelected()
{
  if (!pqDeleteReaction::canDeleteSelected())
    {
    qCritical() << "Cannot delete selected ";
    return;
    }

  pqServerManagerSelectionModel* selModel=
    pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection& selection = *(selModel->selectedItems());
  QSet<pqPipelineSource*> selectedSources;
  ::pqDeleteReactionGetSelectedSet(selection, selectedSources);

  if (selectedSources.size() == 1)
    {
    pqPipelineSource* source = (*selectedSources.begin());
    BEGIN_UNDO_SET(QString("Delete %1").arg(source->getSMName()));
    }
  else
    {
    BEGIN_UNDO_SET("Delete Selection");
    }
  while (selectedSources.size() > 0)
    {
    foreach (pqPipelineSource* source, selectedSources)
      {
      if (source && source->getNumberOfConsumers() == 0)
        {
        selectedSources.remove(source);
        pqApplicationCore::instance()->getObjectBuilder()->destroy(source);
        break;
        }
      }
    }
  END_UNDO_SET();
  pqApplicationCore::instance()->render();
}
