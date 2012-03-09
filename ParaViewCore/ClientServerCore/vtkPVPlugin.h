/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlugin.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPlugin - defines the core interface for any ParaView plugin. 
// .SECTION Description
// vtkPVPlugin defines the core interface for any ParaView plugin. A plugin
// implementing merely this interface is pretty much useless.
// The header file also defines few import macros that are required for
// exporting/importing plugins.
//
// When debugging issues with plugins try setting the PV_PLUGIN_DEBUG
// environment variable on all the processes where you are trying to load the
// plugin. That will print extra information as the plugin is being loaded.
#ifndef __vtkPVPlugin_h
#define __vtkPVPlugin_h

#include "vtkObject.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_VERSION and CMAKE_CXX_COMPILER_ID
#include <vector>
#include <string>

#ifdef _WIN32
// __cdecl gives an unmangled name
# define C_DECL __cdecl
# define C_EXPORT extern "C" __declspec(dllexport)
#else
# define C_DECL
# define C_EXPORT extern "C"
#endif

class VTK_EXPORT vtkPVPlugin
{
  char* FileName;
  void SetFileName(const char* filename);
  friend class vtkPVPluginLoader;
public:
  vtkPVPlugin();
  virtual ~vtkPVPlugin();

  const char* GetFileName()
    { return this->FileName; }

  // Description:
  // Returns the name for this plugin.
  virtual const char* GetPluginName() = 0;

  // Description:
  // Returns the version for this plugin.
  virtual const char* GetPluginVersionString() = 0;

  // Description:
  // Returns true if this plugin is required on the server.
  virtual bool GetRequiredOnServer() = 0;

  // Description:
  // Returns true if this plugin is required on the client.
  virtual bool GetRequiredOnClient() = 0;

  // Description:
  // Returns a ';' separated list of plugin names required by this plugin.
  virtual const char* GetRequiredPlugins() = 0;

  // Description:
  // Provides access to binary resources compiled into the plugin.
  // This is primarily used to compile in icons and compressed help project
  // (qch) files into plugins.
  virtual void GetBinaryResources(std::vector<std::string>& resources);

  // Description:
  // Used when import plugins programmatically.
  // This must only be called after the application has initialized, more
  // specifically, all plugin managers have been created and they have
  // registered their callbacks.
  static void ImportPlugin(vtkPVPlugin* plugin);
};

typedef const char* (C_DECL *pv_plugin_query_verification_data_fptr)();
typedef vtkPVPlugin* (C_DECL *pv_plugin_query_instance_fptr)();

/// TODO: add compiler version.
#define __PV_PLUGIN_VERIFICATION_STRING__ "paraviewplugin|" CMAKE_CXX_COMPILER_ID "|" PARAVIEW_VERSION

// vtkPVPluginLoader checks for existence of this function
// to determine if the shared-library is a paraview-server-manager plugin or
// not. The returned value is used to match paraview version/compiler version
// etc. These global functions are added only for shared builds. In static
// builds, plugins cannot be loaded at runtime (only at compile time) so
// verification is not necessary.
#ifdef BUILD_SHARED_LIBS
# define __PV_PLUGIN_GLOBAL_FUNCTIONS(PLUGIN) \
  C_EXPORT const char* C_DECL pv_plugin_query_verification_data()\
  { return __PV_PLUGIN_VERIFICATION_STRING__; } \
  C_EXPORT vtkPVPlugin* C_DECL pv_plugin_instance() \
  { return pv_plugin_instance_##PLUGIN(); }
#else // BUILD_SHARED_LIBS
// define empty export. When building static, we don't want to define the global
// functions.
# define __PV_PLUGIN_GLOBAL_FUNCTIONS(PLUGIN)
#endif // BUILD_SHARED_LIBS

// vtkPVPluginLoader uses this function to obtain the vtkPVPlugin instance  for
// this plugin. In a plugin, there can only be one call to this macro. When
// using the CMake macro ADD_PARAVIEW_PLUGIN, you don't have to worry about
// this, the CMake macro takes care of it.
# define PV_PLUGIN_EXPORT(PLUGIN, PLUGINCLASS) \
  C_EXPORT vtkPVPlugin* C_DECL pv_plugin_instance_##PLUGIN()  \
  { \
    static PLUGINCLASS instance;\
    return &instance;\
  }\
  __PV_PLUGIN_GLOBAL_FUNCTIONS(PLUGIN);

// PV_PLUGIN_IMPORT_INIT and PV_PLUGIN_IMPORT are provided to make it possible
// to import a plugin at compile time. In static builds, the only way to use a
// plugin is by explicitly importing it using these macros.
// PV_PLUGIN_IMPORT_INIT must be typically placed at after the #include's in a
// cxx file, while PV_PLUGIN_IMPORT must be called at a point after all the
// plugin managers for the application, including the vtkSMPluginManager,
// have been initialized.
# define PV_PLUGIN_IMPORT_INIT(PLUGIN) \
  extern "C" vtkPVPlugin* pv_plugin_instance_##PLUGIN();

# define PV_PLUGIN_IMPORT(PLUGIN)\
  vtkPVPlugin::ImportPlugin(pv_plugin_instance_##PLUGIN());

#endif // __vtkPVPlugin_h

