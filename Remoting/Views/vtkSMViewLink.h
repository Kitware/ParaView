// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMViewLink
 * @brief   create a link between views, with automatic refresh.
 *
 * vtkSMViewLink is a proxy link to synchronize views properties.
 *
 * The "Representations" property is excluded by default
 * as representations should not be duplicated in different views.
 *
 * Camera properties can be excluded fault from the link, see EnableCameraLink.
 */

#ifndef vtkSMViewLink_h
#define vtkSMViewLink_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxyLink.h"

class vtkCallbackCommand;

class VTKREMOTINGVIEWS_EXPORT vtkSMViewLink : public vtkSMProxyLink
{
public:
  static vtkSMViewLink* New();
  vtkTypeMacro(vtkSMViewLink, vtkSMProxyLink);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Reimplemented to check proxy type.
   */
  void AddLinkedProxy(vtkSMProxy* proxy, int updateDir) override;

  /**
   * Remove a linked proxy.
   */
  void RemoveLinkedProxy(vtkSMProxy* proxy) override;

  /**
   * Enable linking of Cameras properties.
   * Uses vtkSMCameraLink to get list of properties to create or remove exception.
   */
  void EnableCameraLink(bool enable);

  /**
   * Callback to render output views. This is useful to update render view when camera changes.
   */
  static void UpdateViewCallback(
    vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /**
   * Update all the views linked with OUTPUT direction.
   */
  virtual void UpdateViews(vtkSMProxy* caller);

protected:
  vtkSMViewLink();
  ~vtkSMViewLink() override;

  /**
   * Override to ajust to this class name.
   */
  std::string GetXMLTagName() override { return "ViewLink"; }

  /**
   * Called when an input proxy is updated (UpdateEvent).
   * Argument is the input proxy.
   * Reimplemented to force an update on the view.
   */
  void UpdateVTKObjects(vtkSMProxy* proxy) override;

  std::map<vtkSMProxy*, vtkSmartPointer<vtkCallbackCommand>> RenderObservers;

  bool Updating = false;
  bool UpdateViewsOnEndEvent = true;
};

#endif
