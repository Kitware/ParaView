/*=========================================================================

   Program: ParaView
   Module:    pqPluginManager.h

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

#ifndef _pqPluginManager_h
#define _pqPluginManager_h

#include <QObject>
#include <QStringList>
#include "vtkSmartPointer.h"
#include "pqCoreExport.h"

class pqServer;
class vtkObject;
class vtkPVPluginInformation;
class pqPluginManagerInternal;

/// extension manager takes care of loading plugins, xml files, and binary qrc files
/// the plugins may also contain xml files and qrc files
class PQCORE_EXPORT pqPluginManager : public QObject
{
  Q_OBJECT
public:
  pqPluginManager(QObject* p = 0);
  ~pqPluginManager();

  enum LoadStatus { LOADED, NOTLOADED, ALREADYLOADED };

  /// attempt to load an extension on a server
  /// if server is NULL, extension will be loaded on client side
  /// return status on success, if NOTLOADED was returned, the error is reported
  /// If errorMsg is non-null, then errors are not reported, but the error
  /// message is put in the errorMsg string
  LoadStatus loadExtension(pqServer* server, const QString& lib, 
    QString* errorMsg=0, bool remote=true);
  
  /// attempt to load all available plugins on a server, 
  /// or client plugins if NULL
  void loadExtensions(pqServer*);

  /// Attempts to load all available plugins in the directory pointed by
  /// \c path. If server is 0, it loads client plugins, else it loads server
  /// plugins
  void loadExtensions(const QString& path, pqServer* server);
  
  /// return all GUI interfaces that have been loaded
  QObjectList interfaces() const;

  template <class T>
    QList<T> findInterfaces() const
      {
      QList<T> list;
      QObjectList objList = this->interfaces();
      foreach (QObject* object, objList)
        {
        if (object && qobject_cast<T>(object))
          {
          list.push_back(qobject_cast<T>(object));
          }
        }
      return list;
      }

  /// return all the plugins loaded on a server, or locally if NULL is passed in
  QList< vtkPVPluginInformation* > loadedExtensions(pqServer*);
  QList< vtkPVPluginInformation* > loadedExtensions(QString extensionKey);
 
  /// add an extra interface.
  /// these interfaces are appended to the ones loaded from plugins
  void addInterface(QObject* iface);
  
  /// remove an extra interface
  void removeInterface(QObject* iface);

  /// Return all the paths that plugins will be searched for.
  QStringList pluginPaths(pqServer*);

  /// Get the pluginInfo that may already exist in this manager by the file name or plugin name.
  vtkPVPluginInformation* getExistingExtensionByFileName(pqServer*, const QString& lib);
  vtkPVPluginInformation* getExistingExtensionByFileName(QString extensionKey, const QString& lib);
  vtkPVPluginInformation* getExistingExtensionByPluginName(pqServer*, const QString& name);
  vtkPVPluginInformation* getExistingExtensionByPluginName(QString extensionKey, const QString& name);
  
  /// Update the plugin AutoLoad state.
  void updatePluginAutoLoadState(vtkPVPluginInformation* plInfo, int autoLoad);

  // load plugin settings
  void addPluginFromSettings();
  // save plugin settings
  void savePluginSettings(bool clearFirst = true);
  // check whether the plugin is ready to run.
  bool isPluginFuntional(vtkPVPluginInformation* plInfo, bool remote);
  // exclude an extension from being saved with settings.
  void removePlugin(pqServer* server, const QString& lib, 
    bool remote=true);
  
signals:
  /// signal for when an interface is loaded
  void guiInterfaceLoaded(QObject* iface);
  
  /// signal for when a gui extension is loaded.  This can come from loading a
  /// gui plugin or a qrc file
  void guiExtensionLoaded();

  /// notification that new xml was added to the server manager, which could
  /// come from a plugin or an xml file
  void serverManagerExtensionLoaded();

  /// notification when plugin information is updated
  void pluginInfoUpdated();

protected:
  LoadStatus loadServerExtension(pqServer* server, const QString& lib, 
    vtkPVPluginInformation* pluginInfo, bool remote);
  LoadStatus loadClientExtension(const QString& lib,
    vtkPVPluginInformation* pluginInfo);

  // add to the list if it isn't already there
  void addExtension(pqServer* server, vtkPVPluginInformation* pluginInfo);
  void addExtension(QString extensionKey, vtkPVPluginInformation* pluginInfo);
  
  /// Handles pqAutoStartInterface plugins.
  void handleAutoStartPlugins(QObject* iface, bool startup);
  
  // check whether the required plugins are functional.
  bool areRequiredPluginsFunctional(vtkPVPluginInformation* plInfo, bool remote);
  
protected slots:
  void onServerConnected(pqServer*);
  void onServerDisconnected(pqServer*);
  void onSMLoadPluginInvoked(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  
private:
  
  pqPluginManagerInternal* Internal;
  // return the extension key for identifying the server, 
  // used for mapping from server to plugins.
  QString getServerURIKey(pqServer* server);
  // return the settings key for the plugin, 
  // used for saving the plugin settings ,
  // "[serverURI]###[filename]###[AutoLoad]"
  QString getPluginSettingsKey(vtkPVPluginInformation*);
  void processPluginSettings(QString& plSettingKey);
  // load the Auto Load plugins on that server
  void loadAutoLoadPlugins(pqServer* server);    
  
  QObjectList Interfaces;
  QObjectList ExtraInterfaces;

};

#endif

