// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCatalystRemoveBreakpointReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkProcessModule.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"

#include <QInputDialog>
#include <QMessageBox>

//-----------------------------------------------------------------------------
pqCatalystRemoveBreakpointReaction::pqCatalystRemoveBreakpointReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(parentObject->parent(), SIGNAL(aboutToShow()), this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqCatalystRemoveBreakpointReaction::onTriggered()
{
  pqLiveInsituManager::instance()->removeBreakpoint();
}

//-----------------------------------------------------------------------------
void pqCatalystRemoveBreakpointReaction::updateEnableState()
{
  pqLiveInsituManager* server = pqLiveInsituManager::instance();
  this->parentAction()->setEnabled(
    (server->linkProxy() && server->breakpointTime() != pqLiveInsituManager::INVALID_TIME) ? true
                                                                                           : false);
}
