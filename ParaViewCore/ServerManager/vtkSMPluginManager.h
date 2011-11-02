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
// .NAME vtkSMPluginManager - manages ParaView plugins.
// .SECTION Description
// vtkSMPluginManager is used to load plugins as well as discover information
// about currently loaded and available plugins.
//
// vtkSMPluginManager supports multiple sessions. Every vtkSMSession registers
// itself with the vtkSMPluginManager during initialization.

#ifndef __vtkSMPluginManager_h
#define __vtkSMPluginManager_h

#include "vtkSMObject.h"

class vtkPVPluginsInformation;
class vtkSMSession;

class VTK_EXPORT vtkSMPluginManager : public vtkSMObject
{
public:
  static vtkSMPluginManager* New();
  vtkTypeMacro(vtkSMPluginManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Register/Unregister a session. Every vtkSMSession must be registered with
  // the vtkSMPluginManager. This is done automatically by vtkSMSession during
  // the initialization stage. Note that the vtkSMSession is not reference
  // counted.
  void RegisterSession(vtkSMSession*);
  void UnRegisterSession(vtkSMSession*);

  // Description:
  // vtkPVPluginsInformation provides information about plugins
  // loaded/available. LocalInformation corresponds to plugins loaded on the
  // local process. For remote sessions i.e. those that connect to a remote
  // server process, one can use GetRemoteInformation() to access information
  // about plugins on the remote process.
  vtkGetObjectMacro(LocalInformation, vtkPVPluginsInformation);
  vtkPVPluginsInformation* GetRemoteInformation(vtkSMSession*);

  // Description:
  // Returns the plugin search paths used either locally or remotely. For
  // non-remote sessions, GetRemotePluginSearchPaths() returns the same value as
  // GetLocalPluginSearchPaths().
  const char* GetLocalPluginSearchPaths();
  const char* GetRemotePluginSearchPaths(vtkSMSession*);

  // Description:
  // Loads the plugin either locally or remotely.
  bool LoadRemotePlugin(const char* filename, vtkSMSession*);
  bool LoadLocalPlugin(const char* filename);

  // Description:
  // Plugin configuration XML is a simple XML that makes ParaView aware of the
  // plugins available and may result in loading of those plugins that are
  // marked for auto-loading. In ParaView application there are two uses for this:
  // \li .plugins - used to notify ParaView of the distributed plugins
  // \li session - used to save/restore the plugins loaded by the users.
  //
  // This method loads the plugin configuration xml either on the local process or the
  // remote server process(es). \c session is only used when
  // remote==true and session itself is a remote session.
  void LoadPluginConfigurationXMLFromString(const char* xmlcontents,
    vtkSMSession* session, bool remote);

  enum
    {
    PluginLoadedEvent = 100000
    };
//BTX
protected:
  vtkSMPluginManager();
  ~vtkSMPluginManager();

  vtkPVPluginsInformation* LocalInformation;

private:
  vtkSMPluginManager(const vtkSMPluginManager&); // Not implemented
  void operator=(const vtkSMPluginManager&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
