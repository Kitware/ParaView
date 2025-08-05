// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRTrackStyleProxy
 * @brief   interactor style for head tracking
 *
 * vtkSMVRTrackStyleProxy is the simplest interface style, it simply maps
 * head tracking data to the eye location.
 *
 * It is expected that the RenderView EyeTransformMatrix is the property
 * that will be connected to the head tracker.
 */
#ifndef vtkSMVRTrackStyleProxy_h
#define vtkSMVRTrackStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkMatrix4x4;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRTrackStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRTrackStyleProxy* New();
  vtkTypeMacro(vtkSMVRTrackStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetControlledPropertySize() override { return 16; }

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRTrackStyleProxy();
  ~vtkSMVRTrackStyleProxy() override = default;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRTrackStyleProxy(const vtkSMVRTrackStyleProxy&) = delete; // Not implemented.
  void operator=(const vtkSMVRTrackStyleProxy&) = delete;         // Not implemented.

  vtkNew<vtkMatrix4x4> TrackerMatrix;
};

#endif // vtkSMVRTrackStyleProxy_h
