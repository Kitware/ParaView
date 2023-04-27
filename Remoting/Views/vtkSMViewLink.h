/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMViewLink
 * @brief   create a link between views, with automatic refresh.
 *
 * vtkSMViewLink is a proxy link to synchronize views properties.
 *
 * Camera properties can be excluded from the link.
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
   * Callback to render output views.
   */
  static void UpdateViewCallback(
    vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  /**
   * Update all the views linked with OUTPUT direction.
   */
  virtual void UpdateViews(vtkSMProxy* caller);

protected:
  vtkSMViewLink() = default;
  ~vtkSMViewLink() override;

  /**
   * Called when an input proxy is updated (UpdateEvent).
   * Argument is the input proxy.
   * Reimplemented to force an update on the view.
   */
  void UpdateVTKObjects(vtkSMProxy* proxy) override;

  std::map<vtkSMProxy*, vtkSmartPointer<vtkCallbackCommand>> RenderObservers;

  bool Updating = false;
};

#endif
