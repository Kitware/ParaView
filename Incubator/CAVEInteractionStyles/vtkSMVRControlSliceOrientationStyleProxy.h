// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRControlSliceOrientationStyleProxy_h
#define vtkSMVRControlSliceOrientationStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkMatrix4x4;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRControlSliceOrientationStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRControlSliceOrientationStyleProxy* New();
  vtkTypeMacro(vtkSMVRControlSliceOrientationStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

  /// Update() called to update all the remote vtkObjects and perhaps even to render.
  ///   Typically processing intensive operations go here. The method should not
  ///   be called from within the handler and is reserved to be called from an
  ///   external interaction style manager.
  bool Update() override;

protected:
  vtkSMVRControlSliceOrientationStyleProxy();
  ~vtkSMVRControlSliceOrientationStyleProxy() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool Enabled;
  bool InitialOrientationRecorded;

  double InitialQuat[4];
  double InitialTrackerQuat[4];
  double UpdatedQuat[4];
  double Normal[4];

  vtkNew<vtkMatrix4x4> InitialInvertedPose;

private:
  vtkSMVRControlSliceOrientationStyleProxy(
    const vtkSMVRControlSliceOrientationStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRControlSliceOrientationStyleProxy&) = delete; // Not implemented
};

#endif // vtkSMVRControlSliceOrientationStyleProxy_h
