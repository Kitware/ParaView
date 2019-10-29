/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkPVPluginsInformation;
class vtkSMSession;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMPluginManager : public vtkSMObject
{
public:
  static vtkSMPluginManager* New();
  vtkTypeMacro(vtkSMPluginManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Register/Unregister a session. Every vtkSMSession must be registered with
   * the vtkSMPluginManager. This is done automatically by vtkSMSession during
   * the initialization stage. Note that the vtkSMSession is not reference
   * counted.
   */
  void RegisterSession(vtkSMSession*);
  void UnRegisterSession(vtkSMSession*);
  //@}

  //@{
  /**
   * vtkPVPluginsInformation provides information about plugins
   * loaded/available. LocalInformation corresponds to plugins loaded on the
   * local process. For remote sessions i.e. those that connect to a remote
   * server process, one can use GetRemoteInformation() to access information
   * about plugins on the remote process.
   */
  vtkGetObjectMacro(LocalInformation, vtkPVPluginsInformation);
  vtkPVPluginsInformation* GetRemoteInformation(vtkSMSession*);
  //@}

  //@{
  /**
   * Returns the plugin search paths used either locally or remotely. For
   * non-remote sessions, GetRemotePluginSearchPaths() returns the same value as
   * GetLocalPluginSearchPaths().
   */
  const char* GetLocalPluginSearchPaths();
  const char* GetRemotePluginSearchPaths(vtkSMSession*);
  //@}

  //@{
  /**
   * Loads the plugin either locally or remotely.
   */
  bool LoadRemotePlugin(const char* filename, vtkSMSession*);
  bool LoadLocalPlugin(const char* filename);
  //@}

  /**
   * Plugin configuration XML is a simple XML that makes ParaView aware of the
   * plugins available and may result in loading of those plugins that are
   * marked for auto-loading. In ParaView application there are two uses for this:
   * \li .plugins - used to notify ParaView of the distributed plugins
   * \li session - used to save/restore the plugins loaded by the users.

   * This method loads the plugin configuration xml either on the local process or the
   * remote server process(es). \c session is only used when
   * remote==true and session itself is a remote session.
   */
  void LoadPluginConfigurationXMLFromString(
    const char* xmlcontents, vtkSMSession* session, bool remote);

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

  vtkPVPluginsInformation* LocalInformation;

private:
  vtkSMPluginManager(const vtkSMPluginManager&) = delete;
  void operator=(const vtkSMPluginManager&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
