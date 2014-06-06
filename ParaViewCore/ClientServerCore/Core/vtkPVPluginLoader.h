/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPluginLoader - Used to load ParaView plugins.
// .SECTION Description
// vtkPVPluginLoader can be used to load plugins for ParaView. vtkPVPluginLoader
// loads the plugin on the local process. For verbose details during the process
// of loading the plugin, try setting the environment variable PV_PLUGIN_DEBUG.
// This class only needed when loading plugins from shared libraries
// dynamically. For statically importing plugins, one directly uses
// PV_PLUGIN_IMPORT() macro defined in vtkPVPlugin.h.

#ifndef __vtkPVPluginLoader_h
#define __vtkPVPluginLoader_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkObject.h"

class vtkIntArray;
class vtkPVPlugin;
class vtkStringArray;
class vtkPVPlugin;

typedef bool (*vtkPluginLoadFunction)(const char*);

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVPluginLoader : public vtkObject
{
public:
  static vtkPVPluginLoader* New();
  vtkTypeMacro(vtkPVPluginLoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tries to the load the plugin given the path to the plugin file.
  bool LoadPlugin(const char* filename)
    { return this->LoadPluginInternal(filename, false); }
  bool LoadPluginSilently(const char* filename)
    { return this->LoadPluginInternal(filename, true); }

  // Description:
  // Simply forwards the call to
  // vtkPVPluginLoader::LoadPluginConfigurationXMLFromString to load
  // configuration xml.
  void LoadPluginConfigurationXMLFromString(const char* xmlcontents);

  // Description:
  // Loads all plugins under the directories mentioned in the SearchPaths.
  void LoadPluginsFromPluginSearchPath();

  // Description:
  // Loads all plugin libraries at a path.
  void LoadPluginsFromPath(const char* path);

  // Description:
  // Returns the full filename for the plugin attempted to load most recently
  // using LoadPlugin().
  vtkGetStringMacro(FileName);

  // Description:
  // Get the plugin name. This returns a valid name only after the plugin has
  // been loaded.
  vtkGetStringMacro(PluginName);

  // Description:
  // Get the plugin version string. This returns a valid version string only
  // after the plugin has been loaded.
  vtkGetStringMacro(PluginVersion);

  // Description:
  // Get the error string if the plugin failed to load. Returns NULL if the
  // plugin was loaded successfully.
  vtkGetStringMacro(ErrorString);

  // Description:
  // Get a string of standard search paths (path1;path2;path3)
  // search paths are based on PV_PLUGIN_PATH,
  // plugin dir relative to executable.
  vtkGetStringMacro(SearchPaths);

  // Description:
  // Returns the status of most recent LoadPlugin call.
  vtkGetMacro(Loaded, bool);

  // Description:
  // Sets the function used to load static plugins.
  static void SetStaticPluginLoadFunction(vtkPluginLoadFunction function);

protected:
  vtkPVPluginLoader();
  ~vtkPVPluginLoader();

  bool LoadPluginInternal(const char* filename, bool no_errors);

  // Description:
  // Called by LoadPluginInternal() to do the final steps in loading of a
  // plugin.
  bool LoadPlugin(const char*file, vtkPVPlugin* plugin);

  vtkSetStringMacro(ErrorString);
  vtkSetStringMacro(PluginName);
  vtkSetStringMacro(PluginVersion);
  vtkSetStringMacro(FileName);
  vtkSetStringMacro(SearchPaths);

  char* ErrorString;
  char* PluginName;
  char* PluginVersion;
  char* FileName;
  char* SearchPaths;
  bool DebugPlugin;
  bool Loaded;
private:
  vtkPVPluginLoader(const vtkPVPluginLoader&); // Not implemented.
  void operator=(const vtkPVPluginLoader&); // Not implemented.

  static vtkPluginLoadFunction StaticPluginLoadFunction;
};

//BTX
// Implementation of Schwartz counter idiom to ensure that the plugin library
// unloading doesn't happen before the ParaView application is finalized.
static class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVPluginLoaderCleanerInitializer
{
public:
  vtkPVPluginLoaderCleanerInitializer();
  ~vtkPVPluginLoaderCleanerInitializer();
} vtkPVPluginLoaderCleanerInitializerInstance; // object here in header.
//ETX
#endif
