// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVPluginsInformation
 * @brief   information about plugins tracked by
 * vtkPVPluginTracker.
 *
 * vtkPVPluginsInformation is used to collect information about plugins tracked
 * by vtkPVPluginTracker.
 */

#ifndef vtkPVPluginsInformation_h
#define vtkPVPluginsInformation_h

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" // needed for exports

class VTKREMOTINGCORE_EXPORT vtkPVPluginsInformation : public vtkPVInformation
{
public:
  static vtkPVPluginsInformation* New();
  vtkTypeMacro(vtkPVPluginsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to iterate over the information collected for each plugin.
   */
  unsigned int GetNumberOfPlugins();
  const char* GetPluginName(unsigned int);
  const char* GetPluginFileName(unsigned int);
  const char* GetPluginVersion(unsigned int);
  bool GetPluginLoaded(unsigned int);
  const char* GetRequiredPlugins(unsigned int);
  bool GetRequiredOnServer(unsigned int);
  bool GetRequiredOnClient(unsigned int);
  const char* GetDescription(unsigned int);
  bool GetAutoLoad(unsigned int);
  bool GetDelayedLoad(unsigned int);
  std::vector<std::string> GetXMLs(unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the plugin status message for a specific plugin
   */
  const char* GetPluginStatusMessage(unsigned int);
  void SetPluginStatusMessage(unsigned int, const char*);
  ///@}

  /**
   * API to change auto-load status.
   */
  void SetAutoLoad(unsigned int cc, bool);

  /**
   * API to change DelayedLoad status.
   */
  void SetDelayedLoad(unsigned int cc, bool);

  /**
   * This is a hack. When the user sets an auto-load option from  the GUI to
   * avoid that choice being overwritten as the information object is updated
   * over time as new plugins are loaded/unloaded, the pqPluginDialog uses this
   * method to set the auto-load flag. This flag is not communicated across
   * processes, but when called, GetAutoLoad() will return the value set using
   * this method.
   */
  void SetAutoLoadAndForce(unsigned int cc, bool);

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  /**
   * Updates the local information with elements from other without overriding
   * auto-load state.
   */
  void Update(vtkPVPluginsInformation* other);

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

  ///@{
  /**
   * Get the plugin search path.
   */
  vtkGetStringMacro(SearchPaths);
  ///@}

protected:
  vtkPVPluginsInformation();
  ~vtkPVPluginsInformation() override;

  char* SearchPaths;
  vtkSetStringMacro(SearchPaths);

private:
  vtkPVPluginsInformation(const vtkPVPluginsInformation&) = delete;
  void operator=(const vtkPVPluginsInformation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
