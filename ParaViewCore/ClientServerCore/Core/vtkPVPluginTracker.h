/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginTracker.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVPluginTracker
 * @brief   a global manager for each processes to keep track
 * of plugins loaded on that process.
 *
 * vtkPVPluginTracker is a singleton that's present on each process to keep
 * track of plugins loaded on that process. Whenever is plugin is loaded (either
 * statically using PV_PLUGIN_IMPORT() or dynamically, it gets registered with
 * the  on every process that it is loaded.
 * Whenever a plugin is registered, this class fires a vtkCommand::RegisterEvent
 * that handlers can listen to, to process the plugin.
*/

#ifndef vtkPVPluginTracker_h
#define vtkPVPluginTracker_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"                 // needed  for vtkSmartPointer;

class vtkPVPlugin;
class vtkPVXMLElement;

typedef bool (*vtkPluginSearchFunction)(const char*);

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVPluginTracker : public vtkObject
{
public:
  static vtkPVPluginTracker* New();
  vtkTypeMacro(vtkPVPluginTracker, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton. This will create the vtkPVPluginTracker
   * singleton the first time this method is called.
   */
  static vtkPVPluginTracker* GetInstance();

  /**
   * Called by vtkPVPluginLoader after a plugin is loaded on the process. This
   * registers the plugin instance with the manager. It fires an event
   * (vtkCommand::RegisterEvent)
   * signalling that a plugin was loaded. Handlers that the process the plugin
   * by detecting the interfaces implemented by the plugin and the processing
   * those on a case-by-case basis.
   * Note there's no call to unregister a plugin. Once a plugin has been loaded,
   * it cannot be unloaded for the lifetime of the process.
   */
  void RegisterPlugin(vtkPVPlugin*);

  /**
   * This API is used to register available plugins without actually loading
   * them.
   */
  unsigned int RegisterAvailablePlugin(const char* filename);

  //@{
  /**
   * Called to load application-specific configuration xml. The xml is of the
   * form:
   * @code
   * <Plugins>
   * <Plugin name="[plugin name]" filename="[optional file name] auto_load="[bool]" />
   * ...
   * </Plugins>
   * @endcode
   * This method will process the XML, locate the plugin shared library and
   * either load the plugin or call RegisterAvailablePlugin based on the status
   * of the auto_load flag. auto_load flag is optional and is 0 by default.
   * filaname is also optional, if not provided this method will look in
   * different place to find the plugin, eg. paraview lib dir. It will NOT look
   * in PV_PLUGIN_PATH.
   */
  void LoadPluginConfigurationXML(const char* filename, bool forceLoad = false);
  void LoadPluginConfigurationXML(vtkPVXMLElement*, bool forceLoad = false);
  void LoadPluginConfigurationXMLFromString(const char* xmlcontents, bool forceLoad = false);
  //@}

  /**
   * Methods to iterate over registered plugins.
   */
  unsigned int GetNumberOfPlugins();

  /**
   * Returns the plugin instance. This is non-null only for loaded plugins. If
   * a plugin was merely registered as a "available" plugin, then one can only
   * use the methods to query some primitive information about that plugin.
   */
  vtkPVPlugin* GetPlugin(unsigned int index);

  //@{
  /**
   * This is provided for wrapped languages since they can't directly access the
   * vtkPVPlugin instance.
   */
  const char* GetPluginName(unsigned int index);
  const char* GetPluginFileName(unsigned int index);
  bool GetPluginLoaded(unsigned int index);
  bool GetPluginAutoLoad(unsigned int index);
  //@}

  /**
   * Sets the function used to load static plugins.
   */
  static void SetStaticPluginSearchFunction(vtkPluginSearchFunction function);

protected:
  vtkPVPluginTracker();
  ~vtkPVPluginTracker() override;

private:
  vtkPVPluginTracker(const vtkPVPluginTracker&) = delete;
  void operator=(const vtkPVPluginTracker&) = delete;

  class vtkPluginsList;
  vtkPluginsList* PluginsList;

  static vtkPluginSearchFunction StaticPluginSearchFunction;
};

#endif

// VTK-HeaderTest-Exclude: vtkPVPluginTracker.h
