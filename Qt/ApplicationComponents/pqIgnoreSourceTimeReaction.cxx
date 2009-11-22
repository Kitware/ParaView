/*=========================================================================

   Program: ParaView
   Module:    pqIgnoreSourceTimeReaction.cxx

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
#include "pqIgnoreSourceTimeReaction.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqIgnoreSourceTimeReaction::pqIgnoreSourceTimeReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  parentObject->setCheckable(true);

  pqApplicationCore* core = pqApplicationCore::instance();
  QObject::connect(core->getSelectionModel(),
    SIGNAL(selectionChanged(const pqServerManagerSelection&,
        const pqServerManagerSelection&)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::updateEnableState()
{
  pqServerManagerSelectionModel* selModel=
    pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection& selection = *(selModel->selectedItems());
  if (selection.size() < 1)
    {
    this->parentAction()->setEnabled(false);
    return;
    }

  // Decide enable state as well as check state for the action.
  QAction* action = this->parentAction();
  bool prev = action->blockSignals(true);
  bool enabled = true;
  bool checked = false;

  // Now determine the check state for the action.
  foreach (pqServerManagerModelItem* item, selection)
    {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port? port->getSource():
      qobject_cast<pqPipelineSource*>(item);
    if (!source)
      {
      enabled = false;
      break;
      }
    pqTimeKeeper* timekeeper = source->getServer()->getTimeKeeper();
    checked = checked || !timekeeper->isSourceAdded(source);
    if (checked)
      {
      break;
      }
    }
  action->setChecked(checked);
  action->blockSignals(prev);
  action->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::ignoreSourceTime(bool ignore)
{
  BEGIN_UNDO_SET("Toggle Ignore Time");
  pqServerManagerSelectionModel* selModel=
    pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection& selection = *(selModel->selectedItems());

  // Now determine the check state for the action.
  foreach (pqServerManagerModelItem* item, selection)
    {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port? port->getSource():
      qobject_cast<pqPipelineSource*>(item);
    if (!source)
      {
      continue;
      }
    pqIgnoreSourceTimeReaction::ignoreSourceTime(source, ignore);
    }
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::ignoreSourceTime(
  pqPipelineSource* source, bool ignore)
{
  if (!source)
    {
    return;
    }

  pqTimeKeeper* timekeeper = source->getServer()->getTimeKeeper();
  if (ignore)
    {
    timekeeper->removeSource(source);
    }
  else
    {
    timekeeper->addSource(source);
    }
}
