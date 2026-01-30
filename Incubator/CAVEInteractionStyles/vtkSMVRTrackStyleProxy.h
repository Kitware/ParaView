// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRTrackStyleProxy
 * @brief   interactor style for head tracking
 *
 * vtkSMVRTrackStyleProxy is the simplest interface style, it simply maps
 * head tracking data from one or more trackers to the eye position and
 * orientation of one or more viewers.
 */
#ifndef vtkSMVRTrackStyleProxy_h
#define vtkSMVRTrackStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

#include <memory>

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRTrackStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRTrackStyleProxy* New();
  vtkTypeMacro(vtkSMVRTrackStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetControlledPropertySize() override { return 0; }
  void SetControlledProxy(vtkSMProxy*) override;

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

  // Overridden to query EyeSeparation property value and apply it to the
  // currently controlled proxy.
  void UpdateVTKObjects() override;

protected:
  vtkSMVRTrackStyleProxy();
  ~vtkSMVRTrackStyleProxy() override = default;
  void HandleTracker(const vtkVREvent& event) override;

  vtkPVXMLElement* SaveConfiguration() override;
  bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator) override;

private:
  vtkSMVRTrackStyleProxy(const vtkSMVRTrackStyleProxy&) = delete; // Not implemented.
  void operator=(const vtkSMVRTrackStyleProxy&) = delete;         // Not implemented.

  class Internal;
  std::unique_ptr<Internal> Internals;
};

#endif // vtkSMVRTrackStyleProxy_h
