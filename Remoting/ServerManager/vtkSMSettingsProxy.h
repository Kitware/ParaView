/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSettingsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMSettingsProxy
 * @brief proxy subclass responsible for linking settings
 * proxies and settings classes on the VTK side.
 *
 * Settings proxies are session specific singleton proxies designed to track
 * configuration options. Typically, properties on such settings proxies are
 * linked to other proxies in the session so that changing a setting property
 * will result in changing all linked proxies.
 *
 * Settings proxies have several unique traits when compared with other
 * properties:
 *
 * 1. In `ReadXMLAttributes`, if for a property an `information_property`
 *    attribute is specified i.e. `vtkSMProperty::GetInformationProperty` is
 *    not null, then a link is setup between that information property and the
 *    original property so that if the information property is modified, the
 *    original property is updated accordingly.
 * 2. Settings proxies may have an underlying vtkObject class that they instantiate
 *    locally. In which case, the proxy observes vtkCommand::ModifiedEvent on that
 *    VTK object and calls `vtkSMProxy::UpdatePropertyInformation` on itself to
 *    ensure all information properties are updated if the vtkObject changes.
 *
 * `vtkSMSettingsProxy::ProcessPropertyLinks` is called during proxy
 * initialization (vtkSMParaViewPipelineController::PreInitializeProxy) to setup
 * links between the proxy being initialized any of the settings proxies for
 * that session. This is done by processing property hints on the proxy being
 * initialized.
 *
 * @section SettingsAndStateFiles SettingsProxies and state files
 *
 * By default, settings proxies are not saved in the XML state files by
 * `vtkSMSessionProxyManager::SaveXMLState`. This is so since settings proxies
 * are often used for application settings to preserve across sessions rather
 * than visualization state. However, there are exceptions. For example, color
 * palettes which are handled using a settings proxy are part of the
 * visualization state and hence should be saved in the XML state. For such
 * settings proxies, one can add the `serializable="1"` attribute in the proxy
 * definition XML.
 *
 * @code{xml}
 *   <SettingsProxy name="ColorPalette" label="Color Palette" serializable="1">
 *   ...
 *   </SettingsProxy>
 * @endcode
 *
 * All settings proxies with this attribute will be saved in the XML state
 * files.
 */

#ifndef vtkSMSettingsProxy_h
#define vtkSMSettingsProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSettingsProxy : public vtkSMProxy
{
public:
  static vtkSMSettingsProxy* New();
  vtkTypeMacro(vtkSMSettingsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to link information properties with their corresponding
   * "setter" properties.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  //@{
  /**
   * Process property link hints on the given proxy.
   */
  static void ProcessPropertyLinks(vtkSMProxy* proxy);
  //@}

  //@{
  /**
   * Add/Remove links. If `unlink_if_modified` is true, if a linked property on the target
   * proxy is modified independently, then the link is broken to avoid
   * overriding the explicitly selected value.
   */
  void AddLink(const char* sourceProperty, vtkSMProxy* target, const char* targetProperty,
    bool unlink_if_modified);
  void RemoveLink(const char* sourceProperty, vtkSMProxy* target, const char* targetProperty);
  //@}

  /**
   * Returns the name of the sourceProperty, if any, with which the target
   * property is linked. Returns nullptr if no such link has been added.
   */
  const char* GetSourcePropertyName(vtkSMProxy* target, const char* targetProperty);

  //@{
  /**
   * Save/Load the state for links registered with this settings proxy.
   */
  void SaveLinksState(vtkPVXMLElement* root);
  void LoadLinksState(vtkPVXMLElement* root, vtkSMProxyLocator* locator);
  //@}

  //@{
  /**
   * Get/Set whether the settings proxy is serializable.
   */
  vtkSetMacro(IsSerializable, bool);
  vtkGetMacro(IsSerializable, bool);
  //@}

protected:
  vtkSMSettingsProxy();
  ~vtkSMSettingsProxy() override;

  /**
   * Overridden from vtkSMProxy to install an observer on the VTK object
   */
  void CreateVTKObjects() override;

  void SetPropertyModifiedFlag(const char* name, int flag) override;

private:
  vtkSMSettingsProxy(const vtkSMSettingsProxy&) = delete;
  void operator=(const vtkSMSettingsProxy&) = delete;

  /**
   * Called whenever the VTK object is modified
   */
  void VTKObjectModified();

  void AutoBreakMapPropertyLink(vtkObject*, unsigned long, void*);

  unsigned long VTKObjectObserverId = 0;
  bool IsSerializable = false;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
