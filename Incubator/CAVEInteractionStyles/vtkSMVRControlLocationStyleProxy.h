// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRControlLocationStyleProxy
 * @brief   an interaction style to control the position of a point with a tracker
 *
 * vtkSMVRControlLocationStyleProxy is an interaction style that uses only the position
 * of the tracker to control the position of a 3D point.
 */
#ifndef vtkSMVRControlLocationStyleProxy_h
#define vtkSMVRControlLocationStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkTransform;
class vtkMatrix4x4;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRControlLocationStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRControlLocationStyleProxy* New();
  vtkTypeMacro(vtkSMVRControlLocationStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

  void SetDeferredUpdate(bool deferred);
  bool GetDeferredUpdate();

protected:
  vtkSMVRControlLocationStyleProxy();
  ~vtkSMVRControlLocationStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;
  vtkPVXMLElement* SaveConfiguration() override;
  bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator) override;

private:
  vtkSMVRControlLocationStyleProxy(
    const vtkSMVRControlLocationStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRControlLocationStyleProxy&) = delete; // Not implemented

  bool Enabled;
  bool InitialPositionRecorded;
  double Origin[4];
  bool FirstUpdate;
  double OriginalOrigin[3];
  bool DeferredUpdate;

  vtkNew<vtkMatrix4x4> InitialInvertedPose;
  vtkNew<vtkMatrix4x4> ModelTransformMatrix;
  vtkNew<vtkMatrix4x4> InverseModelTransformMatrix;
  vtkNew<vtkMatrix4x4> TrackerMatrix;
  vtkNew<vtkMatrix4x4> TransformMatrix;
};

#endif // vtkSMVRControlLocationStyleProxy_h
