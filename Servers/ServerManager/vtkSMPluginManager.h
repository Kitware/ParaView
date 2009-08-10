/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPluginManager - Responsible for managing server manager plugins.
// .SECTION Description
// vtkSMPluginManager is a class that loads and manages server manager plugins.
// .SECTION See Also

#ifndef __vtkSMPluginManager_h
#define __vtkSMPluginManager_h

#include "vtkSMObject.h"

class vtkSMPluginProxy;
class vtkPVPluginInformation;
class vtkPVPluginLoader;
class vtkStringArray;
class vtkIntArray;

class VTK_EXPORT vtkSMPluginManager : public vtkSMObject
{
public:
  static vtkSMPluginManager* New();
  vtkTypeRevisionMacro(vtkSMPluginManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Method to load a plugin in server manager.
  // filename, the full path to the plugin
  // connectionId, the connection ID of the connection to the server
  // serverURI, the server URI in the format of "scheme://host:port"
  //            for buildin server, it will be "builtin:"
  // loadRemote, optional flag to specify whether this is to load plugin 
  //             on client side or on server side. 
  // Return a vtkPVPluginInformation object, if sucess; NULL, otherwise
  vtkPVPluginInformation* LoadPlugin(
    const char* filename, vtkIdType connectionId, const char* serverURI,
    bool loadRemote = true );
  
  // Description:
  // Get the plugin path that specified through some environmental varaibles.
  const char* GetPluginPath(vtkIdType connectionId, const char* serverURI);    
  
  // Description:
  // Process the plugin information if it is loaded.
  void ProcessPluginInfo(vtkSMPluginProxy* pluginProxy);  

  // Description:
  // Update the "Loaded" info of the plugin
  void UpdatePluginLoadInfo(const char* filename, 
    const char* serverURI, int loaded);
    
//BTX
  // Description:
  // Events invoked when LoadPlugin() is called.
  enum
    {
    LoadPluginInvoked = 10000,
    };
//ETX
  
//BTX
protected:
  vtkSMPluginManager();
  ~vtkSMPluginManager();

  friend class vtkSMPluginManagerObserver;

  // Description:
  // Check if the plugin is already loaded, given the file name 
  virtual vtkPVPluginInformation* FindPluginByFileName(
    const char* filename, const char* serverURI);
  
private:
  vtkSMPluginManager(const vtkSMPluginManager&); // Not implemented
  void operator=(const vtkSMPluginManager&); // Not implemented
  
  class vtkSMPluginManagerInternals;
  vtkSMPluginManagerInternals* Internal;

  void ProcessPluginSMXML(vtkStringArray* smXMLArray);
  void ProcessPluginPythonInfo(vtkStringArray* pyNames,
    vtkStringArray* pySources, vtkIntArray* pyFlags);
//ETX
};

#endif
