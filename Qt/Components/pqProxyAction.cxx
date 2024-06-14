// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqProxyAction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqMenuReactionUtils.h"
#include "pqServerManagerModel.h"

#include "vtkSMDocumentation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QAction>
#include <QCoreApplication>

//-----------------------------------------------------------------------------
pqProxyAction::pqProxyAction(QObject* parent, QAction* action)
  : QObject(parent)
  , Action(action)
{
}

//-----------------------------------------------------------------------------
pqProxyAction::~pqProxyAction() = default;

//-----------------------------------------------------------------------------
bool pqProxyAction::IsEnabled()
{
  return this->Action ? this->Action->isEnabled() : false;
}

//-----------------------------------------------------------------------------
QAction* pqProxyAction::GetAction()
{
  return this->Action;
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetDisplayName()
{
  return this->Action ? this->Action->text() : QString();
}

//-----------------------------------------------------------------------------
QIcon pqProxyAction::GetIcon()
{
  return this->Action ? this->Action->icon() : QIcon();
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetProxyDocumentation(QAction* action)
{
  auto prototype = pqProxyAction::GetProxyPrototype(action);
  QString help = prototype->GetDocumentation()->GetShortHelp();

  if (prototype->IsDeprecated())
  {
    help.prepend(tr("(deprecated) "));
  }

  help = help.simplified();
  return QCoreApplication::translate("ServerManagerXML", help.toUtf8());
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetDocumentation()
{
  return pqProxyAction::GetProxyDocumentation(this->Action);
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetProxyName()
{
  return pqProxyAction::GetProxyName(this->Action);
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetProxyGroup()
{
  return pqProxyAction::GetProxyGroup(this->Action);
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetProxyName(QAction* action)
{
  QStringList data_list = action->data().toStringList();
  if (data_list.size() != 2)
  {
    return QString();
  }

  return data_list[1];
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetProxyGroup(QAction* action)
{
  QStringList data_list = action->data().toStringList();
  if (data_list.size() != 2)
  {
    return QString();
  }

  return data_list[0];
}

//-----------------------------------------------------------------------------
QString pqProxyAction::GetRequirement()
{
  pqProxyAction::updateActionsState(QList<QAction*>() << this->Action);
  return this->Action->statusTip();
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxyAction::GetProxyPrototype(QAction* action)
{
  if (!action)
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (!pxm)
  {
    return nullptr;
  }

  auto group = pqProxyAction::GetProxyGroup(action);
  auto name = pqProxyAction::GetProxyName(action);

  return pxm->GetPrototypeProxy(group.toUtf8().data(), name.toUtf8().data());
}

//-----------------------------------------------------------------------------
QList<pqOutputPort*> pqProxyAction::getOutputPorts()
{
  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  QList<pqOutputPort*> outputPorts;
  for (unsigned int cc = 0; cc < selModel->GetNumberOfSelectedProxies(); cc++)
  {
    pqServerManagerModelItem* item =
      smmodel->findItem<pqServerManagerModelItem*>(selModel->GetSelectedProxy(cc));
    pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
    if (opPort)
    {
      source = opPort->getSource();
    }
    else if (source)
    {
      opPort = source->getOutputPort(0);
    }

    // Make sure we still have a valid port, this issue came up with multi-server
    if (opPort)
    {
      outputPorts.append(opPort);
    }
  }

  return outputPorts;
}

//-----------------------------------------------------------------------------
void pqProxyAction::updateActionsState(QList<QAction*> actions)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  pqServer* server = activeObjects->activeServer();
  bool enabled = (server != nullptr);
  enabled = enabled ? server->isMaster() : enabled;

  // Make sure we already have a selection model
  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  enabled = enabled && (selModel != nullptr);

  // selected ports.
  QList<pqOutputPort*> outputPorts;

  // If active proxy is non-existent, then the filters are disabled.
  if (enabled)
  {
    outputPorts = pqProxyAction::getOutputPorts();
  }

  for (pqOutputPort* port : outputPorts)
  {
    auto source = port->getSource();
    if (source && source->modifiedState() == pqProxy::UNINITIALIZED)
    {
      enabled = false;
      // we will update when the active representation updates the data.
      break;
    }
  }

  if (selModel->GetNumberOfSelectedProxies() == 0 || outputPorts.empty())
  {
    enabled = false;
  }

  for (QAction* action : actions)
  {
    pqProxyAction::updateActionStatus(action, enabled, outputPorts);
  }
}

//-----------------------------------------------------------------------------
void pqProxyAction::updateActionStatus(
  QAction* action, bool enabled, const QList<pqOutputPort*>& outputPorts)
{
  vtkSMProxy* prototype = pqProxyAction::GetProxyPrototype(action);
  if (!prototype || !enabled)
  {
    action->setEnabled(false);
    action->setStatusTip(tr("Requires an input"));
    return;
  }

  int numProcs = outputPorts[0]->getServer()->getNumberOfPartitions();
  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(prototype);
  if (sourceProxy &&
    ((sourceProxy->GetProcessSupport() == vtkSMSourceProxy::SINGLE_PROCESS && numProcs > 1) ||
      (sourceProxy->GetProcessSupport() == vtkSMSourceProxy::MULTIPLE_PROCESSES && numProcs == 1)))
  {
    // Skip single process filters when running in multiprocesses and vice
    // versa.
    action->setEnabled(false);
    if (numProcs > 1)
    {
      action->setStatusTip(tr("Not supported in parallel"));
    }
    else
    {
      action->setStatusTip(tr("Supported only in parallel"));
    }

    return;
  }

  vtkSMInputProperty* input = pqMenuReactionUtils::getInputProperty(prototype);
  if (input)
  {
    if (!input->GetMultipleInput() && outputPorts.size() > 1)
    {
      action->setEnabled(false);
      action->setStatusTip(tr("Multiple inputs not supported"));
      return;
    }

    input->RemoveAllUncheckedProxies();
    for (int cc = 0; cc < outputPorts.size(); cc++)
    {
      pqOutputPort* port = outputPorts[cc];
      input->AddUncheckedInputConnection(port->getSource()->getProxy(), port->getPortNumber());
    }

    vtkSMDomain* domain = nullptr;
    if (input->IsInDomains(&domain))
    {
      action->setEnabled(true);
      action->setVisible(true);
      action->setStatusTip(pqProxyAction::GetProxyDocumentation(action));
    }
    else
    {
      action->setEnabled(false);
      // Here we need to go to the domain that returned false and find out why
      // it said the domain criteria wasn't met.
      action->setStatusTip(pqMenuReactionUtils::getDomainDisplayText(domain));
    }
    input->RemoveAllUncheckedProxies();
  }
}
