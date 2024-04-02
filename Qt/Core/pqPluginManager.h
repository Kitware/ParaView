// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPluginManager_h
#define pqPluginManager_h

#include "pqCoreModule.h"
#include <QObject>
#include <QStringList>

class pqServer;
class vtkPVPlugin;
class vtkPVPluginsInformation;
class vtkSMPluginManager;

/**
 * pqPluginManager works with vtkSMPluginManager to keep track for plugins
 * loaded/available. It also ensures that when a new session is created, the
 * default plugin-configuration-xmls are parsed on all processes involved to
 * ensure that auto-load plugins are loaded. It preserves the information about
 * plugins loaded across ParaView sessions in settings so that users can easily
 * load previously loaded plugins.
 *
 * pqPluginManager can work with multiple sessions. It maintains internal
 * data-structures for different sessions.
 */
class PQCORE_EXPORT pqPluginManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginManager(QObject* p = nullptr);
  ~pqPluginManager() override;

  /**
   * Called during application initialization to load plugins from settings.
   * This only loads "local" plugins. pqApplicationCore calls this method
   * explicitly after the essential components of the core have been
   * initialized. This ensures that any plugins  being loaded during startup of
   * application have the environment setup correctly.
   */
  void loadPluginsFromSettings();

  enum LoadStatus
  {
    LOADED,
    NOTLOADED,
    ALREADYLOADED
  };

  /**
   * attempt to load an extension on a server
   * if server is nullptr, extension will be loaded on client side
   * return status on success, if NOTLOADED was returned, the error is reported
   * If errorMsg is non-nullptr, then errors are not reported, but the error
   * message is put in the errorMsg string
   * the plugin arg can be a full path or the name of an existing plugin
   */
  LoadStatus loadExtension(
    pqServer* session, const QString& plugin, QString* errorMsg = nullptr, bool remote = true);

  /**
   * attempt to load all available plugins on a server, or client plugins if
   * nullptr
   */
  void loadExtensions(pqServer*);

  /**
   * return all the plugins loaded on a session. This will either returns the
   * plugins information for local processes or server-process (for remote
   * sessions) based on the state of \c remote.
   */
  vtkPVPluginsInformation* loadedExtensions(pqServer* session, bool remote);

  /**
   * Return all the paths that plugins will be searched for.
   */
  QStringList pluginPaths(pqServer* session, bool remote);

  /**
   * Load a plugin config file and add all plugin from it to the plugin manager of provided server
   */
  void addPluginConfigFile(pqServer* session, const QString& config, bool remote = true);

  /**
   * simply adds the plugin to the ignore list, so when this class tries to
   * serialize the plugin information, it skips the indicated plugin.
   */
  void hidePlugin(const QString& lib, bool remote);
  bool isHidden(const QString& lib, bool remote);

  /**
   * ensures that plugins required on client and server are present on both.
   * Fires requiredPluginsNotLoaded() signal if any mismatch is found.
   * Returns true is all plugin requirements are satisfied, else returns false.
   */
  bool verifyPlugins(pqServer* session);

Q_SIGNALS:
  /**
   * notification when plugin has been loaded.
   */
  void pluginsUpdated();

  /**
   * notification that the plugins on the client and server are mismatched.
   */
  void requiredPluginsNotLoaded(pqServer*);

protected:
  void initialize(vtkSMPluginManager*);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * attempts to load the configuration for plugins for the particular server.
   */
  void loadPluginsFromSettings(pqServer*);

  void onServerConnected(pqServer*);
  void onServerDisconnected(pqServer*);
  void updatePluginLists();

private:
  class pqInternals;
  pqInternals* Internals;

  /**
   * Callback passed on to `vtkPVPluginLoader::SetEULAConfirmationCallback` to
   * confirm EULA for locally loaded plugins.
   */
  static bool confirmEULA(vtkPVPlugin* plugin);
};

#endif
