// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRTravelGrabExoSplitStyleProxy
 * @brief   an interaction style to control a matrix
 *
 * vtkSMVRTravelGrabExoSplitStyleProxy is an interaction style that uses two
 * buttons, along with tracker position and orientation, to control a 4x4 matrix
 * property (mostly only the ModelTransformMatrix of a vtkSMRenderViewProxy),
 * The world/scene appears to orbit about the world origin, hence the name contains
 * "Exo" rather than "Ego".
 */
#ifndef vtkSMVRTravelGrabExoSplitStyleProxy_h
#define vtkSMVRTravelGrabExoSplitStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRTravelGrabExoSplitStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRTravelGrabExoSplitStyleProxy* New();
  vtkTypeMacro(vtkSMVRTravelGrabExoSplitStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRTravelGrabExoSplitStyleProxy();
  ~vtkSMVRTravelGrabExoSplitStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRTravelGrabExoSplitStyleProxy(
    const vtkSMVRTravelGrabExoSplitStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRTravelGrabExoSplitStyleProxy&) = delete; // Not implemented

  // mirrors the state of the button assigned the "Translate World" role,
  // indicating whether or not we are currently translating
  bool EnableTranslate;

  // mirrors the button assigned the "Rotate World" role, indicating
  // whether or not we are currently rotating
  bool EnableRotate;

  // Indicates whether we still need to capture the initial state after
  // button press
  bool IsInitialRecorded;

  // Saved on button down to capture the target property matrix and
  // the inverse wand matrix
  vtkNew<vtkMatrix4x4> InverseWandMatrix;
  vtkNew<vtkMatrix4x4> SavedPropertyMatrix;

  // Some matrices used for other internal computations.  Keep them
  // here to avoid creating and destroying them on every event.
  vtkNew<vtkMatrix4x4> TrackerMatrix;
  vtkNew<vtkMatrix4x4> TransformMatrix;
  vtkNew<vtkMatrix4x4> TranslateMatrix;
  vtkNew<vtkMatrix4x4> TranslateToOrigin;
  vtkNew<vtkMatrix4x4> TranslateBack;

  double CurrentTranslation[3];
};

#endif // vtkSMVRTravelGrabExoSplitStyleProxy_h
