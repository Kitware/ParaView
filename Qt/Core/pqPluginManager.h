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
#include <QMultiMap>
#include <QStringList>
#include "pqCoreExport.h"

class pqServer;

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
  LoadStatus loadExtension(pqServer* server, const QString& lib, QString* errorMsg=0);
  
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
  QStringList loadedExtensions(pqServer*);

  /// add an extra interface.
  /// these interfaces are appended to the ones loaded from plugins
  void addInterface(QObject* iface);
  
  /// remove an extra interface
  void removeInterface(QObject* iface);

  /// Return all the paths that plugins will be searched for.
  QStringList pluginPaths(pqServer*);

signals:
  /// signal for when an interface is loaded
  void guiInterfaceLoaded(QObject* iface);
  
  /// signal for when a gui extension is loaded.  This can come from loading a
  /// gui plugin or a qrc file
  void guiExtensionLoaded();

  /// notification that new xml was added to the server manager, which could
  /// come from a plugin or an xml file
  void serverManagerExtensionLoaded();

protected:
  LoadStatus loadClientExtension(const QString& lib, QString& error);
  LoadStatus loadServerExtension(pqServer* server, const QString& lib, QString& error);

  // add to the list if it isn't already there
  void addExtension(pqServer* server, const QString& lib);

  /// Handles pqAutoStartInterface plugins.
  void handleAutoStartPlugins(QObject* iface, bool startup);

protected slots:
  void onServerConnected(pqServer*);
  void onServerDisconnected(pqServer*);
  
private:
  QObjectList Interfaces;
  QMultiMap<pqServer*, QString> Extensions;
  QObjectList ExtraInterfaces;

  /// List of plugin names for which the server manager XML has been loaded to
  /// avoid duplicate loading of the XMLS.
  QStringList LoadedServerManagerXMLs;

  /// Temporary function to return the plugin name from the library name.
  QString getPluginName(const QString& library);
};

#endif

