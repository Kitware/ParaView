// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRControlOrientationStyleProxy
 * @brief   an interaction style to control a normalized (unit) 3D vector
 *
 * vtkSMVRControlOrientationStyleProxy is an interaction style that uses only the
 * orientation of the tracker to control the orientation of a 3D vector, such as
 * a normal or direction property.
 */
#ifndef vtkSMVRControlOrientationStyleProxy_h
#define vtkSMVRControlOrientationStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkMatrix4x4;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRControlOrientationStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRControlOrientationStyleProxy* New();
  vtkTypeMacro(vtkSMVRControlOrientationStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

  void SetDeferredUpdate(bool deferred);
  bool GetDeferredUpdate();

protected:
  vtkSMVRControlOrientationStyleProxy();
  ~vtkSMVRControlOrientationStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;
  vtkPVXMLElement* SaveConfiguration() override;
  bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator) override;

private:
  vtkSMVRControlOrientationStyleProxy(
    const vtkSMVRControlOrientationStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRControlOrientationStyleProxy&) = delete; // Not implemented

  bool Enabled;
  bool InitialOrientationRecorded;
  double Normal[4];
  bool FirstUpdate;
  double OriginalNormal[3];
  bool DeferredUpdate;

  vtkNew<vtkMatrix4x4> InitialInvertedPose;
  vtkNew<vtkMatrix4x4> TrackerMatrix;
  vtkNew<vtkMatrix4x4> TransformMatrix;
  vtkNew<vtkMatrix4x4> ModelTransformMatrix;
  vtkNew<vtkMatrix4x4> InverseModelTransformMatrix;
};

#endif // vtkSMVRControlOrientationStyleProxy_h
