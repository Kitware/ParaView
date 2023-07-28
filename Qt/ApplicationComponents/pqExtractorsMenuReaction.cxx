// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqExtractorsMenuReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqExtractor.h"
#include "pqFiltersMenuReaction.h"
#include "pqMenuReactionUtils.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxyGroupMenuManager.h"
#include "pqProxySelection.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMDocumentation.h"
#include "vtkSMExtractsController.h"
#include "vtkSMInputProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QCoreApplication>
#include <QMenu>

//-----------------------------------------------------------------------------
pqExtractorsMenuReaction::pqExtractorsMenuReaction(
  pqProxyGroupMenuManager* parentObject, bool hideDisabledActions)
  : Superclass(parentObject)
  , HideDisabledActions(hideDisabledActions)
{
  this->Timer.setInterval(10);
  this->Timer.setSingleShot(true);

  auto& activeObjects = pqActiveObjects::instance();
  this->Timer.connect(&activeObjects, SIGNAL(serverChanged(pqServer*)), SLOT(start()));
  this->Timer.connect(&activeObjects, SIGNAL(dataUpdated()), SLOT(start()));
  this->Timer.connect(&activeObjects, SIGNAL(portChanged(pqOutputPort*)), SLOT(start()));
  this->Timer.connect(&activeObjects, SIGNAL(viewChanged(pqView*)), SLOT(start()));
  this->Timer.connect(
    pqApplicationCore::instance(), SIGNAL(forceFilterMenuRefresh()), SLOT(start()));
  this->connect(parentObject->menu(), SIGNAL(aboutToShow()), SLOT(updateEnableState()));

  this->connect(parentObject, SIGNAL(triggered(const QString&, const QString&)),
    SLOT(createExtractor(const QString&, const QString&)));
}

//-----------------------------------------------------------------------------
pqExtractorsMenuReaction::~pqExtractorsMenuReaction() = default;

//-----------------------------------------------------------------------------
void pqExtractorsMenuReaction::updateEnableState(bool)
{
  auto& activeObjects = pqActiveObjects::instance();
  pqServer* server = activeObjects.activeServer();
  bool enabled = (server != nullptr && server->isMaster());

  // Make sure we already have a selection model
  auto selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  enabled = enabled && (selModel != nullptr);

  pqProxySelection selection;
  pqProxySelectionUtilities::copy(selModel, selection);

  std::vector<pqOutputPort*> outputPorts;
  for (const auto& item : selection)
  {
    auto port = qobject_cast<pqOutputPort*>(item);
    auto source = qobject_cast<pqPipelineSource*>(item);
    if (port)
    {
      source = port->getSource();
    }
    else if (source)
    {
      port = source->getOutputPort(0);
    }

    if (source && source->modifiedState() != pqProxy::UNINITIALIZED && port)
    {
      outputPorts.push_back(port);
    }
  }

  std::vector<vtkSMProxy*> ports;
  std::transform(outputPorts.begin(), outputPorts.end(), std::back_inserter(ports),
    [](pqOutputPort* p) { return p->getOutputPortProxy(); });

  auto view = (activeObjects.activeView() ? activeObjects.activeView()->getProxy() : nullptr);

  vtkNew<vtkSMExtractsController> controller;
  auto manager = qobject_cast<pqProxyGroupMenuManager*>(this->parent());
  Q_ASSERT(manager != nullptr);

  auto actionList = manager->actions();
  for (auto& actn : actionList)
  {
    auto prototype = manager->getPrototype(actn);

    // If the action is disabled
    if (prototype == nullptr || !enabled)
    {
      actn->setEnabled(false);
      actn->setVisible(this->HideDisabledActions ? false : true);
      actn->setStatusTip(tr("Requires an input"));
    }

    else if (controller->CanExtract(prototype, ports) || controller->CanExtract(prototype, view))
    {
      actn->setEnabled(true);
      actn->setVisible(true);
      actn->setStatusTip(QCoreApplication::translate(
        "ServerManagerXML", prototype->GetDocumentation()->GetShortHelp()));
    }
    else // Either we do not have an input or the input domain
    {
      actn->setEnabled(false);
      actn->setVisible(this->HideDisabledActions ? false : true);
      if (auto input = pqMenuReactionUtils::getInputProperty(prototype))
      {
        input->RemoveAllUncheckedProxies();
        for (size_t cc = 0; cc < outputPorts.size(); cc++)
        {
          pqOutputPort* port = outputPorts[cc];
          input->AddUncheckedInputConnection(port->getSource()->getProxy(), port->getPortNumber());
        }

        vtkSMDomain* domain = nullptr;
        if (input && !input->IsInDomains(&domain)) // Wrong input domain
        {
          actn->setStatusTip(pqMenuReactionUtils::getDomainDisplayText(domain));
        }
        else // No Input
        {
          actn->setStatusTip(tr("Requires an input"));
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
pqExtractor* pqExtractorsMenuReaction::createExtractor(
  const QString& group, const QString& name) const
{
  auto& activeObjects = pqActiveObjects::instance();
  auto pxm = activeObjects.proxyManager();
  if (!pxm)
  {
    return nullptr;
  }

  auto view = activeObjects.activeView() ? activeObjects.activeView()->getProxy() : nullptr;
  auto port =
    activeObjects.activePort() ? activeObjects.activePort()->getOutputPortProxy() : nullptr;
  auto prototype = pxm->GetPrototypeProxy(group.toUtf8().data(), name.toUtf8().data());

  vtkNew<vtkSMExtractsController> controller;
  vtkSMProxy* input = nullptr;
  if (controller->CanExtract(prototype, port))
  {
    input = port;
  }
  else if (controller->CanExtract(prototype, view))
  {
    input = view;
  }
  else
  {
    return nullptr;
  }

  BEGIN_UNDO_SET(tr("Create Extract Generator '%1'").arg(name));
  auto generator = controller->CreateExtractor(input, name.toUtf8());
  END_UNDO_SET();
  auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
  return generator ? smmodel->findItem<pqExtractor*>(generator) : nullptr;
}
