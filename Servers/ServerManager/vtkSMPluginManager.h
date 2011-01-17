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
// .NAME vtkSMPluginManager
// .SECTION Description
// vtkSMPluginManager is used to load plugins as well as discover information
// about currently loaded and available plugins.

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
  // Set the session whose plugin state this manager reflects.
  // Note this is not reference counted.
  void SetSession(vtkSMSession*);

  // Description:
  // Initializes the plugin manager with information about client and server
  // side plugins on the session.
  void Initialize();

  // Description:
  // API to iterate over local and remote plugin information objects.
  vtkGetObjectMacro(LocalInformation, vtkPVPluginsInformation);
  vtkGetObjectMacro(RemoteInformation, vtkPVPluginsInformation);

  // Description:
  // Returns the plugin search paths used either locally or remotely.
  const char* GetPluginSearchPaths(bool remote);

  // Description:
  // Loads the plugin either locally or remotely.
  bool LoadRemotePlugin(const char* filename);
  bool LoadLocalPlugin(const char* filename);

  // Description:
  // Loads the plugin configuration xml.
  void LoadPluginConfigurationXMLFromString(const char* xmlcontents,
    bool remote);

  enum
    {
    PluginLoadedEvent = 100000
    };
//BTX
protected:
  vtkSMPluginManager();
  ~vtkSMPluginManager();

  char* PluginSearchPaths;

  vtkPVPluginsInformation* LocalInformation;
  vtkPVPluginsInformation* RemoteInformation;
private:
  vtkSMPluginManager(const vtkSMPluginManager&); // Not implemented
  void operator=(const vtkSMPluginManager&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
