// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkPVServerManagerPluginInterface.h"    // for vtkPVServerManagerPluginInterface
#include "vtkRemotingServerManagerPythonModule.h" // for exports

#include <memory> //for std::unique_ptr.

class vtkPVPluginLoaderCleanerInitializer;

class VTKREMOTINGSERVERMANAGERPYTHON_EXPORT vtkPVPythonAlgorithmPlugin
  : public vtkPVPlugin
  , public vtkPVServerManagerPluginInterface
{
public:
  ///@{
  /**
   * Constructors for the object. The first version reads the python
   * plugin from the .py file at 'filePath'. The second version is
   * used for plugins that include both C++ and Python filters (the
   * Python source code is included in the .so in this case)
   */
  vtkPVPythonAlgorithmPlugin(const char* filePath);
  vtkPVPythonAlgorithmPlugin(const char* moduleName, const char* pythonSourceCode);
  ///@}
  ~vtkPVPythonAlgorithmPlugin() override;

  ///@{
  /// Implementation of the vtkPVPlugin interface.
  const char* GetPluginName() override;
  const char* GetPluginVersionString() override;
  bool GetRequiredOnServer() override { return true; }
  bool GetRequiredOnClient() override { return false; }
  const char* GetRequiredPlugins() override { return ""; }
  const char* GetDescription() override { return ""; }
  const char* GetEULA() override { return nullptr; }
  ///@}

  ///@{
  /// Implementation of the vtkPVServerManagerPluginInterface.
  void GetXMLs(std::vector<std::string>& xmls) override;
  vtkClientServerInterpreterInitializer::InterpreterInitializationCallback
  GetInitializeInterpreterCallback() override
  {
    return nullptr;
  }
  ///@}
  /**
   * Creates the object from Python source code and gets the servermanager XMLs
   * from all the Python filters included in the 'moduleName'.
   */
  static bool InitializeFromStringAndGetXMLs(
    const char* moduleName, const char* pythonSourceCode, std::vector<std::string>& xmls);

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
  /**
   * Initialize function for the object called by the constructors.
   * 'loadPluginFunction' specifies the function to call either
   * pythonalgorithm.load_plugin or
   * pythonalgorithm.load_plugin_from_string. These two function corespond to
   * the two constructors.
   * 'pythonSourceCode' should be nullptr for the first function, and point to
   * the plugin python source code for the second.
   */
  void Initialize(
    const char* moduleNameOrFilePath, const char* loadPluginFunction, const char* pythonSourceCode);
};

// Schwartz counter idiom to register loader for Python plugins (based on Python
// algorithm).
static class VTKREMOTINGSERVERMANAGERPYTHON_EXPORT vtkPVPythonAlgorithmPluginLoaderInitializer
{
public:
  vtkPVPythonAlgorithmPluginLoaderInitializer();
  ~vtkPVPythonAlgorithmPluginLoaderInitializer();

} PythonAlgorithmPluginInitializerInstance;

#endif
// VTK-HeaderTest-Exclude: vtkPVPythonAlgorithmPlugin.h
