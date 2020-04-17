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
#include "pqLiveInsituVisualizationManager.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

#include <QPointer>

#include <cassert>

//-----------------------------------------------------------------------------
class pqLiveInsituVisualizationManager::pqInternals
{
public:
  QPointer<pqServer> InsituSession;
  QPointer<pqServer> DisplaySession;
  vtkWeakPointer<vtkSMLiveInsituLinkProxy> LiveInsituLinkProxy;
  QList<QPointer<pqPipelineSource> > ExtractSourceProxies;
};

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager::pqLiveInsituVisualizationManager(
  int connection_port, pqServer* server)
  : Superclass(server)
  , Internals(new pqInternals())
{
  assert(server != NULL);

  this->Internals->DisplaySession = server;

  pqApplicationCore* core = pqApplicationCore::instance();

  // we need to unregister extracts when the extracts proxy is deleted.
  QObject::connect(core->getServerManagerModel(), SIGNAL(preSourceRemoved(pqPipelineSource*)), this,
    SLOT(sourceRemoved(pqPipelineSource*)));

  vtkSMProxy* proxy =
    core->getObjectBuilder()->createProxy("coprocessing", "LiveInsituLink", server, "coprocessing");

  vtkSMLiveInsituLinkProxy* adaptor = vtkSMLiveInsituLinkProxy::SafeDownCast(proxy);
  assert(adaptor != NULL);

  vtkSMPropertyHelper(adaptor, "InsituPort").Set(connection_port);
  vtkSMPropertyHelper(adaptor, "ProcessType").Set("Visualization");
  adaptor->UpdateVTKObjects();

  // create a new "server session" that acts as the dummy session representing
  // the insitu viz pipeline.
  pqServer* catalyst = core->getObjectBuilder()->createServer(pqServerResource("catalyst:"));
  catalyst->setProperty(
    "LiveInsituVisualizationManager", QVariant::fromValue(static_cast<QObject*>(this)));
  catalyst->setProperty("DisplaySession", QVariant::fromValue<QObject*>(server));
  adaptor->SetInsituProxyManager(catalyst->proxyManager());
  catalyst->setMonitorServerNotifications(true);
  adaptor->InvokeCommand("Initialize");

  pqCoreUtilities::connect(adaptor, vtkCommand::UpdateEvent, this, SLOT(timestepsUpdated()));
  pqCoreUtilities::connect(
    adaptor, vtkCommand::ConnectionClosedEvent, this, SIGNAL(insituDisconnected()));
  pqCoreUtilities::connect(
    adaptor, vtkCommand::ConnectionCreatedEvent, this, SIGNAL(insituConnected()));

  this->Internals->InsituSession = catalyst;
  this->Internals->LiveInsituLinkProxy = adaptor;
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituVisualizationManager::insituSession() const
{
  return this->Internals->InsituSession;
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituVisualizationManager::displaySession() const
{
  return this->Internals->DisplaySession;
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituVisualizationManager::displaySession(pqServer* server)
{
  pqServer* displayServer =
    server ? qobject_cast<pqServer*>(server->property("DisplaySession").value<QObject*>()) : NULL;
  return displayServer ? displayServer : server;
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager::~pqLiveInsituVisualizationManager()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core && this->Internals->InsituSession && core->getObjectBuilder())
  {
    core->getObjectBuilder()->removeServer(this->Internals->InsituSession);
  }

  // Remove the "LiveInsituLink" proxy.
  if (this->Internals->DisplaySession && this->Internals->LiveInsituLinkProxy)
  {
    vtkSMSessionProxyManager* pxm = this->Internals->DisplaySession->proxyManager();
    pxm->UnRegisterProxy("coprocessing",
      pxm->GetProxyName("coprocessing", this->Internals->LiveInsituLinkProxy),
      this->Internals->LiveInsituLinkProxy);
    if (this->Internals->LiveInsituLinkProxy != NULL)
    {
      qWarning("LiveInsituLinkProxy must have been unregistered and deleted by now."
               " Since it wasn't, it would imply a leak.");
    }
    this->Internals->LiveInsituLinkProxy = 0;
  }

  delete this->Internals;
}

//-----------------------------------------------------------------------------
vtkSMLiveInsituLinkProxy* pqLiveInsituVisualizationManager::getProxy() const
{
  return this->Internals->LiveInsituLinkProxy;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituVisualizationManager::hasExtracts(pqOutputPort* port) const
{
  if (port == NULL || port->getServer() != this->Internals->InsituSession ||
    this->Internals->LiveInsituLinkProxy == NULL)
  {
    return false;
  }

  if (this->Internals->LiveInsituLinkProxy->HasExtract(
        port->getSource()->getSMGroup().toLocal8Bit().data(),
        port->getSource()->getSMName().toLocal8Bit().data(), port->getPortNumber()))
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituVisualizationManager::addExtract(pqOutputPort* port)
{
  if (this->hasExtracts(port))
  {
    // don't add another extract for the same object.
    return false;
  }

  pqPipelineSource* source = port->getSource();

  vtkSMProxy* proxy =
    this->Internals->LiveInsituLinkProxy->CreateExtract(source->getSMGroup().toLocal8Bit().data(),
      source->getSMName().toLocal8Bit().data(), port->getPortNumber());

  QString name;
  if (source->getNumberOfOutputPorts() > 1)
  {
    name = QString("Extract: %1 (%2)").arg(source->getSMName()).arg(port->getPortNumber());
  }
  else
  {
    name = QString("Extract: %1").arg(source->getSMName());
  }

  this->Internals->DisplaySession->proxyManager()->RegisterProxy(
    "sources", name.toLocal8Bit().data(), proxy);

  pqPipelineSource* pqproxy =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(proxy);
  assert(pqproxy);
  pqproxy->setModifiedState(pqProxy::UNINITIALIZED);
  pqproxy->setProperty("CATALYST_EXTRACT", true);

  this->Internals->ExtractSourceProxies.push_back(pqproxy);

  // To ensure that the Pipeline browser updates the icon for the port, we fire
  // a bogus event.
  pqProxy::ModifiedState curState = source->modifiedState();
  source->setModifiedState(curState == pqProxy::MODIFIED ? pqProxy::UNMODIFIED : pqProxy::MODIFIED);
  source->setModifiedState(curState);

  // pqActiveObjects::instance().setActiveServer(pqproxy->getServer());
  // this->setRepresentationVisibility(
  //  pqproxy->getOutputPort(0),
  //  pqActiveObjects::instance().activeView(), true);
  return true;
}

//-----------------------------------------------------------------------------
void pqLiveInsituVisualizationManager::sourceRemoved(pqPipelineSource* source)
{
  if (source->getServer() != this->Internals->DisplaySession ||
    !source->property("CATALYST_EXTRACT").toBool() || this->Internals->LiveInsituLinkProxy == NULL)
  {
    return;
  }

  // remove extract.
  this->Internals->LiveInsituLinkProxy->RemoveExtract(source->getProxy());
  this->Internals->ExtractSourceProxies.removeAll(source);
}

//-----------------------------------------------------------------------------
void pqLiveInsituVisualizationManager::timestepsUpdated()
{
  foreach (pqPipelineSource* source, this->Internals->ExtractSourceProxies)
  {
    if (source)
    {
      source->getProxy()->UpdateVTKObjects();
      source->updatePipeline();
      source->setModifiedState(pqProxy::UNMODIFIED);
    }
  }
  this->Internals->ExtractSourceProxies.clear();
  Q_EMIT nextTimestepAvailable();
}
