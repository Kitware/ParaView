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
/**
 * @class   vtkPVPlugin
 * @brief   defines the core interface for any ParaView plugin.
 *
 * vtkPVPlugin defines the core interface for any ParaView plugin. A plugin
 * implementing merely this interface is pretty much useless.
 * The header file also defines few import macros that are required for
 * exporting/importing plugins.
 *
 * When debugging issues with plugins try setting the
 * `PARAVIEW_LOG_PLUGIN_VERBOSITY=<level>`  environment variable on all the processes
 * where you are trying to load the plugin. That will print extra information as
 * the plugin is being loaded. See `vtkPVLogger::SetPluginVerbosity` for
 * details.
 */

#ifndef vtkPVPlugin_h
#define vtkPVPlugin_h

#include "vtkObject.h"
#include "vtkPVConfig.h"           // needed for PARAVIEW_VERSION
#include "vtkRemotingCoreModule.h" //needed for exports
#include <string>
#include <vector>

#ifdef _WIN32
// __cdecl gives an unmangled name
#define C_DECL __cdecl
#define C_EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__)
#define C_DECL
#define C_EXPORT extern "C" __attribute__((visibility("default")))
#else
#define C_DECL
#define C_EXPORT extern "C"
#endif

class VTKREMOTINGCORE_EXPORT vtkPVPlugin
{
public:
  vtkPVPlugin();
  virtual ~vtkPVPlugin();

  const char* GetFileName() { return this->FileName; }

  /**
   * Returns the name for this plugin.
   */
  virtual const char* GetPluginName() = 0;

  /**
   * Returns the version for this plugin.
   */
  virtual const char* GetPluginVersionString() = 0;

  /**
   * Returns true if this plugin is required on the server.
   */
  virtual bool GetRequiredOnServer() = 0;

  /**
   * Returns true if this plugin is required on the client.
   */
  virtual bool GetRequiredOnClient() = 0;

  /**
   * Returns a ';' separated list of plugin names required by this plugin.
   */
  virtual const char* GetRequiredPlugins() = 0;

  /**
   * Returns a description of this plugin.
   */
  virtual const char* GetDescription() = 0;

  /**
   * Returns EULA for the plugin, if any. If none, this will return nullptr.
   */
  virtual const char* GetEULA() = 0;

  /**
   * Provides access to binary resources compiled into the plugin.
   * This is primarily used to compile in icons and compressed help project
   * (qch) files into plugins.
   */
  virtual void GetBinaryResources(std::vector<std::string>& resources);

  //@{
  /**
   * Used when import plugins programmatically.
   * This must only be called after the application has initialized, more
   * specifically, all plugin managers have been created and they have
   * registered their callbacks.
   *
   * Note, if the plugin has EULA and the user declines the EULA, the import
   * request will be ignored, and the plugin won't be imported. This does not
   * mean, however, that the plugin won't have any side effects as the plugin
   * library can have singletons that get initialized on library load.
   *
   * @returns true if the plugin was successfully imported.
   */
  static bool ImportPlugin(vtkPVPlugin* plugin);

  /**
   * Type for EULAConfirmationCallback
   */
  typedef bool (*EULAConfirmationCallback)(vtkPVPlugin*);

  //@{
  /**
   * Get/Set the static callback to call to confirm EULA
   */
  static void SetEULAConfirmationCallback(EULAConfirmationCallback callback);
  static EULAConfirmationCallback GetEULAConfirmationCallback();
  //@}

protected:
  /**
   * Set the filename the plugin is loaded from, if any. If it's nullptr, then
   * its assumed that the plugin is "linked-in" i.e. not loaded from an external
   * file.
   */
  void SetFileName(const char* filename);

private:
  /**
   * Called to confirm EULA in `ImportPlugin` if the plugin has a non-empty EULA.
   * Based on whether EULAConfirmationCallback is specified, this will
   * accept the EULA and print a message on the terminal or prompt the user via
   * the callback to accept the EULA.
   */
  static bool ConfirmEULA(vtkPVPlugin* plugin);

  char* FileName;

  static EULAConfirmationCallback EULAConfirmationCallbackPtr;
  friend class vtkPVPluginLoader;

private:
  vtkPVPlugin(const vtkPVPlugin&) = delete;
  void operator=(const vtkPVPlugin&) = delete;
};
//@}

#ifndef __WRAP__
typedef vtkPVPlugin*(C_DECL* pv_plugin_query_instance_fptr)();
#endif

#ifdef PARAVIEW_BUILDING_PLUGIN

// vtkPVPluginLoader checks for existence of this function
// to determine if the shared-library is a paraview-server-manager plugin or
// not. The returned value is used to match paraview version/compiler version
// etc. These global functions are added only for shared builds. In static
// builds, plugins cannot be loaded at runtime (only at compile time) so
// verification is not necessary.
#if PARAVIEW_PLUGIN_BUILT_SHARED
#define _PV_PLUGIN_GLOBAL_FUNCTIONS(PLUGIN)                                                        \
  C_EXPORT vtkPVPlugin* C_DECL pv_plugin_instance() { return pv_plugin_instance_##PLUGIN(); }
#else
// define empty export. When building static, we don't want to define the global
// functions.
#define _PV_PLUGIN_GLOBAL_FUNCTIONS(PLUGIN)
#endif

// vtkPVPluginLoader uses this function to obtain the vtkPVPlugin instance  for
// this plugin. In a plugin, there can only be one call to this macro. When
// using the CMake macro ADD_PARAVIEW_PLUGIN, you don't have to worry about
// this, the CMake macro takes care of it.
#define PV_PLUGIN_EXPORT(PLUGIN, PLUGINCLASS)                                                      \
  C_EXPORT vtkPVPlugin* C_DECL pv_plugin_instance_##PLUGIN()                                       \
  {                                                                                                \
    static PLUGINCLASS instance;                                                                   \
    return &instance;                                                                              \
  }                                                                                                \
  _PV_PLUGIN_GLOBAL_FUNCTIONS(PLUGIN);

// PV_PLUGIN_IMPORT_INIT and PV_PLUGIN_IMPORT are provided to make it possible
// to import a plugin at compile time. In static builds, the only way to use a
// plugin is by explicitly importing it using these macros.
// PV_PLUGIN_IMPORT_INIT must be typically placed at after the #include's in a
// cxx file, while PV_PLUGIN_IMPORT must be called at a point after all the
// plugin managers for the application, including the vtkSMPluginManager,
// have been initialized.
#define PV_PLUGIN_IMPORT_INIT(PLUGIN) extern "C" vtkPVPlugin* pv_plugin_instance_##PLUGIN();

#define PV_PLUGIN_IMPORT(PLUGIN) vtkPVPlugin::ImportPlugin(pv_plugin_instance_##PLUGIN());

#endif

#endif // vtkPVPlugin_h
// VTK-HeaderTest-Exclude: vtkPVPlugin.h
