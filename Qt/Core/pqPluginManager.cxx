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
#include "pqPluginManager.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "vtkPVPluginsInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMPluginLoaderProxy.h"
#include "vtkSMPluginManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkWeakPointer.h"

#include <QCoreApplication>
#include <QPointer>
#include <QSet>
#include <QtDebug>

#include <vtksys/ios/sstream>
class pqPluginManager::pqInternals
{
public:
  QSet<QString> LocalHiddenPlugins;
  QSet<QString> RemoteHiddenPlugins;
  QList<QPointer<pqServer> > Servers;

  QString getXML(vtkPVPluginsInformation* info, bool remote)
    {
    vtksys_ios::ostringstream stream;
    stream << "<?xml version=\"1.0\" ?>\n";
    stream << "<Plugins>\n";
    for (unsigned int cc=0; cc < info->GetNumberOfPlugins(); cc++)
      {
      if ((remote &&
          this->RemoteHiddenPlugins.contains(info->GetPluginFileName(cc))) ||
        (!remote &&
         this->LocalHiddenPlugins.contains(info->GetPluginFileName(cc))))
        {
        continue;
        }

      stream << "  <Plugin name=\"" << info->GetPluginName(cc) << "\""
        << " filename=\"" << info->GetPluginFileName(cc) << "\""
        << " auto_load=\"" << (info->GetAutoLoad(cc)? 1 : 0) << "\" />\n";
      }
    stream << "</Plugins>\n";
    //cout << stream.str().c_str() << endl;
    return QString(stream.str().c_str());
    }
};

//-----------------------------------------------------------------------------
pqPluginManager::pqPluginManager(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();

  // we ensure that the auto-load plugins are loaded before the application
  // realizes that a new server connection has been made.
  // (BUG #12238).
  QObject::connect(smmodel, SIGNAL(serverReady(pqServer*)),
    this, SLOT(loadPluginsFromSettings(pqServer*)));
  QObject::connect(smmodel, SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(onServerDisconnected(pqServer*)));

  // After the new server has been setup, we can validate if the plugin
  // requirements have been met successfully.
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerConnected(pqServer*)));

  // observer plugin loaded events from PluginManager to detect plugins loaded
  // from Python or otherwise.
  vtkSMPluginManager* mgr =
    vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  mgr->AddObserver(vtkSMPluginManager::PluginLoadedEvent,
    this, &pqPluginManager::updatePluginLists);
}

//-----------------------------------------------------------------------------
pqPluginManager::~pqPluginManager()
{
  // save all settings for each open server session.
  foreach (pqServer* server, this->Internals->Servers)
    {
    this->onServerDisconnected(server);
    }
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadPluginsFromSettings()
{
  // Load local plugins information and then load those plugins.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("/PluginsList/Local/%1").arg(
    QCoreApplication::applicationFilePath());
  QString local_plugin_config = settings->value(key).toString();
  if (!local_plugin_config.isEmpty())
    {
    vtkSMProxyManager::GetProxyManager()->GetPluginManager()->
      LoadPluginConfigurationXMLFromString(
        local_plugin_config.toLatin1().data(), NULL, false);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadPluginsFromSettings(pqServer* server)
{
  // Tell the server to load all default-plugins.
  if (server && server->isRemote())
    {
    // locate the xml-config from settings associated with this server and ask
    // the server to parse it.
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QString uri = server->getResource().schemeHostsPorts().toURI();
    QString key = QString("/PluginsList/%1/%2").arg(uri).arg(
      QCoreApplication::applicationFilePath());
    QString remote_plugin_config = settings->value(key).toString();
    // now pass this xml to the vtkPVPluginTracker on the remote
    // processes.
    if (!remote_plugin_config.isEmpty())
      {
      vtkSMProxyManager::GetProxyManager()->GetPluginManager()->
        LoadPluginConfigurationXMLFromString(
          remote_plugin_config.toLatin1().data(), server->session(), true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerConnected(pqServer* server)
{
  this->Internals->Servers.push_back(server);
  this->updatePluginLists();

  // Validate plugins i.e. check plugins that are required on client and server
  // are indeed present on both.
  if (!this->verifyPlugins(server))
    {
    emit this->requiredPluginsNotLoaded(server);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerDisconnected(pqServer* server)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (server && server->isRemote())
    {
    // locate the xml-config from settings associated with this server and ask
    // the server to parse it.
    QString uri = server->getResource().schemeHostsPorts().toURI();
    QString key = QString("/PluginsList/%1/%2").arg(uri).arg(
      QCoreApplication::applicationFilePath());
    settings->setValue(key,
      this->Internals->getXML(this->loadedExtensions(server, true), true));
    }

  // just save the local plugin info to be on the safer side.
  QString key = QString("/PluginsList/Local/%1").arg(
    QCoreApplication::applicationFilePath());
  settings->setValue(key,
    this->Internals->getXML(this->loadedExtensions(server, false), false));

  this->Internals->Servers.removeAll(server);
}

//-----------------------------------------------------------------------------
void pqPluginManager::updatePluginLists()
{
  emit this->pluginsUpdated();
}

//-----------------------------------------------------------------------------
vtkPVPluginsInformation* pqPluginManager::loadedExtensions(
  pqServer* session, bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  return (remote && session && session->isRemote())?
    mgr->GetRemoteInformation(session->session()) :
    mgr->GetLocalInformation();
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadExtension(
  pqServer* server, const QString& lib, QString* vtkNotUsed(errorMsg), bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();

  bool ret_val = false;
  if (remote && server && server->isRemote())
    {
    ret_val = mgr->LoadRemotePlugin(lib.toLatin1().data(), server->session());
    }
  else
    {
    ret_val = mgr->LoadLocalPlugin(lib.toLatin1().data());
    }

  return ret_val? LOADED : NOTLOADED;
}

//-----------------------------------------------------------------------------
QStringList pqPluginManager::pluginPaths(pqServer* session, bool remote)
{
  vtkSMPluginManager* mgr = vtkSMProxyManager::GetProxyManager()->GetPluginManager();
  QString paths = remote?  mgr->GetRemotePluginSearchPaths(session->session()):
    mgr->GetLocalPluginSearchPaths();
  return paths.split(';', QString::SkipEmptyParts);
}

//-----------------------------------------------------------------------------
void pqPluginManager::hidePlugin(const QString& lib, bool remote)
{
  if (remote)
    {
    this->Internals->RemoteHiddenPlugins.insert(lib);
    }
  else
    {
    this->Internals->LocalHiddenPlugins.insert(lib);
    }
}

//-----------------------------------------------------------------------------
bool pqPluginManager::isHidden(const QString& lib, bool remote)
{
  return remote?
    this->Internals->RemoteHiddenPlugins.contains(lib) :
    this->Internals->LocalHiddenPlugins.contains(lib);
}

//-----------------------------------------------------------------------------
bool pqPluginManager::verifyPlugins(pqServer* activeServer)
{
  if (!activeServer  || !activeServer->isRemote())
    {
    // no verification needed for non-remote servers.
    return true;
    }

  vtkPVPluginsInformation* local_info = this->loadedExtensions(activeServer, false);
  vtkPVPluginsInformation* remote_info = this->loadedExtensions(activeServer, true);
  return vtkPVPluginsInformation::PluginRequirementsSatisfied(
      local_info, remote_info);
}
