/*=========================================================================

   Program: ParaView
   Module:    pqPluginManager.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// plugin loader takes care of loading plugins
/// containing GUI extensions and server manager extensions
class PQCORE_EXPORT pqPluginManager : public QObject
{
  Q_OBJECT
public:
  pqPluginManager(QObject* p = 0);
  ~pqPluginManager();

  enum LoadStatus { LOADED, NOTLOADED, ALREADYLOADED };

  /// attempt to load a plugin
  /// return status on success, if NOTLOADED was returned, the error is reported
  /// If errorMsg is non-null, then errors are not reported, but the error
  /// message is put in the errorMsg string
  LoadStatus loadPlugin(pqServer* server, const QString& lib, QString* errorMsg=0);
  
  /// attempt to load all available plugins on a server, 
  /// or client plugins if NULL
  void loadPlugins(pqServer*);

  /// Attempts to load all available plugins in the directory pointed by
  /// \c path. If server is 0, it loads client plugins, else it loads server
  /// plugins
  void loadPlugins(const QString& path, pqServer* server);
  
  /// return all GUI interfaces that have been loaded
  QObjectList interfaces();

  /// return all the plugins loaded on a server, or locally if NULL is passed in
  QStringList loadedPlugins(pqServer*);

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
  
  /// signal for when a gui plugin is loaded
  void guiPluginLoaded();

  /// notification that new extensions were added to the server manager
  void serverManagerExtensionLoaded();

protected:

  LoadStatus loadClientPlugin(const QString& lib, QString& error);
  LoadStatus loadServerPlugin(pqServer* server, const QString& lib, QString& error);

protected slots:
  void onServerConnected(pqServer*);
  void onServerDisconnected(pqServer*);

private:

  QObjectList Interfaces;
  QMultiMap<pqServer*, QString> Plugins;
  QObjectList ExtraInterfaces;
};

#endif

