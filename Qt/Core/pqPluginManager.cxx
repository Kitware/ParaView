/*=========================================================================

   Program: ParaView
   Module:    pqPluginManager.cxx

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

#include "pqPluginManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QMessageBox>
#include <QMultiMap>
#include <QPluginLoader>
#include <QResource>

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkIntArray.h"
#include "vtkProcessModule.h"
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVEnvironmentInformationHelper.h"
#include "vtkPVPluginInformation.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPythonModule.h"
#include "vtkStringArray.h"
#include "vtkSMApplication.h"
#include "vtkSMObject.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkToolkits.h"

#include "vtksys/SystemTools.hxx"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "pqApplicationCore.h"
#include "pqAutoStartInterface.h"
#include "pqFileDialogModel.h"
#include "pqObjectBuilder.h"
#include "pqPlugin.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"


//-----------------------------------------------------------------------------
class pqPluginManagerInternal
{
public:
  pqPluginManagerInternal()
    {
    this->IsCurrentServerRemote = false;
    this->NeedUpdatePluginInfo = false;
    }
  ~pqPluginManagerInternal()
    {    
    foreach (QObject* iface, this->Interfaces)
      {
      pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
      if (asi)
        {
        asi->shutdown();
        }
      }

    foreach (QObject* iface, this->ExtraInterfaces)
      {
      pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
      if (asi)
        {
        asi->shutdown();
        }
      }
      
    foreach(vtkPVPluginInformation* plInfo, this->Extensions.values())
      {
      if(plInfo)
        {
        plInfo->Delete();
        }
      }
    this->Extensions.clear();
    }
  QObjectList Interfaces;
  // Map <ServerURI, PluginInfo> for all the plugins loaded or unloaded
  QMultiMap<QString, vtkPVPluginInformation* > Extensions;
  QObjectList ExtraInterfaces;
  vtkSmartPointer<vtkSMPluginManager> SMPluginMananger;
  vtkSmartPointer<vtkEventQtSlotConnect> SMPluginManangerConnect;
  bool IsCurrentServerRemote;
  bool NeedUpdatePluginInfo;
};

//-----------------------------------------------------------------------------
pqPluginManager::pqPluginManager(QObject* p)
  : QObject(p)
{
  this->Internal = new pqPluginManagerInternal();
  this->Internal->SMPluginMananger = 
    vtkSMObject::GetApplication()->GetPluginManager();
  this->Internal->SMPluginManangerConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->SMPluginManangerConnect->Connect(
      this->Internal->SMPluginMananger, vtkSMPluginManager::LoadPluginInvoked,
      this, 
      SLOT(onSMLoadPluginInvoked(vtkObject*, unsigned long, void*, void*)));
  
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), 
    SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerConnected(pqServer*)));
  
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(), 
    SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(onServerDisconnected(pqServer*)));
  
//  this->addPluginFromSettings();
}

//-----------------------------------------------------------------------------
pqPluginManager::~pqPluginManager()
{
  this->savePluginSettings();
  this->Internal->SMPluginManangerConnect->Disconnect();
  // this->Internal->SMPluginManangerConnect->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QObjectList pqPluginManager::interfaces() const
{
  return this->Internal->Interfaces + this->Internal->ExtraInterfaces;
}

//-----------------------------------------------------------------------------

pqPluginManager::LoadStatus pqPluginManager::loadServerExtension(
  pqServer* server, const QString& lib, 
  vtkPVPluginInformation* pluginInfo, bool remote)
{
  pqPluginManager::LoadStatus success = NOTLOADED;
  vtkPVPluginInformation* smPluginInfo = NULL;
  if(server)
    {
    smPluginInfo = this->Internal->SMPluginMananger->LoadPlugin(
      lib.toAscii().constData(), 
      server->GetConnectionID(),
      this->getServerURIKey(remote ? server : NULL).toAscii().constData(), remote);
    }
  else
    {
    smPluginInfo = this->Internal->SMPluginMananger->LoadLocalPlugin(lib.toAscii().constData());
    }
    
  if(smPluginInfo)
    {
    pluginInfo->DeepCopy(smPluginInfo);
    }

  if(pluginInfo->GetLoaded())
    {
    success = LOADED;
    }

  return success;
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadClientExtension(
  const QString& lib, vtkPVPluginInformation* pluginInfo)
{
  pqPluginManager::LoadStatus success = NOTLOADED;

  QFileInfo fi(lib);
  QString error;
  if(fi.suffix() == "bqrc")
    {
    if(QResource::registerResource(lib))
      {
      success = LOADED;
      pluginInfo->SetLoaded(1);
      this->addExtension(NULL, pluginInfo);
      emit this->guiExtensionLoaded();
      }
    else
      {
      error = "Unable to register resource on client.";
      }
    }
  else if(fi.suffix() == "xml")
    {
    QFile f(lib);
    if(f.open(QIODevice::ReadOnly))
      {
      QByteArray dat = f.readAll();
  
      vtkSMProxyManager::GetProxyManager()->LoadConfigurationXML(
        dat.data());
      //        pluginInfo->SetFileName(lib.toAscii().constData());
      pluginInfo->SetLoaded(1);
      this->addExtension(NULL, pluginInfo);
      success = LOADED;
      emit this->serverManagerExtensionLoaded();
      }
    else
      {
      error = "Unable to open client plugin, " + lib;
      }
    }
  else
    {
    QPluginLoader qplugin(lib);
    if(qplugin.load())
      {
      QObject* pqpluginObject = qplugin.instance();
      pqPlugin* pqplugin = qobject_cast<pqPlugin*>(pqpluginObject);
      if(pqplugin)
        {
        pqpluginObject->setParent(this);  // take ownership to clean up later
        success = LOADED;
//        pluginInfo->SetFileName(lib.toAscii().constData());
        pluginInfo->SetLoaded(1);
        this->addExtension(NULL, pluginInfo);
        emit this->guiExtensionLoaded();
        QObjectList ifaces = pqplugin->interfaces();
        foreach(QObject* iface, ifaces)
          {
          this->Internal->Interfaces.append(iface);
          this->handleAutoStartPlugins(iface, true);
          emit this->guiInterfaceLoaded(iface);
          }
        }
      else
        {
        error = lib + ", is not a ParaView Client Plugin.";
        qplugin.unload();
        }
      }
    else
      {
      error = qplugin.errorString();
      }
    }

  // We still want the plugin info even the plugin can
  if(!pluginInfo->GetLoaded() && !error.isEmpty())
    {
    QString loadError(error);
    if(pluginInfo->GetError())
      {
      loadError.append("\n").append(pluginInfo->GetError());
      }
    pluginInfo->SetError(loadError.toAscii().constData());
    this->addExtension(NULL, pluginInfo);
    emit this->pluginInfoUpdated();
    }

  return success;
}

//-----------------------------------------------------------------------------
static QStringList getLibraries(const QString& path, pqServer* server)
{
  QStringList libs;
  pqFileDialogModel model(server);
  model.setCurrentPath(path);
  int numfiles = model.rowCount(QModelIndex());
  for(int i=0; i<numfiles; i++)
    {
    QModelIndex idx = model.index(i, 0, QModelIndex());
    QString file = model.getFilePaths(idx)[0];
    QFileInfo fileinfo(file);
    // if file names start with known lib suffixes
    if(server)
      {
      if(fileinfo.completeSuffix().indexOf(QRegExp("(so|dll|sl|dylib)$")) == 0)
        {
        libs.append(file);
        }
      }
    else
      {
      if(fileinfo.completeSuffix().indexOf(QRegExp("(so|dll|sl|dylib|xml|bqrc)$")) == 0)
        {
        libs.append(file);
        }
      }
    }
  return libs;
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadExtensions(pqServer* server)
{ 
  QStringList plugin_paths = this->pluginPaths(server);
  foreach(QString path, plugin_paths)
    {
    this->loadExtensions(path, server);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadExtensions(const QString& path, pqServer* server)
{
  QStringList libs = ::getLibraries(path, server);
  foreach(QString lib, libs)
    {
    QString dummy;
    if(!this->getExistingExtensionByFileName(server, lib))
      {
      this->loadExtension(server, lib, &dummy);
      
      // if now the plugin is loaded, we set the auto-load to true
      vtkPVPluginInformation* pluginInfo = 
        this->getExistingExtensionByFileName(server, lib);
      if(pluginInfo && pluginInfo->GetLoaded() && !pluginInfo->GetAutoLoad())
        {
        pluginInfo->SetAutoLoad(1);
        this->Internal->NeedUpdatePluginInfo = true;
        }
      }
    }
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadExtension(
  pqServer* server, const QString& lib, QString* errorReturn, bool remote)
{
  LoadStatus success1 = NOTLOADED;
  LoadStatus success2 = NOTLOADED;

  pqServer* realServer = server && server->isRemote() ? server : NULL;
  
  // check if it is already loaded
  vtkPVPluginInformation* existingPlugin = 
    this->getExistingExtensionByFileName(remote ? realServer : NULL, lib);
  if(existingPlugin && existingPlugin->GetLoaded())
    {
    return ALREADYLOADED;
    }

  // always look for SM/VTK stuff in the plugin
  VTK_CREATE(vtkPVPluginInformation, pluginInfo);
  
  success1 = this->loadServerExtension(
    realServer, lib, pluginInfo, remote);
     
  if(!realServer || !remote)
    {
    // check if this plugin has gui stuff in it
    success2 = loadClientExtension(lib, pluginInfo);
    }

  // return an error if it failed to load
  if(success1 == NOTLOADED && success2 == NOTLOADED)
    {
    if(!errorReturn)
      {
      QMessageBox::information(NULL, "Extension Load Failed", pluginInfo->GetError());
      }
    else
      {
      *errorReturn = pluginInfo->GetError();
      }
    return NOTLOADED;
    }
  else
    {
    return LOADED;
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::removePlugin(
  pqServer* server, const QString& lib, bool remote)
{
  vtkPVPluginInformation* existingPlugin = 
  this->getExistingExtensionByFileName(remote ? server : NULL, lib);
  if(existingPlugin)
    {
    this->Internal->Extensions.remove(
      QString(existingPlugin->GetServerURI()), existingPlugin);
    existingPlugin->Delete();
    }
}

//-----------------------------------------------------------------------------
QList< vtkPVPluginInformation* > pqPluginManager::loadedExtensions(
  pqServer* server) 
{
  return this->loadedExtensions(this->getServerURIKey(server));
}

//-----------------------------------------------------------------------------
QList< vtkPVPluginInformation* > pqPluginManager::loadedExtensions(
  QString extensionKey) 
{
  return this->Internal->Extensions.values(extensionKey);
}

//-----------------------------------------------------------------------------
void pqPluginManager::addInterface(QObject* iface)
{
  if(!this->Internal->ExtraInterfaces.contains(iface))
    {
    this->Internal->ExtraInterfaces.append(iface);
    this->handleAutoStartPlugins(iface, true);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::removeInterface(QObject* iface)
{
  int idx = this->Internal->ExtraInterfaces.indexOf(iface);
  if(idx != -1)
    {
    this->Internal->ExtraInterfaces.removeAt(idx);
    this->handleAutoStartPlugins(iface, false);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerConnected(pqServer* server)
{ 
  this->Internal->NeedUpdatePluginInfo = false;
  this->Internal->IsCurrentServerRemote = 
    server && server->isRemote() ? true : false;
  if(this->Internal->Extensions.size()==0)
    {
    this->addPluginFromSettings();
    }  
  this->loadAutoLoadPlugins(server);
  this->loadExtensions(server);
  if(this->Internal->NeedUpdatePluginInfo)
    {
    emit this->pluginInfoUpdated();
    this->Internal->NeedUpdatePluginInfo = false;
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerDisconnected(pqServer* server)
{
  // remove referenced plugins
  // this->Internal->Extensions.remove(server);
  // 
  if(server && this->Internal->IsCurrentServerRemote)
    {
    foreach(vtkPVPluginInformation* plInfo, 
      this->Internal->Extensions.values(this->getServerURIKey(server)))
      {
      plInfo->SetLoaded(0);
      this->Internal->SMPluginMananger->UpdatePluginLoadInfo(
        plInfo->GetFileName(),
        this->getServerURIKey(server).toAscii().constData(), 0);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::handleAutoStartPlugins(QObject* iface, bool startup)
{
  pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
  if (asi)
    {
    if (startup)
      {
      asi->startup();
      }
    else
      {
      asi->shutdown();
      }
    }
}

//-----------------------------------------------------------------------------
QStringList pqPluginManager::pluginPaths(pqServer* server)
{
  if (vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST"))
    {
    cout << 
      "Ignoring plugin paths since the application is being run on the dashboard"
      << endl;
    return QStringList();
    }

  QString pv_plugin_path;

  if(!server || !this->Internal->IsCurrentServerRemote)
    {
    pv_plugin_path = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
    if(!pv_plugin_path.isEmpty())
      {
      pv_plugin_path += ";";
      }
#if defined (Q_OS_MAC)
    //Look in the Application Package "ParaView.app/Contents/plugins
    pv_plugin_path += QCoreApplication::applicationDirPath() + QDir::separator()
      + "../Plugins;";
    //Look for a folder called "plugins" at the same level as ParaView.app
    pv_plugin_path += QCoreApplication::applicationDirPath() + QDir::separator()
      + "../../../Plugins;";
#else
    pv_plugin_path += QCoreApplication::applicationDirPath() + QDir::separator()
      + "plugins";
#endif
    }
  else
    {
    pv_plugin_path = this->Internal->SMPluginMananger->GetPluginPath(
      server->GetConnectionID(),
      this->getServerURIKey(server).toAscii().constData());
    }

    // add $APPDATA/<organization>/<appname>/Plugins  or
    // $HOME/.config/<organization>/<appname>/Plugins

  QString settingsRoot;
  if(server && this->Internal->IsCurrentServerRemote)
    {
    settingsRoot = vtkSMObject::GetApplication()->GetSettingsRoot(server->GetConnectionID());
    }
  else
    {
#if defined(Q_OS_WIN)
    settingsRoot = QString::fromLocal8Bit(getenv("APPDATA"));
#else
    settingsRoot = QString::fromLocal8Bit(getenv("HOME"));
#endif
    }

#if !defined(Q_OS_WIN)
  if(!settingsRoot.isEmpty())
    {
    settingsRoot += "/.config";
    }
#endif

  if(!settingsRoot.isEmpty())
    {
    QString homePluginPath = QString("%1/%2/%3/Plugins");
    homePluginPath = homePluginPath.arg(settingsRoot);
    homePluginPath = homePluginPath.arg(QCoreApplication::organizationName());
    homePluginPath = homePluginPath.arg(QCoreApplication::applicationName());
    if(!pv_plugin_path.isEmpty())
      {
      pv_plugin_path += ";";
      }
    pv_plugin_path += homePluginPath;
    }

  // trim any whitespace before or after the path delimiters
  // note, shouldn't be a problem with drive letters on Windows "c:\"
  pv_plugin_path = pv_plugin_path.trimmed();
  pv_plugin_path = pv_plugin_path.replace(QRegExp("(\\;|\\:)\\s+"), ";");
  pv_plugin_path = pv_plugin_path.replace(QRegExp("\\s+(\\;|\\:)"), ";");

  // pre-parse the string replacing ':' with ';', watching out for windows drive letters
  // assumes ';' is not used as part of a directory name
  for(int index=0; index < pv_plugin_path.size(); index++)
    {
    QChar c = pv_plugin_path.at(index);
    if(c == ':')
      {
      bool prevIsChar = index > 0 && pv_plugin_path.at(index-1).isLetter();
      bool prevPrevIsDelim = index == 1 || (index > 1 && pv_plugin_path.at(index-2) == ';');
      if(!(prevIsChar && prevPrevIsDelim))
        {
        pv_plugin_path.replace(index, 1, ';');
        }
      }
    }

  QStringList plugin_paths = pv_plugin_path.split(';', QString::SkipEmptyParts);
  return plugin_paths;
}

//-----------------------------------------------------------------------------
void pqPluginManager::addExtension(pqServer* server, 
  vtkPVPluginInformation* pluginInfo)
{
  if(!pluginInfo)
    {
    return;
    }
  this->addExtension(this->getServerURIKey(server), pluginInfo);
 }
 
//-----------------------------------------------------------------------------
void pqPluginManager::addExtension(QString extensionKey, 
  vtkPVPluginInformation* pluginInfo)
{
  vtkPVPluginInformation* plInfo = 
    this->getExistingExtensionByFileName(extensionKey, pluginInfo->GetFileName());
  if(!plInfo)
    {
    vtkPVPluginInformation* localInfo = vtkPVPluginInformation::New();
    localInfo->DeepCopy(pluginInfo);
    this->Internal->Extensions.insert(extensionKey, localInfo);
    }
  else
    {
    int autoLoad = plInfo->GetAutoLoad();
    plInfo->DeepCopy(pluginInfo);
    plInfo->SetAutoLoad(autoLoad);
    }
}

//-----------------------------------------------------------------------------
vtkPVPluginInformation* pqPluginManager::getExistingExtensionByFileName(
 pqServer* server, const QString& lib)
{
  return this->getExistingExtensionByFileName(this->getServerURIKey(server), lib);
}

//-----------------------------------------------------------------------------
vtkPVPluginInformation* pqPluginManager::getExistingExtensionByFileName(
  QString extensionKey, const QString& lib)
{
  foreach(vtkPVPluginInformation* plInfo, this->loadedExtensions(extensionKey))
    {
    if(QString(plInfo->GetFileName()) == lib)
      {
      return plInfo;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVPluginInformation* pqPluginManager::getExistingExtensionByPluginName(
  pqServer* server, const QString& name)
{
  return this->getExistingExtensionByFileName(this->getServerURIKey(server), name);
}

//-----------------------------------------------------------------------------
vtkPVPluginInformation* pqPluginManager::getExistingExtensionByPluginName(
  QString extensionKey, const QString& name)
{
  foreach(vtkPVPluginInformation* plInfo, this->loadedExtensions(extensionKey))
    {
    if(QString(plInfo->GetPluginName()) == name)
      {
      return plInfo;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
QString pqPluginManager::getServerURIKey(pqServer* server) 
{
  return  server && this->Internal->IsCurrentServerRemote ? 
          server->getResource().schemeHostsPorts().toURI() : 
          QString("builtin:");
}

//-----------------------------------------------------------------------------
void pqPluginManager::updatePluginAutoLoadState(
  vtkPVPluginInformation* plInfo, int autoLoad)
{
  if(vtkPVPluginInformation* existingInfo =
      this->getExistingExtensionByFileName(QString(plInfo->GetServerURI()), 
        QString(plInfo->GetFileName())))
    {
    existingInfo->SetAutoLoad(autoLoad);  
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onSMLoadPluginInvoked(
  vtkObject*, unsigned long ulEvent, void*, void* call_data)
{
  vtkPVPluginInformation* plInfo = 
    static_cast<vtkPVPluginInformation*>(call_data);
  if(!plInfo || ulEvent != vtkSMPluginManager::LoadPluginInvoked)
    {
    return;
    }

  this->addExtension(plInfo->GetServerURI(), plInfo);  
  
  if(plInfo->GetLoaded())
    {
    emit this->serverManagerExtensionLoaded();
    }
  else
    {
    emit this->pluginInfoUpdated();
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::addPluginFromSettings()
{
  // get remembered plugins
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList pluginlist = settings->value("/AutoLoadPlugins").toStringList();
  foreach(QString pluginSettingKey, pluginlist)
    {
    this->processPluginSettings(pluginSettingKey);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::savePluginSettings(bool clearFirst)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList pluginlist = settings->value("/AutoLoadPlugins").toStringList();
  if(clearFirst)
    {
    pluginlist.clear();
    }
  
  for(int i=0; i<this->Internal->Extensions.uniqueKeys().count(); i++)
    {
    QString serverURI = this->Internal->Extensions.uniqueKeys().value(i);
    foreach(vtkPVPluginInformation* plInfo, this->loadedExtensions(serverURI))
      {
        QString settingKey = this->getPluginSettingsKey(plInfo);
        if(!pluginlist.contains(settingKey))
          {
          pluginlist.push_back(settingKey);
          }
      }
    }
  settings->setValue("/AutoLoadPlugins", pluginlist);
}

//-----------------------------------------------------------------------------
QString pqPluginManager::getPluginSettingsKey(vtkPVPluginInformation* plInfo)
{
  QString plSettingKey;
  if(plInfo)
    {
    plSettingKey = plInfo->GetServerURI() ? plInfo->GetServerURI() : "builtin:";
    plSettingKey.append("###").append(plInfo->GetFileName()).append("###").append(
      QString::number(plInfo->GetAutoLoad())).append("###").append(
      plInfo->GetPluginName()).append("###").append(plInfo->GetPluginVersion());
    }
  
  return plSettingKey;
}

//-----------------------------------------------------------------------------
void pqPluginManager::processPluginSettings(QString& plSettingKey)
{
  QRegExp rx("(.+)###(.+)###(\\d)###(.+)###(.+)$");
  if(rx.indexIn(plSettingKey)==0)
    {
    QString serverURI = rx.cap(1);
    QString fileName = rx.cap(2);
    int autoLoad = rx.cap(3).toInt();
    QString pluginName = rx.cap(4);
    QString pluginVersion = rx.cap(5);
    if(!this->getExistingExtensionByFileName(serverURI, fileName))
      {
      VTK_CREATE(vtkPVPluginInformation, pluginInfo);
      pluginInfo->SetServerURI(serverURI.toAscii().constData());
      pluginInfo->SetFileName(fileName.toAscii().constData());
      pluginInfo->SetAutoLoad(autoLoad>0 ? 1 : 0);
      pluginInfo->SetPluginName(pluginName.toAscii().constData());
      pluginInfo->SetPluginVersion(pluginVersion.toAscii().constData());
      this->addExtension(pluginInfo->GetServerURI(), pluginInfo);
      }
    } 
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadAutoLoadPlugins(pqServer* server)
{
  foreach(vtkPVPluginInformation* plInfo, this->loadedExtensions(server))
    {
    if(plInfo->GetAutoLoad() && !plInfo->GetLoaded())
      {
      QString dummy;
      this->loadExtension(server, plInfo->GetFileName(), &dummy);
      }
    }
}
//----------------------------------------------------------------------------
bool pqPluginManager::areRequiredPluginsFunctional(
  vtkPVPluginInformation* plInfo, bool remote)
{
  if(!plInfo->GetRequiredPlugins())
    {
    return true;
    }
    
  QString strDepends = plInfo->GetRequiredPlugins();
  if(strDepends.isEmpty())
    {
    return true;
    }
  
  QStringList list = strDepends.split(";");
  foreach(QString pluginName, list)
    {
    if(pluginName.isEmpty())
      {
      continue;
      }
    vtkPVPluginInformation* pluginInfo = 
      this->getExistingExtensionByPluginName(NULL, pluginName);
    if(!pluginInfo && this->Internal->IsCurrentServerRemote)
      {
      pluginInfo = this->getExistingExtensionByPluginName(
        pqApplicationCore::instance()->getActiveServer(), pluginName);
      }
    if(!this->isPluginFuntional(pluginInfo, remote))
      {
      return false;
      }
    }
  return true;  
}

//----------------------------------------------------------------------------
bool pqPluginManager::isPluginFuntional(
  vtkPVPluginInformation* plInfo, bool remote)
{
  if(!plInfo || !plInfo->GetLoaded())
    {
    return false;
    }
    
  if(this->Internal->IsCurrentServerRemote)
    {
    if(remote && plInfo->GetRequiredOnClient())
      {
      vtkPVPluginInformation* clientPlugin = 
        this->getExistingExtensionByFileName(
          NULL, QString(plInfo->GetFileName()));
      if(!clientPlugin || !clientPlugin->GetLoaded())
        {
        plInfo->SetError("warning: it is also required on client! \n Note for developers: If this plugin is only required on server, add REQUIRED_ON_SERVER as an argument when calling ADD_PARAVIEW_PLUGIN in CMakelist.txt");
        return false;
        }
      }
    if(!remote && plInfo->GetRequiredOnServer())
      {
      vtkPVPluginInformation* serverPlugin = 
        this->getExistingExtensionByFileName(
          pqApplicationCore::instance()->getActiveServer(), 
          QString(plInfo->GetFileName()));
      if(!serverPlugin || !serverPlugin->GetLoaded())
        {
        plInfo->SetError("warning: it is also required on server! \n Note for developers: If this plugin is only required on client, add REQUIRED_ON_CLIENT as an argument when calling ADD_PARAVIEW_PLUGIN in CMakelist.txt");
        return false;
        }
      }
    }
    
  if(!this->areRequiredPluginsFunctional(plInfo, remote))
    {
    plInfo->SetError("Missing required plugins!");
    return false;
    }
  return true;    
}
