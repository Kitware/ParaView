/*=========================================================================

   Program: ParaView
   Module:    pqActiveServer.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqInsituServer.h"

#include <limits>

#include <QInputDialog>
#include <QMessageBox>
#include <QVariant>


#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

double pqInsituServer::INVALID_TIME = - std::numeric_limits<double>().max();
vtkIdType pqInsituServer::INVALID_TIME_STEP =
  std::numeric_limits<vtkIdType>().min();


//-----------------------------------------------------------------------------
pqInsituServer* pqInsituServer::instance()
{
  static pqInsituServer* s_insituServer = NULL;
  if (! s_insituServer)
    {
    s_insituServer = new pqInsituServer();
    }
  return s_insituServer;
}

//-----------------------------------------------------------------------------
pqInsituServer::pqInsituServer() :
  BreakpointTime(INVALID_TIME),
  Time(INVALID_TIME),
  BreakpointTimeStep(INVALID_TIME_STEP),
  TimeStep(INVALID_TIME_STEP)
{
  pqServerManagerModel* model =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(model,
                   SIGNAL(sourceAdded(pqPipelineSource*)),
                   this, SLOT(onSourceAdded(pqPipelineSource*)));
  QObject::connect(this, SIGNAL(breakpointHit(pqServer*)),
                   this, SLOT(onBreakpointHit(pqServer*)),
                   Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
bool pqInsituServer::isInsituServer(pqServer* server)
{
  return server ? (server->getResource().scheme() == "catalyst") : false;
}

//-----------------------------------------------------------------------------
bool pqInsituServer::isDisplayServer(pqServer* server)
{
  return this->Managers.contains(server);
}

//-----------------------------------------------------------------------------
pqServer* pqInsituServer::insituServer()
{
  pqActiveObjects& ao = pqActiveObjects::instance();
  pqServer* as = ao.activeServer();
  if (pqInsituServer::isInsituServer(as))
    {
    return as;
    }
  else if (this->isDisplayServer(as))
    {
    return managerFromDisplay(as)->insituServer();
    }
  else
    {
    pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
    QList<pqServer*> servers = smModel->findItems<pqServer*>();
    for(QList<pqServer*>::Iterator it = servers.begin();
        it != servers.end(); ++it)
      {
      pqServer* server = *it;
      if (pqInsituServer::isInsituServer(server))
        {
        return server;
        }
      }
    return NULL;
    }
}


//-----------------------------------------------------------------------------
vtkSMLiveInsituLinkProxy* pqInsituServer::linkProxy(
  pqServer* insituServer)
{
  if (insituServer)
    {
    pqLiveInsituVisualizationManager* mgr =
      pqInsituServer::managerFromInsitu(insituServer);
    if (mgr)
      {
      vtkSMLiveInsituLinkProxy* proxy = mgr->getProxy();
      return proxy;
      }
    }
  return NULL;
}


//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager* pqInsituServer::connect(pqServer* server)
{
  if (! server)
    {
    return NULL;
    }
  ManagersType::iterator it = this->Managers.find(server);
  if (it == this->Managers.end())
    {
    // Make sure we are in multi-server mode
    vtkProcessModule::GetProcessModule()->MultipleSessionsSupportOn();

    bool user_ok = false;
    int portNumber = QInputDialog::getInt(
      pqCoreUtilities::mainWidget(),
      "Catalyst Server Port",
      "Enter the port number to accept connections \nfrom Catalyst on:",
      22222, 1024, 0x0fffffff, 1, &user_ok);
    if (!user_ok)
      {
      // user cancelled.
      return false;
      }

    pqLiveInsituVisualizationManager* mgr =
      new pqLiveInsituVisualizationManager(portNumber, server);
    QObject::connect(mgr, SIGNAL(catalystDisconnected()),
      this, SLOT(onCatalystDisconnected()));
    this->Managers[server] = mgr;
    QMessageBox::information(pqCoreUtilities::mainWidget(),
      "Ready for Catalyst connections",
      QString("Accepting connections from Catalyst Co-Processor \n"
      "for live-coprocessing on port %1").arg(portNumber));
    emit catalystConnected(server);
    return mgr;
    }
  else
    {
    qWarning("A Catalyst connection has already been established.");
    return *it;
    }
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager* pqInsituServer::managerFromDisplay(
  pqServer* displayServer)
{
  return this->Managers[displayServer];
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager* pqInsituServer::managerFromInsitu(
  pqServer* insituServer)
{
  return qobject_cast<pqLiveInsituVisualizationManager*>(
    insituServer->property("LiveInsituVisualizationManager").value<QObject*>());
}


//-----------------------------------------------------------------------------
void pqInsituServer::onCatalystDisconnected()
{
  pqLiveInsituVisualizationManager* mgr =
    qobject_cast<pqLiveInsituVisualizationManager*>(this->sender());
  if (!mgr)
    {
    return;
    }

  QMessageBox::information(pqCoreUtilities::mainWidget(),
    "Catalyst Disconnected",
    "Connection to Catalyst Co-Processor has been terminated involuntarily. "
    "This implies either a communication error, or that the "
    "Catalyst co-processor has terminated. "
    "The Catalyst session will now be cleaned up. "
    "You can start a new one if you want to monitor for additional Catalyst "
    "connection requests.");

  mgr->deleteLater();

  // Remove the mgr from the map, so that we can allow the user to connect to
  // another Catalyst session, if he wants.
  for (ManagersType::iterator iter = this->Managers.begin();
    iter != this->Managers.end(); ++iter)
    {
    if (iter.value() == mgr)
      {
      this->Managers.erase(iter);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqInsituServer::pipelineSource(pqServer* insituServer)
{
  if (insituServer)
    {
    vtkSMSessionProxyManager* manager = insituServer->proxyManager();
    vtkSMProxy* proxy = manager->GetProxy("sources", "PVTrivialProducer");
    return pqApplicationCore::instance()->getServerManagerModel()->
      findItem<pqPipelineSource*>(proxy);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqInsituServer::time(pqPipelineSource* pipelineSource, double* time,
                          vtkIdType* timeStep)
{
  if (pipelineSource)
    {
    pqOutputPort* port = pipelineSource->getOutputPort(0);
    vtkPVDataInformation* dataInfo = port->getDataInformation();
    if (dataInfo->GetHasTime())
      {
      *time = dataInfo->GetTime();

      pqServerManagerModel* model =
        pqApplicationCore::instance()->getServerManagerModel();
      vtkSMSession* session = pipelineSource->getSourceProxy()->GetSession();
      pqServer* insituServer = model->findServer(session);
      vtkSMLiveInsituLinkProxy* linkProxy = pqInsituServer::linkProxy(insituServer);
      Q_ASSERT (linkProxy);
      *timeStep = linkProxy->GetTimeStep();
      }
    }
  else
    {
    *time = INVALID_TIME;
    *timeStep = 0;
    }
}

//-----------------------------------------------------------------------------
void pqInsituServer::onSourceAdded(pqPipelineSource* source)
{
  vtkSMSourceProxy* sourceProxy = source->getSourceProxy();
  pqServerManagerModel* model =
    pqApplicationCore::instance()->getServerManagerModel();
  vtkSMSession* session = sourceProxy->GetSession();
  pqServer* server = model->findServer(session);
  if (QString(sourceProxy->GetXMLGroup()) == "sources" &&
      QString(sourceProxy->GetXMLName()) == "PVTrivialProducer" &&
      pqInsituServer::isInsituServer (server))
    {
    QObject::connect(source, SIGNAL(dataUpdated(pqPipelineSource*)),
                     this, SLOT(onDataUpdated(pqPipelineSource*)));
    }
}

//-----------------------------------------------------------------------------
bool pqInsituServer::isTimeBreakpointHit() const
{
  return this->Time != INVALID_TIME &&
    this->BreakpointTime != INVALID_TIME &&
    this->BreakpointTime <= this->Time;
}

//-----------------------------------------------------------------------------
bool pqInsituServer::isTimeStepBreakpointHit() const
{
  return this->TimeStep != INVALID_TIME_STEP &&
    this->BreakpointTimeStep != INVALID_TIME_STEP &&
    this->BreakpointTimeStep <= this->TimeStep;
}


//-----------------------------------------------------------------------------
void pqInsituServer::onDataUpdated(pqPipelineSource* source)
{
  pqInsituServer::time(source, &this->Time, &this->TimeStep);
  emit timeUpdated();
  if (isTimeBreakpointHit() || isTimeStepBreakpointHit())
    {
    pqServerManagerModel* model =
      pqApplicationCore::instance()->getServerManagerModel();
    vtkSMSession* session = source->getSourceProxy()->GetSession();
    pqServer* insituServer = model->findServer(session);
    // The server talks to the client using vtkPVSessionBase::NotifyAllClients
    // While vtkSMLiveInsituLinkProxy::LoadState notifies us that the time
    // changed we try to send to the server an updated property. This does not
    // work unless we send this update through the message queue (we wait
    // for LoadState to finish).
    emit breakpointHit(insituServer);
    }
}

//-----------------------------------------------------------------------------
void pqInsituServer::onBreakpointHit(pqServer* insituServer)
{
  vtkSMLiveInsituLinkProxy* proxy = pqInsituServer::linkProxy(insituServer);
  if (proxy)
    {
    vtkSMPropertyHelper(proxy, "SimulationPaused").Set(true);
    proxy->UpdateVTKObjects();
    this->removeBreakpoint();
    }
}

//-----------------------------------------------------------------------------
void pqInsituServer::setBreakpoint(double time)
{
  if (this->BreakpointTime != time)
    {
    pqServer* insituServer = this->insituServer();
    if (insituServer)
      {
      this->BreakpointTime = time;
      this->BreakpointTimeStep = INVALID_TIME_STEP;
      emit breakpointAdded(insituServer);
      }
    }
}

//-----------------------------------------------------------------------------
void pqInsituServer::setBreakpoint(vtkIdType timeStep)
{
  if (this->BreakpointTimeStep != timeStep)
    {
    pqServer* insituServer = this->insituServer();
    if (insituServer)
      {
      this->BreakpointTime = INVALID_TIME;
      this->BreakpointTimeStep = timeStep;
      emit breakpointAdded(insituServer);
      }
    }
}

//-----------------------------------------------------------------------------
void pqInsituServer::removeBreakpoint()
{
  if (this->hasBreakpoint())
    {
    pqServer* insituServer = this->insituServer();
    if (insituServer)
      {
      this->BreakpointTime = INVALID_TIME;
      this->BreakpointTimeStep = INVALID_TIME_STEP;
      emit breakpointRemoved(insituServer);
      }
    }
}
