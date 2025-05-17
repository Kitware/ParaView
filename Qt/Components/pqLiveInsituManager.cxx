// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLiveInsituManager.h"

#include <iostream>
#include <limits>

#include <QInputDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QVariant>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTestUtility.h"
#include "pqWidgetEventPlayer.h"

#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <cassert>

// #define pqLiveInsituManagerDebugMacro(x) std::cerr << x << endl;
#define pqLiveInsituManagerDebugMacro(x)

namespace
{
class pqLiveInsituEventPlayer : public pqWidgetEventPlayer
{
  typedef pqWidgetEventPlayer Superclass;

public:
  QPointer<pqLiveInsituManager> Manager;

  pqLiveInsituEventPlayer(QObject* p)
    : Superclass(p)
  {
  }

  using Superclass::playEvent;
  bool playEvent(QObject*, const QString& command, const QString& arguments, bool& error) override
  {
    if (command == "pqLiveInsituManager" && this->Manager)
    {
      if (arguments.startsWith("wait_timestep"))
      {
        QString arg = arguments;
        QString subCommand;
        vtkIdType timeStep;
        QTextStream istr(&arg);
        istr >> subCommand >> timeStep;
        this->Manager->waitTimestep(timeStep);
      }
      else if (arguments == "wait_breakpoint_hit")
      {
        this->Manager->waitBreakpointHit();
      }
      else
      {
        error = true;
      }
      return true;
    }
    else
    {
      return false;
    }
  }
};
};

double pqLiveInsituManager::INVALID_TIME = -std::numeric_limits<double>().max();
vtkIdType pqLiveInsituManager::INVALID_TIME_STEP = std::numeric_limits<vtkIdType>().min();

//-----------------------------------------------------------------------------
pqLiveInsituManager* pqLiveInsituManager::instance()
{
  static pqLiveInsituManager* s_liveInsituManager = nullptr;
  if (!s_liveInsituManager)
  {
    s_liveInsituManager = new pqLiveInsituManager();
  }
  return s_liveInsituManager;
}

