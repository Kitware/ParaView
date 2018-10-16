/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonAlgorithmPlugin.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVPythonAlgorithmPlugin
 * @brief packages a Python module into a ParaView plugin.
 *
 * vtkPVPythonAlgorithmPlugin helps us support loading a Python module as a
 * ParaView `plugin`. The only supported type of plugin is a server-manager
 * plugin that adds support for algorithm proxies i.e. readers, filters, and
 * writers.
 */

#ifndef vtkPVPythonAlgorithmPlugin_h
#define vtkPVPythonAlgorithmPlugin_h

#include "vtkPVPlugin.h"

#include "vtkPVPythonAlgorithmModule.h"        // for exports
#include "vtkPVServerManagerPluginInterface.h" // for vtkPVServerManagerPluginInterface

#include <memory> //for std::unique_ptr.

class vtkPVPluginLoaderCleanerInitializer;

class VTKPVPYTHONALGORITHM_EXPORT vtkPVPythonAlgorithmPlugin
  : public vtkPVPlugin,
    public vtkPVServerManagerPluginInterface
{
public:
  vtkPVPythonAlgorithmPlugin(const char* pythonmodule);
  ~vtkPVPythonAlgorithmPlugin() override;

  //@{
  /// Implementation of the vtkPVPlugin interface.
  const char* GetPluginName() override;
  const char* GetPluginVersionString() override;
  bool GetRequiredOnServer() override { return true; }
  bool GetRequiredOnClient() override { return false; }
  const char* GetRequiredPlugins() override { return ""; }
  const char* GetDescription() override { return ""; }
  const char* GetEULA() override { return nullptr; }
  //@}

  //@{
  /// Implementation of the vtkPVServerManagerPluginInterface.
  void GetXMLs(std::vector<std::string>& xmls) override;
  vtkClientServerInterpreterInitializer::InterpreterInitializationCallback
  GetInitializeInterpreterCallback() override
  {
    return nullptr;
  }
  //@}

private:
  vtkPVPythonAlgorithmPlugin(const vtkPVPythonAlgorithmPlugin&) = delete;
  void operator=(const vtkPVPythonAlgorithmPlugin&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  friend class vtkPVPythonAlgorithmPluginLoaderInitializer;
  /**
   * Callback registered with vtkPVPluginLoader to load a plugin. If the plugin
   * refers to a Python plugin package, this method will attempt to load it.
   */
  static bool LoadPlugin(const char* pname);
};

// Schwartz counter idiom to register loader for Python plugins (based on Python
// algorithm).
static class VTKPVPYTHONALGORITHM_EXPORT vtkPVPythonAlgorithmPluginLoaderInitializer
{
public:
  vtkPVPythonAlgorithmPluginLoaderInitializer();
  ~vtkPVPythonAlgorithmPluginLoaderInitializer();

} PythonAlgorithmPluginInitializerInstance;

#endif
// VTK-HeaderTest-Exclude: vtkPVPythonAlgorithmPlugin.h
