// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCatalystPauseSimulationReaction.h"

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
pqCatalystPauseSimulationReaction::pqCatalystPauseSimulationReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(parentObject->parent(), SIGNAL(aboutToShow()), this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqCatalystPauseSimulationReaction::setPauseSimulation(bool pause)
{
  vtkSMLiveInsituLinkProxy* proxy = pqLiveInsituManager::instance()->linkProxy();
  if (proxy)
  {
    vtkSMPropertyHelper(proxy, "SimulationPaused").Set(pause);
    proxy->UpdateVTKObjects();
    if (!pause)
    {
      proxy->InvokeCommand("LiveChanged");
    }
  }
}

//-----------------------------------------------------------------------------
void pqCatalystPauseSimulationReaction::updateEnableState(Type type)
{
  bool enabled = false;
  vtkSMLiveInsituLinkProxy* proxy = pqLiveInsituManager::instance()->linkProxy();
  if (proxy &&
    ((type == PAUSE) != (vtkSMPropertyHelper(proxy, "SimulationPaused").GetAs<int>() == 1)))
  {
    enabled = true;
  }
  this->parentAction()->setEnabled(enabled);
}
