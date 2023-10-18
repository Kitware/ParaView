// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRControlSlicePositionStyleProxy_h
#define vtkSMVRControlSlicePositionStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkTransform;
class vtkMatrix4x4;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRControlSlicePositionStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRControlSlicePositionStyleProxy* New();
  vtkTypeMacro(vtkSMVRControlSlicePositionStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

  /// Update() called to update all the remote vtkObjects and perhaps even to render.
  ///   Typically processing intensive operations go here. The method should not
  ///   be called from within the handler and is reserved to be called from an
  ///   external interaction style manager.
  bool Update() override;

protected:
  vtkSMVRControlSlicePositionStyleProxy();
  ~vtkSMVRControlSlicePositionStyleProxy() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool Enabled;
  bool InitialPositionRecorded;
  double Origin[4];
  vtkNew<vtkMatrix4x4> InitialInvertedPose;

private:
  vtkSMVRControlSlicePositionStyleProxy(
    const vtkSMVRControlSlicePositionStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRControlSlicePositionStyleProxy&) = delete; // Not implemented
};

#endif // vtkSMVRControlSlicePositionStyleProxy_h
