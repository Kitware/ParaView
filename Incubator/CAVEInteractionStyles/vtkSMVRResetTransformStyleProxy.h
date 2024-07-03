// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRResetTransformStyleProxy_h
#define vtkSMVRResetTransformStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRResetTransformStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRResetTransformStyleProxy* New();
  vtkTypeMacro(vtkSMVRResetTransformStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRResetTransformStyleProxy();
  ~vtkSMVRResetTransformStyleProxy() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableNavigate;    /* mirrors the button assigned the "Navigate World" role */
  bool IsInitialRecorded; /* flag indicating that we're in the middle of a navigation operation */

  vtkNew<vtkMatrix4x4> SavedModelViewMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseWandMatrix;

private:
  vtkSMVRResetTransformStyleProxy(
    const vtkSMVRResetTransformStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRResetTransformStyleProxy&) = delete; // Not implemented

  float GetSpeedFactor(vtkCamera* cam); // WRS-TODO: what does this do?
};

#endif // vtkSMVRResetTransformStyleProxy_h