//-----------------------------------------------------------------------------
pqLiveInsituManager::pqLiveInsituManager()
  : BreakpointTime(INVALID_TIME)
  , Time(INVALID_TIME)
  , BreakpointTimeStep(INVALID_TIME_STEP)
  , TimeStep(INVALID_TIME_STEP)
{
  pqLiveInsituEventPlayer* player = new pqLiveInsituEventPlayer(nullptr);
  player->Manager = this;
  // the testUtility takes ownership of the player.
  pqApplicationCore::instance()->testUtility()->eventPlayer()->addWidgetEventPlayer(player);

  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    model, SIGNAL(sourceAdded(pqPipelineSource*)), this, SLOT(onSourceAdded(pqPipelineSource*)));
  QObject::connect(this, SIGNAL(breakpointHit(pqServer*)), this, SLOT(onBreakpointHit(pqServer*)),
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
bool pqLiveInsituManager::isInsituServer(pqServer* server)
{
  return server ? (server->getResource().scheme() == "catalyst") : false;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituManager::isWriterParametersProxy(vtkSMProxy* proxy)
{
  return proxy && strcmp(proxy->GetXMLGroup(), "insitu_writer_parameters") == 0;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituManager::isInsitu(pqProxy* pipelineSource)
{
  pqServer* insituSession = pqLiveInsituManager::instance()->selectedInsituServer();
  if (!insituSession)
  {
    return false;
  }
  return insituSession->session() == pipelineSource->getProxy()->GetSession();
}

//-----------------------------------------------------------------------------
bool pqLiveInsituManager::isDisplayServer(pqServer* server)
{
  return this->Managers.contains(server);
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituManager::selectedInsituServer()
{
  pqActiveObjects& ao = pqActiveObjects::instance();
  pqServer* as = ao.activeServer();
  if (pqLiveInsituManager::isInsituServer(as))
  {
    return as;
  }
  else if (this->isDisplayServer(as))
  {
    return managerFromDisplay(as)->insituSession();
  }
  else
  {
    pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqServer*> servers = smModel->findItems<pqServer*>();
    for (QList<pqServer*>::Iterator it = servers.begin(); it != servers.end(); ++it)
    {
      pqServer* server = *it;
      if (pqLiveInsituManager::isInsituServer(server))
      {
        return server;
      }
    }
    return nullptr;
  }
}

//-----------------------------------------------------------------------------
vtkSMLiveInsituLinkProxy* pqLiveInsituManager::linkProxy(pqServer* insituSession)
{
  if (insituSession)
  {
    pqLiveInsituVisualizationManager* mgr = pqLiveInsituManager::managerFromInsitu(insituSession);
    if (mgr)
    {
      vtkSMLiveInsituLinkProxy* proxy = mgr->getProxy();
      return proxy;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager* pqLiveInsituManager::connect(pqServer* server, int portNumber)
{
  if (!server)
  {
    return nullptr;
  }
  ManagersType::iterator it = this->Managers.find(server);
  if (it == this->Managers.end())
  {
    // Make sure we are in multi-server mode
    vtkProcessModule::GetProcessModule()->MultipleSessionsSupportOn();

    bool user_ok = false;
    if (portNumber == -1)
    {
      portNumber = QInputDialog::getInt(pqCoreUtilities::mainWidget(), tr("Catalyst Server Port"),
        tr("Enter the port number to accept connections\nfrom Catalyst on:"), 22222, 1024,
        0x0fffffff, 1, &user_ok);
      if (!user_ok)
      {
        // user cancelled.
        return nullptr;
      }
    }

    pqLiveInsituVisualizationManager* mgr =
      new pqLiveInsituVisualizationManager(portNumber, server);
    QObject::connect(mgr, SIGNAL(insituDisconnected()), this, SLOT(onCatalystDisconnected()));
    this->Managers[server] = mgr;
    QObject::connect(mgr, SIGNAL(insituConnected()), this, SLOT(close()));
    Q_EMIT connectionInitiated(server);
    return mgr;
  }
  else
  {
    qWarning("A Catalyst connection has already been established.");
    return *it;
  }
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager* pqLiveInsituManager::managerFromDisplay(pqServer* displaySession)
{
  return this->Managers[displaySession];
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager* pqLiveInsituManager::managerFromInsitu(pqServer* insituSession)
{
  return qobject_cast<pqLiveInsituVisualizationManager*>(
    insituSession->property("LiveInsituVisualizationManager").value<QObject*>());
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::onCatalystDisconnected()
{
  pqLiveInsituVisualizationManager* mgr =
    qobject_cast<pqLiveInsituVisualizationManager*>(this->sender());
  if (!mgr)
  {
    return;
  }

  QMessageBox::information(pqCoreUtilities::mainWidget(), tr("Catalyst Disconnected"),
    tr("Connection to Catalyst Co-Processor has been terminated involuntarily. "
       "This implies either a communication error, or that the "
       "Catalyst co-processor has terminated. "
       "The Catalyst session will now be cleaned up. "
       "You can start a new one if you want to monitor for additional Catalyst "
       "connection requests."));

  this->closeConnection();
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::closeConnection()
{
  pqServer* catalystServer = this->selectedInsituServer();
  if (!catalystServer)
  {
    return;
  }

  pqLiveInsituVisualizationManager* mgr = pqLiveInsituManager::managerFromInsitu(catalystServer);
  if (!mgr)
  {
    qWarning("Trying to close a connection where there is no LiveInsituManager, abort.");
    return;
  }

  mgr->deleteLater();

  // Remove the mgr from the map, so that we can allow the user to connect to
  // another Catalyst session, if he wants.
  for (ManagersType::iterator iter = this->Managers.begin(); iter != this->Managers.end(); ++iter)
  {
    if (iter.value() == mgr)
    {
      this->Managers.erase(iter);
      break;
    }
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqLiveInsituManager::pipelineSource(pqServer* insituSession)
{
  if (insituSession)
  {
    vtkSMSessionProxyManager* manager = insituSession->proxyManager();
    vtkSMProxy* proxy = manager->GetProxy("sources", "PVTrivialProducer");
    return pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(
      proxy);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::time(pqPipelineSource* pipelineSource, double* time, vtkIdType* timeStep)
{
  if (pipelineSource)
  {
    pqOutputPort* port = pipelineSource->getOutputPort(0);
    vtkPVDataInformation* dataInfo = port->getDataInformation();
    if (dataInfo->GetHasTime())
    {
      *time = dataInfo->GetTime();

      pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
      vtkSMSession* session = pipelineSource->getSourceProxy()->GetSession();
      pqServer* insituSession = model->findServer(session);
      vtkSMLiveInsituLinkProxy* linkProxy = pqLiveInsituManager::linkProxy(insituSession);
      assert(linkProxy);
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
void pqLiveInsituManager::onSourceAdded(pqPipelineSource* source)
{
  vtkSMSourceProxy* sourceProxy = source->getSourceProxy();
  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMSession* session = sourceProxy->GetSession();
  pqServer* server = model->findServer(session);
  if (QString(sourceProxy->GetXMLGroup()) == "sources" &&
    QString(sourceProxy->GetXMLName()) == "PVTrivialProducer" &&
    pqLiveInsituManager::isInsituServer(server))
  {
    QObject::connect(
      source, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(onDataUpdated(pqPipelineSource*)));
  }
}

//-----------------------------------------------------------------------------
bool pqLiveInsituManager::isTimeBreakpointHit() const
{
  return this->Time != INVALID_TIME && this->BreakpointTime != INVALID_TIME &&
    this->BreakpointTime <= this->Time;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituManager::isTimeStepBreakpointHit() const
{
  return this->TimeStep != INVALID_TIME_STEP && this->BreakpointTimeStep != INVALID_TIME_STEP &&
    this->BreakpointTimeStep <= this->TimeStep;
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::onDataUpdated(pqPipelineSource* source)
{
  pqLiveInsituManager::time(source, &this->Time, &this->TimeStep);
  Q_EMIT timeUpdated();
  if (isTimeBreakpointHit() || isTimeStepBreakpointHit())
  {
    pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
    vtkSMSession* session = source->getSourceProxy()->GetSession();
    pqServer* insituSession = model->findServer(session);
    // The server talks to the client using vtkPVSessionBase::NotifyAllClients
    // While vtkSMLiveInsituLinkProxy::LoadState notifies us that the time
    // changed we try to send to the server an updated property. This does not
    // work unless we send this update through the message queue (we wait
    // for LoadState to finish).
    Q_EMIT breakpointHit(insituSession);
  }
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::onBreakpointHit(pqServer* insituSession)
{
  vtkSMLiveInsituLinkProxy* proxy = pqLiveInsituManager::linkProxy(insituSession);
  if (proxy)
  {
    vtkSMPropertyHelper(proxy, "SimulationPaused").Set(true);
    proxy->UpdateVTKObjects();
    this->removeBreakpoint();
  }
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::setBreakpoint(double t)
{
  if (this->BreakpointTime != t)
  {
    pqServer* insituSession = this->selectedInsituServer();
    if (insituSession)
    {
      this->BreakpointTime = t;
      this->BreakpointTimeStep = INVALID_TIME_STEP;
      Q_EMIT breakpointAdded(insituSession);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::setBreakpoint(vtkIdType _timeStep)
{
  if (this->BreakpointTimeStep != _timeStep)
  {
    pqServer* insituSession = this->selectedInsituServer();
    if (insituSession)
    {
      this->BreakpointTime = INVALID_TIME;
      this->BreakpointTimeStep = _timeStep;
      Q_EMIT breakpointAdded(insituSession);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::removeBreakpoint()
{
  if (this->hasBreakpoint())
  {
    pqServer* insituSession = this->selectedInsituServer();
    if (insituSession)
    {
      this->BreakpointTime = INVALID_TIME;
      this->BreakpointTimeStep = INVALID_TIME_STEP;
      Q_EMIT breakpointRemoved(insituSession);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::waitTimestep(vtkIdType ts)
{
  pqEventDispatcher::deferEventsIfBlocked(true);
  pqLiveInsituVisualizationManager* visManager =
    this->managerFromInsitu(this->selectedInsituServer());
  pqLiveInsituManagerDebugMacro("===== start waitTimestep(" << ts << ")"
                                                            << " ===== " << this->timeStep());
  while (ts > this->timeStep())
  {
    QEventLoop loop;
    QObject::connect(visManager, SIGNAL(nextTimestepAvailable()), &loop, SLOT(quit()));
    loop.exec();
    pqLiveInsituManagerDebugMacro("===== waitTimestep(" << ts << ")"
                                                        << " ===== " << this->timeStep());
  }
  pqLiveInsituManagerDebugMacro("===== end waitTimestep(" << ts << ")"
                                                          << " ===== " << this->timeStep());
  pqEventDispatcher::deferEventsIfBlocked(false);
}

//-----------------------------------------------------------------------------
void pqLiveInsituManager::waitBreakpointHit()
{
  pqEventDispatcher::deferEventsIfBlocked(true);
  pqLiveInsituManagerDebugMacro("=== begin waitConnected ===");
  QEventLoop loop;
  QObject::connect(this, SIGNAL(breakpointHit(pqServer*)), &loop, SLOT(quit()));
  loop.exec();
  pqLiveInsituManagerDebugMacro("=== end waitConnected ===");
  pqEventDispatcher::deferEventsIfBlocked(false);
}
