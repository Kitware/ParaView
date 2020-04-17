/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerObserver.cxx

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

#include "pqServerManagerObserver.h"

#include "pqServer.h"
#include "pqXMLUtil.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkObject.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <QList>
#include <QMap>
#include <QtDebug>

//-----------------------------------------------------------------------------
class pqServerManagerObserverInternal
{
public:
  pqServerManagerObserverInternal()
  {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }
  /// Used to listen to proxy manager events.
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqServerManagerObserver::pqServerManagerObserver(QObject* p)
  : QObject(p)
{
  this->Internal = new pqServerManagerObserverInternal();

  // Listen to interesting events from the process module. Since proxy manager
  // is created on per-session basis, we wait to handle the proxy manager events
  // until after a session is created.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->Internal->VTKConnect->Connect(pm, vtkCommand::ConnectionCreatedEvent, this,
    SLOT(connectionCreated(vtkObject*, unsigned long, void*, void*)));
  this->Internal->VTKConnect->Connect(pm, vtkCommand::ConnectionClosedEvent, this,
    SLOT(connectionClosed(vtkObject*, unsigned long, void*, void*)));
}

//-----------------------------------------------------------------------------
pqServerManagerObserver::~pqServerManagerObserver()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::connectionCreated(vtkObject*, unsigned long, void*, void* callData)
{
  vtkIdType sessionId = *reinterpret_cast<vtkIdType*>(callData);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMSession* session = vtkSMSession::SafeDownCast(pm->GetSession(sessionId));
  if (!session)
  {
    // ignore all non-server-manager sessions.
    return;
  }

  // Listen to interesting events from the proxy manager. Every time a new
  // session is created, a new proxy manager is created. So we need to do this
  // initialization of observing event every time.
  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::RegisterEvent, this,
    SLOT(proxyRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);
  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::UnRegisterEvent, this,
    SLOT(proxyUnRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);
  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::LoadStateEvent, this,
    SLOT(stateLoaded(vtkObject*, unsigned long, void*, void*)));
  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::SaveStateEvent, this,
    SLOT(stateSaved(vtkObject*, unsigned long, void*, void*)));
  Q_EMIT this->connectionCreated(sessionId);
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::connectionClosed(vtkObject*, unsigned long, void*, void* callData)
{
  vtkIdType sessionId = *reinterpret_cast<vtkIdType*>(callData);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMSession* session = vtkSMSession::SafeDownCast(pm->GetSession(sessionId));
  if (!session)
  {
    // ignore all non-server-manager sessions.
    return;
  }

  Q_EMIT this->connectionClosed(sessionId);

  vtkSMSessionProxyManager* proxyManager =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  // disconnect all signals from the proxyManager since the proxy manager is
  // going to be destroyed once the session closes.
  this->Internal->VTKConnect->Disconnect(proxyManager);
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::proxyRegistered(
  vtkObject*, unsigned long, void*, void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation* info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);
  if (!info || !this->Internal)
  {
    return;
  }

  if (info->Type == vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION)
  {
    Q_EMIT this->compoundProxyDefinitionRegistered(info->ProxyName);
  }
  else if (info->Type == vtkSMProxyManager::RegisteredProxyInformation::PROXY && info->Proxy)
  {
    Q_EMIT this->proxyRegistered(info->GroupName, info->ProxyName, info->Proxy);
  }
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::proxyUnRegistered(
  vtkObject*, unsigned long, void*, void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation* info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation*>(callData);

  if (!info || !this->Internal)
  {
    return;
  }

  if (info->Type == vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION)
  {
    Q_EMIT this->compoundProxyDefinitionUnRegistered(info->ProxyName);
  }
  else if (info->Type == vtkSMProxyManager::RegisteredProxyInformation::PROXY && info->Proxy)
  {
    Q_EMIT this->proxyUnRegistered(info->GroupName, info->ProxyName, info->Proxy);
  }
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::stateLoaded(vtkObject*, unsigned long, void*, void* callData)
{
  vtkSMProxyManager::LoadStateInformation& info =
    *reinterpret_cast<vtkSMProxyManager::LoadStateInformation*>(callData);
  Q_EMIT this->stateLoaded(info.RootElement, info.ProxyLocator);
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::stateSaved(vtkObject*, unsigned long, void*, void* callData)
{
  vtkSMProxyManager::LoadStateInformation& info =
    *reinterpret_cast<vtkSMProxyManager::LoadStateInformation*>(callData);
  Q_EMIT this->stateSaved(info.RootElement);
}
