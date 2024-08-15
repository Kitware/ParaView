// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRGrabTransformStyleProxy_h
#define vtkSMVRGrabTransformStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRGrabTransformStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRGrabTransformStyleProxy* New();
  vtkTypeMacro(vtkSMVRGrabTransformStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRGrabTransformStyleProxy();
  ~vtkSMVRGrabTransformStyleProxy() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableNavigate;    /* mirrors the button assigned the "Navigate World" role */
  bool IsInitialRecorded; /* flag indicating that we're in the middle of a navigation operation */

  vtkNew<vtkMatrix4x4> SavedModelViewMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseWandMatrix;

private:
  vtkSMVRGrabTransformStyleProxy(const vtkSMVRGrabTransformStyleProxy&) = delete;
  void operator=(const vtkSMVRGrabTransformStyleProxy&) = delete;

  float GetSpeedFactor(vtkCamera* cam, vtkMatrix4x4* mvmatrix); /* WRS-TODO: what does this do? */
};

#endif // vtkSMVRGrabTransformStyleProxy_h
