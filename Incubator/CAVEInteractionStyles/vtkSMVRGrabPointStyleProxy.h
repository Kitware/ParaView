// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRGrabPointStyleProxy_h
#define vtkSMVRGrabPointStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRGrabPointStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRGrabPointStyleProxy* New();
  vtkTypeMacro(vtkSMVRGrabPointStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetControlledPropertySize() override { return 3; }

  /// Update() called to update all the remote vtkObjects and perhaps even to render.
  ///   Typically processing intensive operations go here. The method should not
  ///   be called from within the handler and is reserved to be called from an
  ///   external interaction style manager.
  bool Update() override;

protected:
  vtkSMVRGrabPointStyleProxy();
  ~vtkSMVRGrabPointStyleProxy() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableNavigate;    /* mirrors the button assigned the "Navigate World" role */
  bool IsInitialRecorded; /* flag indicating that we're in the middle of a navigation operation */
  double Origin[4];

  vtkNew<vtkMatrix4x4> SavedModelViewMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseWandMatrix;

private:
  vtkSMVRGrabPointStyleProxy(const vtkSMVRGrabPointStyleProxy&) = delete; // Not implemented
  void operator=(const vtkSMVRGrabPointStyleProxy&) = delete;             // Not implemented

  float GetSpeedFactor(vtkCamera* cam); // WRS-TODO: what does this do?
};

#endif // vtkSMVRGrabPointStyleProxy_h
