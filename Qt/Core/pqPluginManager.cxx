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

#include <QLibrary>
#include <QPluginLoader>
#include <QFileInfo>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QResource>

#include "vtkIntArray.h"
#include "vtkProcessModule.h"
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVEnvironmentInformationHelper.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPythonModule.h"
#include "vtkStringArray.h"
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkToolkits.h"

#include "vtksys/SystemTools.hxx"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include "pqApplicationCore.h"
#include "pqAutoStartInterface.h"
#include "pqFileDialogModel.h"
#include "pqPlugin.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
pqPluginManager::pqPluginManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* sm =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(sm, SIGNAL(serverAdded(pqServer*)),
                   this, SLOT(onServerConnected(pqServer*)));
  QObject::connect(sm, SIGNAL(serverRemoved(pqServer*)),
                   this, SLOT(onServerDisconnected(pqServer*)));
}

//-----------------------------------------------------------------------------
pqPluginManager::~pqPluginManager()
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
}

//-----------------------------------------------------------------------------
QObjectList pqPluginManager::interfaces() const
{
  return this->Interfaces + this->ExtraInterfaces;
}

//-----------------------------------------------------------------------------
QString pqPluginManager::getPluginName(const QString& library)
{
  QFileInfo info(library);
  QString name = info.baseName(); // remove the path and the extension.
  if (name.startsWith("lib"))
    {
    name = name.mid(3); // remove "lib" from the name.
    }
  return name;
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadServerExtension(pqServer* server, const QString& lib,
                                       QString& error)
{
  pqPluginManager::LoadStatus success = NOTLOADED;

  if(server)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* pxy = pxm->NewProxy("misc", "PluginLoader");
    if(pxy && !lib.isEmpty())
      {
      pxy->SetConnectionID(server->GetConnectionID());
      // data & render servers
      pxy->SetServers(vtkProcessModule::SERVERS);
      vtkSMProperty* prop = pxy->GetProperty("FileName");
      pqSMAdaptor::setElementProperty(prop, lib);
      pxy->UpdateVTKObjects();
      pxy->UpdatePropertyInformation();
      prop = pxy->GetProperty("Loaded");
      success = pqSMAdaptor::getElementProperty(prop).toInt() ? LOADED : NOTLOADED;
      if(success == LOADED)
        {
        prop = pxy->GetProperty("ServerManagerXML");
        QString pluginName = this->getPluginName(lib);
        if (!this->LoadedServerManagerXMLs.contains(pluginName))
          {
          QList<QVariant> xmls = pqSMAdaptor::getMultipleElementProperty(prop);
          foreach(QVariant xml, xmls)
            {
            vtkSMProxyManager::GetProxyManager()->LoadConfigurationXML(
              xml.toString().toAscii().data());
            }
          this->LoadedServerManagerXMLs.push_back(pluginName);
          }

#ifdef VTK_WRAP_PYTHON
        prop = pxy->GetProperty("PythonModuleNames");
        QList<QVariant> names = pqSMAdaptor::getMultipleElementProperty(prop);
        prop = pxy->GetProperty("PythonModuleSources");
        QList<QVariant> sources = pqSMAdaptor::getMultipleElementProperty(prop);
        prop = pxy->GetProperty("PythonPackageFlags");
        QList<QVariant> pflags = pqSMAdaptor::getMultipleElementProperty(prop);
        for (int i = 0; i < names.size(); i++)
          {
          VTK_CREATE(vtkPVPythonModule, module);
          module->SetFullName(names[i].toString().toAscii().data());
          module->SetSource(sources[i].toString().toAscii().data());
          module->SetIsPackage(pflags[i].toInt());
          vtkPVPythonModule::RegisterModule(module);
          }
#endif //VTK_WRAP_PYTHON
        }
      else
        {
        error =
          pqSMAdaptor::getElementProperty(pxy->GetProperty("Error")).toString();
        }
      pxy->UnRegister(NULL);
      }
    }
  else
    {
    VTK_CREATE(vtkPVPluginLoader, loader);
    loader->SetFileName(lib.toAscii().data());
    success = loader->GetLoaded() ? LOADED : NOTLOADED;
    if(success == LOADED)
      {
      int i;
      QString pluginName = this->getPluginName(lib);
       if (!this->LoadedServerManagerXMLs.contains(pluginName))
         {
         vtkStringArray* xmls = loader->GetServerManagerXML();
         for(i=0; i<xmls->GetNumberOfValues(); i++)
           {
           vtkSMProxyManager::GetProxyManager()->LoadConfigurationXML(
             xmls->GetValue(i).c_str());
           }
         this->LoadedServerManagerXMLs.push_back(pluginName);
         }

#ifdef VTK_WRAP_PYTHON
      vtkStringArray *names = loader->GetPythonModuleNames();
      vtkStringArray *sources = loader->GetPythonModuleSources();
      vtkIntArray *pflags = loader->GetPythonPackageFlags();
      for (i = 0; i < names->GetNumberOfValues(); i++)
        {
        VTK_CREATE(vtkPVPythonModule, module);
        module->SetFullName(names->GetValue(i).c_str());
        module->SetSource(sources->GetValue(i).c_str());
        module->SetIsPackage(pflags->GetValue(i));
        vtkPVPythonModule::RegisterModule(module);
        }
#endif //VTK_WRAP_PYTHON
      }
    else
      {
      error = loader->GetError();
      }
    }

  if(success == LOADED)
    {
    this->addExtension(server, lib);
    emit this->serverManagerExtensionLoaded();
    }

  return success;
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadClientExtension(const QString& lib, QString& error)
{
  pqPluginManager::LoadStatus success = NOTLOADED;

  QFileInfo fi(lib);
  if(fi.suffix() == "bqrc")
    {
    if(QResource::registerResource(lib))
      {
      success = LOADED;
      this->addExtension(NULL, lib);
      emit this->guiExtensionLoaded();
      }
    else
      {
      error = "Unable to register resource";
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
      this->addExtension(NULL, lib);
      success = LOADED;
      emit this->serverManagerExtensionLoaded();
      }
    else
      {
      error = "Unable to open " + lib;
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
        this->addExtension(NULL, lib);
        emit this->guiExtensionLoaded();
        QObjectList ifaces = pqplugin->interfaces();
        foreach(QObject* iface, ifaces)
          {
          this->Interfaces.append(iface);
          this->handleAutoStartPlugins(iface, true);
          emit this->guiInterfaceLoaded(iface);
          }
        }
      else
        {
        error = "This is not a ParaView Client Plugin.";
        qplugin.unload();
        }
      }
    else
      {
      error = qplugin.errorString();
      }
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
    this->loadExtension(server, lib, &dummy);
    }
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadExtension(
  pqServer* server, const QString& lib, QString* errorReturn)
{
  LoadStatus success1 = NOTLOADED;
  LoadStatus success2 = NOTLOADED;
  QString error;

  // treat builtin server as null
  if(server && !server->isRemote())
    {
    server = NULL;
    }

  // check if it is already loaded
  if(this->loadedExtensions(server).contains(lib))
    {
    return ALREADYLOADED;
    }

  // always look for SM/VTK stuff in the plugin
  success1 = pqPluginManager::loadServerExtension(server, lib, error);

  if(!server)
    {
    // check if this plugin has gui stuff in it
    success2 = loadClientExtension(lib, error);
    }

  // return an error if it failed to load
  if(success1 == NOTLOADED && success2 == NOTLOADED)
    {
    if(!errorReturn)
      {
      QMessageBox::information(NULL, "Extension Load Failed", error);
      }
    else
      {
      *errorReturn = error;
      }
    }

  // return success if any client or server stuff was found in it
  if(success1 == LOADED || success2 == LOADED)
    {
    return LOADED;
    }

  return NOTLOADED;
}

//-----------------------------------------------------------------------------
QStringList pqPluginManager::loadedExtensions(pqServer* server)
{
  if(server && !server->isRemote())
    {
    server = NULL;
    }
  return this->Extensions.values(server);
}

//-----------------------------------------------------------------------------
void pqPluginManager::addInterface(QObject* iface)
{
  if(!this->ExtraInterfaces.contains(iface))
    {
    this->ExtraInterfaces.append(iface);
    this->handleAutoStartPlugins(iface, true);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::removeInterface(QObject* iface)
{
  int idx = this->ExtraInterfaces.indexOf(iface);
  if(idx != -1)
    {
    this->ExtraInterfaces.removeAt(idx);
    this->handleAutoStartPlugins(iface, false);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerConnected(pqServer* server)
{
  this->loadExtensions(server);
}

//-----------------------------------------------------------------------------
void pqPluginManager::onServerDisconnected(pqServer* server)
{
  // remove referenced plugins
  this->Extensions.remove(server);
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

  if(!server)
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
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* pxy = pxm->NewProxy("misc", "PluginLoader");
    pxy->SetConnectionID(server->GetConnectionID());
    pxy->UpdateVTKObjects();
    pxy->UpdatePropertyInformation();
    pv_plugin_path =
      pqSMAdaptor::getElementProperty(pxy->GetProperty("SearchPaths")).toString();
    pxy->UnRegister(NULL);
    }

    // add $APPDATA/<organization>/<appname>/Plugins  or
    // $HOME/.config/<organization>/<appname>/Plugins

  QString settingsRoot;
  if(server)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* helper = pxm->NewProxy("misc", "EnvironmentInformationHelper");
    helper->SetConnectionID(server->GetConnectionID());
    helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
#if defined(Q_OS_WIN)
    pqSMAdaptor::setElementProperty(helper->GetProperty("Variable"), "APPDATA");
#else
    pqSMAdaptor::setElementProperty(helper->GetProperty("Variable"), "HOME");
#endif
    helper->UpdateVTKObjects();
    vtkPVEnvironmentInformation* info = vtkPVEnvironmentInformation::New();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(helper->GetConnectionID(),
      vtkProcessModule::DATA_SERVER, info, helper->GetID());
    settingsRoot = info->GetVariable();
    info->Delete();
    helper->UnRegister(NULL);
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

void pqPluginManager::addExtension(pqServer* server, const QString& lib)
{
  if(!this->Extensions.values(server).contains(lib))
    {
    this->Extensions.insert(server, lib);
    }
}

