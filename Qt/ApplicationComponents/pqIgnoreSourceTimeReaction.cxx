// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqIgnoreSourceTimeReaction.h"

#include "pqActiveObjects.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "vtkSMTimeKeeperProxy.h"

//-----------------------------------------------------------------------------
pqIgnoreSourceTimeReaction::pqIgnoreSourceTimeReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  parentObject->setCheckable(true);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::updateEnableState()
{
  if (!pqActiveObjects::instance().activeSource())
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
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  if (!source)
  {
    enabled = false;
  }
  else
  {
    pqTimeKeeper* timeKeeper = source->getServer()->getTimeKeeper();
    checked = checked ||
      // "checked" when the source proxy is not being tracked.
      !vtkSMTimeKeeperProxy::IsTimeSourceTracked(timeKeeper->getProxy(), source->getProxy());
  }
  action->setChecked(checked);
  action->blockSignals(prev);
  action->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::ignoreSourceTime(bool ignore)
{
  BEGIN_UNDO_SET(tr("Toggle Ignore Time"));
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  if (source)
  {
    pqIgnoreSourceTimeReaction::ignoreSourceTime(source, ignore);
  }
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqIgnoreSourceTimeReaction::ignoreSourceTime(pqPipelineSource* source, bool ignore)
{
  if (!source)
  {
    return;
  }

  pqTimeKeeper* timeKeeper = source->getServer()->getTimeKeeper();
  vtkSMTimeKeeperProxy::SetSuppressTimeSource(timeKeeper->getProxy(), source->getProxy(), ignore);
}
