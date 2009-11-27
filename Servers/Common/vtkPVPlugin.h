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
// .NAME vtkPVPlugin
// .SECTION Description
//

#ifndef __vtkPVPlugin_h
#define __vtkPVPlugin_h

#include "vtkObject.h"
#include "vtkPVConfig.h" // needed for PARAVIEW_VERSION and CMAKE_CXX_COMPILER_ID
#include <vtkstd/vector>
#include <vtkstd/string>

#ifdef _WIN32
// __cdecl gives an unmangled name
# define C_DECL __cdecl
# define C_EXPORT extern "C" __declspec(dllexport)
#else
# define C_DECL
# define C_EXPORT extern "C"
#endif

/// vtkPVPlugin defines the core interface for any ParaView plugin. A plugin
/// implementing merely this interface is pretty much useless.
class VTK_EXPORT vtkPVPlugin
{
public:
  virtual ~vtkPVPlugin() {}

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
};

typedef const char* (C_DECL *pv_plugin_query_verification_data_fptr)();
typedef vtkPVPlugin* (C_DECL *pv_plugin_query_instance_fptr)();

/// TODO: add compiler version.
#define __PV_PLUGIN_VERIFICATION_STRING__ "paraviewplugin|" CMAKE_CXX_COMPILER_ID "|" PARAVIEW_VERSION

// vtkPVPluginLoader checks for existence of this function
// to determine if the shared-library is a paraview-server-manager plugin or
// not. The returned value is used to match paraview version/compiler version
// etc.
// TODO: handle static-builds -- i.e. these methods must not exist in static
// builds of ParaView.
#define __PV_PLUGIN_VERIFICATION_DATA(PLUGIN) \
  C_EXPORT const char* C_DECL pv_plugin_query_verification_data()\
  { return __PV_PLUGIN_VERIFICATION_STRING__; } \
  C_EXPORT vtkPVPlugin* C_DECL pv_plugin_instance() \
  { return pv_plugin_instance_##PLUGIN(); }
 
// vtkPVPluginLoader uses this function to obtain the vtkPVPlugin subclass for
// this plugin.
#define PV_PLUGIN_EXPORT(PLUGIN, PLUGINCLASS) \
  C_EXPORT PLUGINCLASS* C_DECL pv_plugin_instance_##PLUGIN()  \
  { \
    static PLUGINCLASS instance;\
    return &instance;\
  }\
  __PV_PLUGIN_VERIFICATION_DATA(PLUGIN);

// FIXME: This still needs some work to make sure all kinds of plugins work when
// compiled in statically.
//// Call this macro when linking against this plugin directly.
#define fixme_PV_PLUGIN_IMPORT(PLUGIN) \
{\
  vtkPVPlugin* plugin = pv_plugin_instance_##PLUGIN();\
  vtkPVPluginLoader* loader= vtkPVPluginLoader::New();\
  loader->Load(plugin);\
  loader->Delete();\
}

#endif

