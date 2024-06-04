// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMPluginLoaderProxy
 * @brief   used to load a plugin remotely.
 *
 * vtkSMPluginLoaderProxy is used to load a plugin on dataserver and
 * renderserver processes. Simply call vtkSMPluginLoaderProxy::LoadPlugin() with
 * the right path to load the plugin remotely.
 */

#ifndef vtkSMPluginLoaderProxy_h
#define vtkSMPluginLoaderProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMPluginLoaderProxy : public vtkSMProxy
{
public:
  static vtkSMPluginLoaderProxy* New();
  vtkTypeMacro(vtkSMPluginLoaderProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Loads the plugin. Returns true on success else false. To get the error
   * string, call UpdatePropertyInformation() on this proxy and then look at the
   * ErrorString property.
   */
  bool LoadPlugin(const char* filename);

  /**
   * Loads the plugin based on its name. Returns true on success else false. To get the error
   * string, call UpdatePropertyInformation() on this proxy and then look at the
   * ErrorString property.
   */
  bool LoadPluginByName(const char* name, bool acceptDelayed = true);

  /**
   * Loads the configuration xml contents. Look at
   * vtkPVPluginTracker::LoadPluginConfigurationXMLFromString() to see the
   * details about the configuration xml.
   */
  void LoadPluginConfigurationXMLFromString(const char* xmlcontents);

  /**
   * Loads a configuration xml file. Look at
   * vtkPVPluginTracker::LoadPluginConfigurationXML() to see the
   * details about the configuration xml.
   */
  void LoadPluginConfigurationXML(const char* configurationFile);

protected:
  vtkSMPluginLoaderProxy();
  ~vtkSMPluginLoaderProxy() override;

private:
  vtkSMPluginLoaderProxy(const vtkSMPluginLoaderProxy&) = delete;
  void operator=(const vtkSMPluginLoaderProxy&) = delete;
};

#endif
