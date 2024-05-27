// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFiltersMenuReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqChangeInputDialog.h"
#include "pqCollaborationManager.h"
#include "pqCoreUtilities.h"
#include "pqMenuReactionUtils.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqProxyAction.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSourcesMenuReaction.h"
#include "pqUndoStack.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMap>

//-----------------------------------------------------------------------------
pqFiltersMenuReaction::pqFiltersMenuReaction(
  pqProxyGroupMenuManager* menuManager, bool hideDisabledActions)
  : Superclass(menuManager)
  , HideDisabledActions(hideDisabledActions)
{
  QObject::connect(&this->Timer, SIGNAL(timeout()), this, SLOT(setEnableStateDirty()));
  this->Timer.setInterval(10);
  this->Timer.setSingleShot(true);

  QObject::connect(menuManager, SIGNAL(triggered(const QString&, const QString&)), this,
    SLOT(onTriggered(const QString&, const QString&)));

  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)), &this->Timer, SLOT(start()));
  QObject::connect(
    activeObjects, SIGNAL(selectionChanged(pqProxySelection)), &this->Timer, SLOT(start()));
  QObject::connect(activeObjects, SIGNAL(dataUpdated()), &this->Timer, SLOT(start()));
  QObject::connect(pqApplicationCore::instance()->getPluginManager(), SIGNAL(pluginsUpdated()),
    &this->Timer, SLOT(start()));

  QObject::connect(
    pqApplicationCore::instance(), SIGNAL(forceFilterMenuRefresh()), &this->Timer, SLOT(start()));

  QObject::connect(pqApplicationCore::instance(), SIGNAL(updateMasterEnableState(bool)), this,
    SLOT(setEnableStateDirty()));

  QObject::connect(menuManager, SIGNAL(categoriesUpdated()), this, SLOT(setEnableStateDirty()));

  // force the state to compute the first time
  this->IsDirty = true;
  this->updateEnableState(false);
}

//-----------------------------------------------------------------------------
void pqFiltersMenuReaction::setEnableStateDirty()
{
  this->IsDirty = true;
  this->updateEnableState(true);
}

//-----------------------------------------------------------------------------
void pqFiltersMenuReaction::updateEnableState(bool updateOnlyToolbars)
{
  if (!this->IsDirty)
  {
    return;
  }
  // Impossible to validate anything without any SessionProxyManager
  if (!vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager())
  {
    pqTimer::singleShot(100, this, SLOT(updateEnableState())); // Try later
    return;
  }

  pqProxyGroupMenuManager* mgr = static_cast<pqProxyGroupMenuManager*>(this->parent());
  QList<QAction*> actionsList;
  if (updateOnlyToolbars)
  {
    actionsList += mgr->actionsInToolbars();
    const QList<QAction*>& allActions = mgr->actions();
    for (QAction* action : allActions)
    {
      // If the action has a keyboard shortcut it must always be updated.
      if (action->shortcut() != QKeySequence())
      {
        actionsList.append(action);
      }
    }
  }
  else
  {
    actionsList = mgr->actions();
  }

  pqProxyAction::updateActionsState(actionsList);

  for (QAction* action : actionsList)
  {
    if (!action->isEnabled() && this->HideDisabledActions)
    {
      action->setVisible(false);
    }
  }

  // Hide unused submenus
  QMenu* menu = mgr->menu();
  bool anyMenuShown = false;
  QList<QAction*> menuActions = menu->findChildren<QAction*>();
  for (QAction* menuAction : menuActions)
  {
    if (menuAction->isSeparator() || !menuAction->menu() ||
      menuAction->menu() == mgr->getFavoritesMenu())
    {
      continue;
    }

    bool visible = (menuAction->menu() && !menuAction->menu()->isEmpty());
    menuAction->setVisible(visible);
    anyMenuShown = anyMenuShown || visible;

    QList<QAction*> subMenuActions = menuAction->menu()->actions();
  }
  menu->setEnabled(anyMenuShown);

  // If we updated only the toolbars, then the state of other actions may still
  // be dirty
  this->IsDirty = updateOnlyToolbars;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqFiltersMenuReaction::createFilter(
  const QString& xmlgroup, const QString& xmlname)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  if (!pqSourcesMenuReaction::warnOnCreate(xmlgroup, xmlname))
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy(xmlgroup.toUtf8().data(), xmlname.toUtf8().data());
  if (!prototype)
  {
    qCritical() << "Unknown proxy type: " << xmlname;
    return nullptr;
  }

  // Get the list of selected sources.
  QMap<QString, QList<pqOutputPort*>> namedInputs;
  QList<pqOutputPort*> selectedOutputPorts;

  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  // Determine the list of selected output ports.
  for (unsigned int cc = 0; cc < selModel->GetNumberOfSelectedProxies(); cc++)
  {
    pqServerManagerModelItem* item =
      smmodel->findItem<pqServerManagerModelItem*>(selModel->GetSelectedProxy(cc));

    pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
    if (opPort)
    {
      selectedOutputPorts.push_back(opPort);
    }
    else if (source)
    {
      selectedOutputPorts.push_back(source->getOutputPort(0));
    }
  }

  QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
  namedInputs[inputPortNames[0]] = selectedOutputPorts;

  // If the filter has more than 1 input ports, we are simply going to ask the
  // user to make selection for the inputs for each port. We may change that in
  // future to be smarter.
  if (pqPipelineFilter::getRequiredInputPorts(prototype).size() > 1)
  {
    vtkSMProxy* filterProxy = pxm->GetPrototypeProxy("filters", xmlname.toUtf8().data());
    vtkSMPropertyHelper helper(filterProxy, inputPortNames[0]);
    helper.RemoveAllValues();

    for (pqOutputPort* outputPort : selectedOutputPorts)
    {
      helper.Add(outputPort->getSource()->getProxy(), outputPort->getPortNumber());
    }

    pqChangeInputDialog dialog(filterProxy, pqCoreUtilities::mainWidget());
    dialog.setObjectName("SelectInputDialog");
    if (QDialog::Accepted != dialog.exec())
    {
      helper.RemoveAllValues();
      // User aborted creation.
      return nullptr;
    }
    helper.RemoveAllValues();
    namedInputs = dialog.selectedInputs();
  }

  BEGIN_UNDO_SET(tr("Create '%1'").arg(xmlname));
  pqPipelineSource* filter = builder->createFilter("filters", xmlname, namedInputs, server);
  END_UNDO_SET();
  return filter;
}
