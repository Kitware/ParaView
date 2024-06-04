// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPluginManager
 * @brief   manages ParaView plugins.
 *
 * vtkSMPluginManager is used to load plugins as well as discover information
 * about currently loaded and available plugins.
 *
 * vtkSMPluginManager supports multiple sessions. Every vtkSMSession registers
 * itself with the vtkSMPluginManager during initialization.
 */

#ifndef vtkSMPluginManager_h
#define vtkSMPluginManager_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVPluginsInformation;
class vtkSMSession;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPluginManager : public vtkSMObject
{
public:
  static vtkSMPluginManager* New();
  vtkTypeMacro(vtkSMPluginManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Register/Unregister a session. Every vtkSMSession must be registered with
   * the vtkSMPluginManager. This is done automatically by vtkSMSession during
   * the initialization stage. Note that the vtkSMSession is not reference
   * counted.
   */
  void RegisterSession(vtkSMSession*);
  void UnRegisterSession(vtkSMSession*);
  ///@}

  ///@{
  /**
   * vtkPVPluginsInformation provides information about plugins
   * loaded/available. LocalInformation corresponds to plugins loaded on the
   * local process. For remote sessions i.e. those that connect to a remote
   * server process, one can use GetRemoteInformation() to access information
   * about plugins on the remote process.
   */
  vtkGetObjectMacro(LocalInformation, vtkPVPluginsInformation);
  vtkPVPluginsInformation* GetRemoteInformation(vtkSMSession*);
  ///@}

  ///@{
  /**
   * Returns the plugin search paths used either locally or remotely. For
   * non-remote sessions, GetRemotePluginSearchPaths() returns the same value as
   * GetLocalPluginSearchPaths().
   */
  const char* GetLocalPluginSearchPaths();
  const char* GetRemotePluginSearchPaths(vtkSMSession*);
  ///@}

  ///@{
  /**
   * Loads the plugin either locally or remotely.
   * plugin can either be a full path or a plugin name.
   */
  bool LoadRemotePlugin(const char* plugin, vtkSMSession*);
  bool LoadLocalPlugin(const char* plugin);
  ///@}

  ///@{
  /**
   * Plugin configuration XML is a simple XML that makes ParaView aware of the
   * plugins available and may result in loading of those plugins that are
   * marked for auto-loading. In ParaView application there are two uses for this:
   * \li .plugins - used to notify ParaView of the distributed plugins
   * \li session - used to save/restore the plugins loaded by the users.

   * This method loads the plugin configuration xml content or file
   * either on the local process or the remote server process(es).
   * \c session is only used when remote==true and session itself is a remote session.
   */
  void LoadPluginConfigurationXMLFromString(
    const char* xmlcontents, vtkSMSession* session, bool remote);
  void LoadPluginConfigurationXML(
    const char* configurationFile, vtkSMSession* session, bool remote);
  ///@}

  /**
   * Method to load remote plugins in order to meet plugin requirement across processes.
   * This also updates the "StatusMessage" for all the plugins. If StatusMessage
   * is empty for a loaded plugin, it implies that everything is fine. If some
   * requirement is not met, the StatusMessage includes the error message.
   * Set onlyCheck to true to only check and set status without loading plugins.
   */
  bool FulfillPluginRequirements(vtkSMSession* session, bool onlyCheck = false);

  enum
  {
    PluginLoadedEvent = 100000,
    LocalPluginLoadedEvent,
    RemotePluginLoadedEvent
  };

protected:
  vtkSMPluginManager();
  ~vtkSMPluginManager() override;

  bool InLoadPlugin;
  void OnPluginRegistered();
  void OnPluginAvailable();
  void UpdateLocalPluginInformation();

  /**
   * Protected method used by FulfillPluginRequirements to check and load
   * client/server plugin requirements
   */
  bool FulfillPluginClientServerRequirements(vtkSMSession* session,
    const std::map<std::string, unsigned int>& inputMap, vtkPVPluginsInformation* inputPluginInfo,
    const std::map<std::string, unsigned int>& outputMap, vtkPVPluginsInformation* outputPluginInfo,
    bool inputClient, bool onlyCheck);

  vtkPVPluginsInformation* LocalInformation;

private:
  vtkSMPluginManager(const vtkSMPluginManager&) = delete;
  void operator=(const vtkSMPluginManager&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
