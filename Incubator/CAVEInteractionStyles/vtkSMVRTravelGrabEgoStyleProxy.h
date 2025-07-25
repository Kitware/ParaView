// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRTravelGrabEgoStyleProxy
 * @brief   an interaction style to control a matrix
 *
 * vtkSMVRTravelGrabEgoStyleProxy is an interaction style that uses a single
 * button, along with tracker position and orientation, to control a 4x4 matrix
 * property (mostly only the ModelTransformMatrix of a vtkSMRenderViewProxy),
 * The world/scene appears to orbit about the tracker, hence the name contains
 * "Ego" rather than "Exo".
 */
#ifndef vtkSMVRTravelGrabEgoStyleProxy_h
#define vtkSMVRTravelGrabEgoStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRTravelGrabEgoStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRTravelGrabEgoStyleProxy* New();
  vtkTypeMacro(vtkSMVRTravelGrabEgoStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetControlledPropertySize() override { return 16; }

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRTravelGrabEgoStyleProxy();
  ~vtkSMVRTravelGrabEgoStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRTravelGrabEgoStyleProxy(const vtkSMVRTravelGrabEgoStyleProxy&) = delete;
  void operator=(const vtkSMVRTravelGrabEgoStyleProxy&) = delete;

  // mirrors the button assigned the "Navigate World" role and indicates
  // whether we're in the middel of a navigation operation
  bool EnableNavigate;

  // Have we captured the initial state when the button was first pressed?
  bool IsInitialRecorded;

  vtkNew<vtkMatrix4x4> SavedPropertyMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseWandMatrix;
  vtkNew<vtkMatrix4x4> TransformMatrix;
  vtkNew<vtkMatrix4x4> TrackerMatrix;
};

#endif // vtkSMVRTravelGrabEgoStyleProxy_h
